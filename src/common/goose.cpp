#include <cstdarg>
#include "random_util.h"
#include "goose.h"
#include "assets.h"
#include "behavior.h"
#include "behaviors/states/jail_state.h"
#include "event_bus.h"
#include "config.h"
#include "goose_math.h"
#include "world.h"
#include "cursor_backend.h"
#include "log.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <mutex>

#ifdef __APPLE__
#include "goose_drawing.h"
#include "cg_renderer.h"
#include <CoreGraphics/CoreGraphics.h>
#include <mach/mach_time.h>
#endif

// --- Magic numbers extracted as named constants ---
static constexpr double kLogInterval = 0.1;
static constexpr float kSpringForce = 50.0f;
static constexpr float kSpringDamping = 0.80f;
static constexpr float kSurprisedSpeedMultiplier = 1.5f;
static constexpr float kEdgeAvoidMinForce = 0.1f;
static constexpr float kSpeedEpsilon = 1e-4f;
static constexpr float kNighttimeSpeedFactor = 0.6f;
static constexpr int kNighttimeStartHour = 23;
static constexpr int kNighttimeEndHour = 6;
static constexpr float kShudderDirChangeThreshold = 90.0f;
static constexpr int kNighttimeCheckIntervalSec = 60;

static bool IsNighttime() {
    // H6: Cache nighttime status in an atomic<bool> so every physics tick can
    // read it without acquiring a mutex. The cached value is refreshed at most
    // once per kNighttimeCheckIntervalSec using a lightweight try_lock so
    // contention never blocks the render thread.
    static std::atomic<bool> s_isNight{false};
    static std::mutex s_refreshMutex;
    static std::atomic<time_t> s_lastCheck{0};

    time_t now = std::time(nullptr);
    // Lock-free fast path: return cached value if still fresh.
    if (now - s_lastCheck.load(std::memory_order_relaxed) < kNighttimeCheckIntervalSec) {
        return s_isNight.load(std::memory_order_relaxed);
    }
    // Slow path: refresh under mutex (only one thread at a time).
    if (s_refreshMutex.try_lock()) {
        // Double-check after acquiring lock.
        if (now - s_lastCheck.load(std::memory_order_relaxed) >= kNighttimeCheckIntervalSec) {
            struct tm tm_info;
            if (localtime_r(&now, &tm_info)) {
                s_isNight.store(
                    (tm_info.tm_hour >= kNighttimeStartHour || tm_info.tm_hour < kNighttimeEndHour),
                    std::memory_order_relaxed);
            }
            s_lastCheck.store(now, std::memory_order_relaxed);
        }
        s_refreshMutex.unlock();
    }
    return s_isNight.load(std::memory_order_relaxed);
}
static constexpr float kFetchCurvatureRange = 200.0f;
static constexpr float kFetchCurvatureCenter = 100.0f;
static constexpr float kFetchCurvatureDivisor = 100.0f;
static constexpr float kTwoPi = 2.0f * PI;
static constexpr float kRadToDeg = 180.0f / PI;
static constexpr float kDegToRad = PI / 180.0f;
static constexpr double kStuckThresholdTime = 3.0;
static constexpr float kStuckMinMovementThreshold = 10.0f;

static bool s_stateChanged = true;
static double s_lastLogTime = 0;

Goose::Goose(int id_, const std::string &name_, int screenW, int screenH)
    : id(id_), name(name_) {
  pos = {(float)(rng_util::RandRange((int)(screenW - g_config.spawn.marginX * 2)) +
                 g_config.spawn.marginX),
         (float)(rng_util::RandRange((int)(screenH - g_config.spawn.marginY * 2)) +
                 g_config.spawn.marginY)};
  target = pos;
  dir = (float)(rng_util::RandRange((int)g_config.movement.initDirectionMax));
  currentSpeed = g_config.movement.baseWalkSpeed;

  attackMouseBias = 0;
  memeFetchBias = rng_util::RandRange(g_config.item.memeFetchBiasMax);
  noteFetchBias = rng_util::RandRange(g_config.item.noteFetchBiasMax);
  randomOffset = rng_util::RandFloatRange(0.0f, 3.0f);

  cursorChaseEnabled = g_config.cursor.chaseEnabled;
  cursorChaseChance = 5;
  snatchDuration = g_config.snatch.duration;
  mudEnabled = g_config.mud.enabled;
  mudChance = g_config.mud.chance;
  mudLifetime = g_config.mud.lifetime;
  ISO_SCALE = {g_config.physics.isoScaleX, g_config.physics.isoScaleY};

  // Seed feet at their home positions instead of (0, 0). Without this, the
  // first SolveFeet call uses the (0, 0) sentinel to detect "uninitialized"
  // and resets to home, but the *moveDuration* member stays at the struct's
  // default 0.2 until the first step actually triggers. Combined with the
  // single-foot-at-a-time gate in SolveFeet, this means the leg animation
  // visibly lags the body for the first few seconds after spawn — the
  // "skating" the rig had pre-first-fetch. Pre-seeding home positions and
  // step durations puts the rig in a valid walking state from frame 1.
  rig.lFoot.currentPos = GetFootHome(g_config.step.leftFootAngle);
  rig.rFoot.currentPos = GetFootHome(g_config.step.rightFootAngle);
  rig.lFoot.moveStartTime = -1.0;
  rig.rFoot.moveStartTime = -1.0;
  rig.lFoot.moveDuration = g_config.step.durationWalk;
  rig.rFoot.moveDuration = g_config.step.durationWalk;

  PickNewTarget(screenW, screenH);
}

void Goose::onHonk() {
  if (g_config.general.appearanceMode == APPEARANCE_STALIN) {
    g_assets.Gulag();
  } else {
    g_assets.Honk();
  }
}

Goose::~Goose() {
    delete heldItem;
    heldItem = nullptr;
    BehaviorStateManager::Instance().RemoveForGoose(id);
#ifdef __APPLE__
    Goose_DestroyPerGooseWindow(this);
#endif
}

Vector2 Goose::GetBeakTipDevice() {
  Vector2 rawFwd = Vector2::FromAngleDegrees(dir);
  Vector2 fwd{rawFwd.x * ISO_SCALE.x, rawFwd.y * ISO_SCALE.y};

  Vector2 neckHeadDev = WorldCoord::RigNeckHead(*this).toVector2();
  float totalBeakOffset =
      WorldCoord::Scale(g_config.rig.beakBaseOffset + g_config.rig.beakLen);
  Vector2 beakTipDevice = neckHeadDev + fwd * totalBeakOffset;

  if (state == GooseState::SNATCH_CURSOR) {
    DebugLog("[BTDEV] g%d: dir=%.0f neck(%.0f,%.0f) btDev(%.0f,%.0f)\n", id,
            dir, neckHeadDev.x, neckHeadDev.y, beakTipDevice.x,
            beakTipDevice.y);
  }

  return beakTipDevice;
}

void Goose::UpdateRig() {
  Vector2 rawFwd = Vector2::FromAngleDegrees(dir);
  Vector2 fwd{rawFwd.x * ISO_SCALE.x, rawFwd.y * ISO_SCALE.y};
  Vector2 up{0, -1};

  float facing = Dot(Vector2::Normalize(fwd), Vector2{0, 1});
  float back = Clamp(-facing, 0.0f, 1.0f);

  rig.underbody = pos + up * g_config.rig.underbodyY;
  rig.body = pos + up * g_config.rig.bodyY;

  int targetState =
      (state == GooseState::WANDER)
          ? 0
          : ((currentSpeed >= g_config.rig.runSpeedThreshold) ? 1 : 0);
  rig.neckLerp =
      Lerp(rig.neckLerp, (float)targetState, g_config.rig.neckLerpRate);

  float neckH = Lerp(g_config.rig.neckHeightIdle, g_config.rig.neckHeightMoving,
                     rig.neckLerp);
  float neckExt =
      Lerp(g_config.rig.neckExtIdle, g_config.rig.neckExtMoving, rig.neckLerp);

  rig.neckBase = rig.body + fwd * g_config.rig.neckBaseX +
                 up * (g_config.rig.footOffsetY * back);
  rig.neckHead = rig.neckBase +
                 fwd * (neckExt + g_config.rig.headForwardBias * back) +
                 up * neckH;

  rig.head1 = rig.neckHead + fwd * g_config.rig.head1OffsetX;
  rig.head2 = rig.neckHead + fwd * g_config.rig.head2OffsetX;
}

Vector2 Goose::GetFootHome(float angleOffset) {
  float ang = dir + angleOffset;
  Vector2 raw = Vector2::FromAngleDegrees(ang);
  Vector2 side{raw.x * ISO_SCALE.x, raw.y * ISO_SCALE.y};
  return pos + side * WorldCoord::Scale(g_config.step.footSpacing);
}

void Goose::SolveFeet(double time) {
  Vector2 lHome = GetFootHome(g_config.step.leftFootAngle);
  Vector2 rHome = GetFootHome(g_config.step.rightFootAngle);

  if (rig.lFoot.currentPos.x == 0 && rig.lFoot.currentPos.y == 0) {
    rig.lFoot.currentPos = lHome;
    rig.rFoot.currentPos = rHome;
  }

  float speed = std::max(0.0f, currentSpeed);
  float denom = std::max(
      1.0f, (g_config.movement.baseRunSpeed - g_config.movement.baseWalkSpeed));
  float speed01 =
      Clamp((speed - g_config.movement.baseWalkSpeed) / denom, 0.0f, 1.0f);
  Gait gait{
      Lerp(g_config.step.stepTriggerWalk, g_config.step.stepTriggerRun, speed01),
      Lerp(g_config.step.overshootWalk, g_config.step.overshootRun, speed01),
      Lerp(g_config.step.durationWalk, g_config.step.durationRun, speed01),
      Lerp(g_config.step.liftWalk, g_config.step.liftRun, speed01),
  };

  StepFoot(rig.lFoot, lHome, gait, time);
  StepFoot(rig.rFoot, rHome, gait, time);
}

void Goose::StepFoot(FootState &f, Vector2 home, const Gait &gait, double time) {
  // NaN guard — recover corrupted foot positions
  if (!std::isfinite(f.currentPos.x) || !std::isfinite(f.currentPos.y)) {
    f.currentPos = home;
    f.moveStartTime = -1.0;
    return;
  }
  if (!std::isfinite(home.x) || !std::isfinite(home.y)) {
    home = pos;
  }

  if (f.moveStartTime < 0) {
    float dist = Vector2::Distance(f.currentPos, home);

    if (dist > g_config.step.snapDistance) {
      f.currentPos = home;
      f.moveStartTime = -1.0;
      return;
    }

    if (dist > gait.stepTrigger) {
      if (rig.lFoot.moveStartTime >= 0.0 || rig.rFoot.moveStartTime >= 0.0)
        return;

      f.moveOrigin = f.currentPos;
      f.moveDir = Vector2::Normalize(home - f.currentPos);
      f.moveStartTime = time;

      float distFactor =
          Clamp(dist / g_config.step.distFactorBase,
                g_config.step.distFactorMin, g_config.step.distFactorMax);
      f.moveDuration = Clamp(gait.baseDur * distFactor, g_config.step.durationMin,
                             g_config.step.durationMax);
    }
  } else {
    Vector2 target = home + f.moveDir * gait.overshoot;
    float p = (float)(time - f.moveStartTime) /
              std::max(g_config.step.minDuration, f.moveDuration);

    if (p >= 1.0f) {
      f.currentPos = home;
      f.moveStartTime = -1;

      if (mudEnabled && (rng_util::RandRange(100)) < mudChance) {
        Footprint fp;
        fp.pos = home; // already device coords (GetFootHome returns pos + offset)
        fp.dir = dir + ((&f == &rig.lFoot) ? g_config.step.leftFootAngle
                                           : g_config.step.rightFootAngle);
        fp.timeSpawned = time;
        fp.lifetime = mudLifetime;
        g_world.footprints.push(fp);
      }

      if (time - lastStepSoundTime > stepSoundCooldown) {
        g_assets.Pat();
        lastStepSoundTime = time;
      }
    } else {
      float e = CubicEaseInOut(p);
      Vector2 base = Vector2::Lerp(f.moveOrigin, target, e);
      float lift = std::sin((float)PI * p) * gait.liftAmt;
      f.currentPos = base + Vector2{0.0f, -lift};
    }
  }
}

void Goose::UpdateDirection() {
  if (state == GooseState::SNATCH_CURSOR) {
    Vector2 revDir = snatchFwd * g_config.physics.directionReverseMultiplier;
    dir = std::atan2(revDir.y, revDir.x) * kRadToDeg;
  } else if (Vector2::Length(vel) > g_config.physics.directionRotateMinVel) {
    Vector2 curDirVec = Vector2::FromAngleDegrees(dir);
    Vector2 targetDirVec = Vector2::Normalize(vel);

    if (state == GooseState::RETURNING) {
      targetDirVec = targetDirVec * g_config.physics.directionReverseMultiplier;
    }

    Vector2 blend = Vector2::Lerp(curDirVec, targetDirVec,
                                   g_config.movement.directionBlendRate);
    dir = std::atan2(blend.y, blend.x) * kRadToDeg;
  }
}

static float CalculateTargetSpeed(const Goose& g, float dist) {
  bool needsRun = (dist > g_config.movement.runDistanceThreshold ||
                   g.state == GooseState::FETCHING || g.state == GooseState::CHASE_CURSOR ||
                   g.state == GooseState::SNATCH_CURSOR || g.state == GooseState::RETURNING);
  float baseSpeed = needsRun ? g_config.movement.baseRunSpeed : g_config.movement.baseWalkSpeed;

  if (IsNighttime()) {
    baseSpeed *= kNighttimeSpeedFactor;
  }

  return g.isSurprised ? g_config.movement.baseRunSpeed * kSurprisedSpeedMultiplier : baseSpeed;
}

void Goose::UpdatePhysics(double dt, int w, int h) {
  if (isResting) {
    vel = {0, 0};
    currentSpeed = 0.0f;
    target = pos;
  } else {
    float dist = Vector2::Length(target - pos);
    float tSpeed = CalculateTargetSpeed(*this, dist);
    currentSpeed = Lerp(currentSpeed, tSpeed, g_config.movement.speedLerpRate);

    Vector2 steerForce = CalculateSeekForce();
    steerForce += CalculateCurveForce(dist);
    steerForce += CalculateSeparationForce();

    Vector2 avoidance = CalculateEdgeAvoidance(w, h);
    if (Vector2::Length(avoidance) > kEdgeAvoidMinForce) {
      steerForce += (avoidance - vel) * g_config.physics.edgeAvoidForce;
    }

    float steerMag = Vector2::Length(steerForce);
    if (steerMag > g_config.movement.maxForce) {
      steerForce = steerForce * (g_config.movement.maxForce / steerMag);
    }

    vel = vel + steerForce * (float)dt;

    float speed = Vector2::Length(vel);
    if (speed > currentSpeed && speed > kSpeedEpsilon) {
      vel = vel * (currentSpeed / speed);
    }
  }

  pos = pos + vel * (float)dt;
  ClampToScreen(w, h);
}

void Goose::UpdateDetection(double time, int w, int h) {
  // Shudder detection: track rapid direction changes without significant movement
  float distSinceLastShudderCheck = Vector2::Length(pos - shudderLastPos);
  float dirChange = std::abs(dir - shudderLastDir);
  if (dirChange > kShudderDirChangeThreshold) {
    shudderDirChanges++;
  }
  shudderLastPos = pos;
  shudderLastDir = dir;
  if (time - shudderCheckTime > SHUDDER_WINDOW) {
    if (shudderDirChanges >= SHUDDER_DIR_THRESHOLD && distSinceLastShudderCheck < SHUDDER_MOVE_THRESHOLD) {
      DebugLog("[SHUDDER] t=%.1f g%d pos(%.0f,%.0f) dir=%.0f dirChanges=%d distMoved=%.0f state=%d\n",
              time, id, pos.x, pos.y, dir, shudderDirChanges, distSinceLastShudderCheck, (int)state);
    }
    shudderDirChanges = 0;
    shudderCheckTime = time;
    shudderLastPos = pos;
  }

  // Stuck detection: only check during movement states, skip stationary modes
  auto* jailState = BehaviorStateManager::Instance().Get<JailState>(id, "jail");
  bool isJailed = (jailState && jailState->isJailed);
  bool isStationary = isResting || isJailed || state == GooseState::SNATCH_CURSOR;
  float distMoved = Vector2::Length(pos - stuckCheckPos);
  if (!isStationary && (state == GooseState::WANDER || state == GooseState::CHASE_CURSOR || state == GooseState::FETCHING || state == GooseState::RETURNING)) {
    if (distMoved > kStuckMinMovementThreshold) {
      // Normal movement, reset tracker
      stuckCheckPos = pos;
      stuckCheckTime = time;
    } else if (time - stuckCheckTime > kStuckThresholdTime) {
      // Goose is stuck, pick new wander target
      DebugLog("[STUCK] t=%.1f g%d pos(%.0f,%.0f) state=%d vel(%.1f,%.1f)\n",
              time, id, pos.x, pos.y, (int)state, vel.x, vel.y);
      EventBus::Instance().Publish(GooseStuckEvent{id, pos.x, pos.y, time - stuckCheckTime});
      target = Vector2((float)(rng_util::RandRange((int)w)), (float)(rng_util::RandRange((int)h)));
      stuckCheckPos = pos;
      stuckCheckTime = time;
    }
  } else {
    // Stationary mode or non-movement state, reset tracker
    stuckCheckPos = pos;
    stuckCheckTime = time;
  }
}

void Goose::UpdateAnimation(double dt, double time) {
  UpdateDirection();
  UpdateRig();
  SolveFeet(time);
  UpdateDrag(dt);
}

void Goose::UpdateDebug(double time, const CursorState& cursor) {
  if (debugSnatch && state == GooseState::SNATCH_CURSOR &&
      time - lastDebugLog > debugLogInterval) {
    lastDebugLog = time;
    Vector2 btDev = GetBeakTipDevice();
    DebugLog("[S%d] t=%.2f pos(%.1f,%.1f) dir=%.1f vel(%.1f,%.1f) spd=%.0f "
            "tgt(%.0f,%.0f) btDev(%.0f,%.0f) angle=%.2f fwd(%.2f,%.2f) "
            "anchor(%.0f,%.0f) radius=%.0f cursor(%.0f,%.0f) grabber=%d\n",
            id, time, pos.x, pos.y, dir, vel.x, vel.y, currentSpeed, target.x,
            target.y, btDev.x, btDev.y, snatchAngle, snatchFwd.x, snatchFwd.y,
            snatchAnchor.x, snatchAnchor.y, snatchRadius, cursor.position.x,
            cursor.position.y, g_world.cursorGrabberId);
  }
}

CursorAction Goose::Update(double dt, double time, int w, int h,
                           const CursorState &cursor) {
  lastUpdateTime = time;
  if (state != prevState) {
    static constexpr const char* stateNames[] = {"W", "F", "R", "C", "S"};
    constexpr int kStateCount = (int)(sizeof(stateNames)/sizeof(stateNames[0]));
    int pi = static_cast<int>(prevState);
    int ci = static_cast<int>(state);
    const char* prevSn = (pi >= 0 && pi < kStateCount) ? stateNames[pi] : "?";
    const char* curSn  = (ci >= 0 && ci < kStateCount) ? stateNames[ci] : "?";
    DebugLog("!! t=%.1f g%d %s->%s tgt(%.0f,%.0f) c(%.0f,%.0f)\n", time, id,
            prevSn, curSn, target.x, target.y, cursor.position.x,
            cursor.position.y);
    prevState = state;
    s_stateChanged = true;
  }

  s_lastLogTime += dt;
  if ((s_lastLogTime > kLogInterval || s_stateChanged) && g_config.debug.toTerminal) {
    LogTick(time, cursor);
    s_lastLogTime = 0;
    s_stateChanged = false;
  }

  CursorAction action = UpdateBehaviors(dt, time, w, h, cursor);

  UpdatePhysics(dt, w, h);
  UpdateDetection(time, w, h);
  UpdateAnimation(dt, time);
  UpdateDebug(time, cursor);

  return action;
}

void Goose::ForceFetch(FetchType type, int w, int h, double time) {
  DebugLog("[FF] g%d ForceFetch type=%d w=%d h=%d\n", id, type, w, h);
  forceItemFetch = type;
  StartFetch(w, h, time);
  DebugLog("[FF] after StartFetch state=%d heldItem=%p\n", (int)state, (void*)heldItem);
}

void Goose::ForceFetchText(const std::string &text, int w, int h) {
  forceItemFetch = FetchType::Text;
  forcedText = text;
  StartFetch(w, h);
}

void Goose::ForceWander(int w, int h) {
  state = GooseState::WANDER;
  delete heldItem;
  heldItem = nullptr;
  dragInit = false;
  forcedText.clear();
  PickNewTarget(w, h);
}

void Goose::StartFetch(int w, int h, double time) {
  state = GooseState::FETCHING;
  fetchStartTime = time; // allow 0 or -1 to suppress stale timeout

  int side = rng_util::RandRange(4);
  float edgeMargin = g_config.spawn.fetchEdgeMargin;
  switch (side) {
  case 0:
    target = {-edgeMargin, (float)(rng_util::RandRange(h))};
    break;
  case 1:
    target = {(float)w + edgeMargin, (float)(rng_util::RandRange(h))};
    break;
  case 2:
    target = {(float)(rng_util::RandRange(w)), -edgeMargin};
    break;
  case 3:
    target = {(float)(rng_util::RandRange(w)), (float)h + edgeMargin};
    break;
  }

  parabolicCurvature = ((rng_util::RandRange((int)kFetchCurvatureRange)) - (int)kFetchCurvatureCenter) / kFetchCurvatureDivisor;
}

void Goose::PickNewTarget(int w, int h) {
  if (state == GooseState::FETCHING || state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING) {
    return;
  }
  parabolicCurvature = 0;

  float margin = g_config.spawn.marginX;
  target = {(float)(rng_util::RandRange((int)(w - margin * 2)) + margin),
            (float)(rng_util::RandRange((int)(h - margin * 2)) + margin)};
}

void Goose::UpdateDrag(double dt) {
  if (!heldItem) {
      dragInit = false;
      return;
  }

  if (dt < g_config.physics.dragMinDt) return;

  if (!dragInit) {
    dragPos = GetBeakTipDevice();
    dragRot = dir * kDegToRad;
    dragVel = {0, 0};
    dragRotVel = 0.0f;
    dragInit = true;
    return;
  }

  Vector2 targetPos = GetBeakTipDevice();
  Vector2 diff = targetPos - dragPos;

  // Spring physics
  dragVel = dragVel + diff * (kSpringForce * (float)dt);
  dragVel = dragVel * kSpringDamping;
  dragPos = dragPos + dragVel * (float)dt;

  if (Vector2::Length(dragVel) > g_config.physics.dragVelocityThreshold) {
      float targetRot = std::atan2(dragVel.y, dragVel.x);
      
      float rotDiff = std::fmod(targetRot - dragRot + PI, kTwoPi);
      if (rotDiff < 0) rotDiff += kTwoPi;
      rotDiff -= PI;
      
      dragRot += rotDiff * g_config.physics.dragRotationSpeed * (float)dt;
  }
}

void Goose::tick(WorldContext& world, double dt, double time) {
    CursorState cursor = {};
    if (g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }

    CursorAction action = Update(dt, time, world.screenWidth, world.screenHeight, cursor);

    if (g_cursorProvider && !action.isNone()) {
        g_cursorProvider->Execute(action);
    }

    BehaviorContext ctx;
    ctx.goose = this;
    ctx.time = time;
    ctx.isJailed = false;
    ctx.world = &world;
    BehaviorRegistry::Instance().TickAll(this, dt, time);
}

#ifdef __APPLE__
void Goose::drawBody(CGContextRef ctx) {
    DrawGoose(this, ctx);
}
#endif

void Goose::render(IRenderer* renderer) {
    draw(renderer);
}

void Goose::draw(IRenderer* renderer) {
#ifdef __APPLE__
    if (!renderer) return;
    CGContextRef ctx = (CGContextRef)renderer->nativeContext();
    if (!ctx) return;

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, pos.x, pos.y);
    CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
    CGContextTranslateCTM(ctx, -pos.x, -pos.y);
    BehaviorRegistry::Instance().RenderPass(this, renderer, true);
    CGContextRestoreGState(ctx);

    drawBody(ctx);

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, pos.x, pos.y);
    CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
    CGContextTranslateCTM(ctx, -pos.x, -pos.y);
    BehaviorRegistry::Instance().RenderPass(this, renderer, false);
    CGContextRestoreGState(ctx);
#else
    (void)renderer;
#endif
}