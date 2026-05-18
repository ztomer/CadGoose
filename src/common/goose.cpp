#include <cstdarg>
#include "goose.h"
#include "assets.h"
#include "behavior.h"
#include "config.h"
#include "goose_math.h"
#include "world.h"
#include "cursor_backend.h"
#include <cmath>
#include <cstdio>

#ifdef __APPLE__
#include "goose_drawing.h"
#include "cg_renderer.h"
#include <CoreGraphics/CoreGraphics.h>
#endif

// --- Magic numbers extracted as named constants ---
static constexpr const char* kDebugLogPath = "/tmp/goose_debug.log";
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
    static time_t s_lastCheck = 0;
    static bool s_isNight = false;
    time_t now = std::time(nullptr);
    if (now - s_lastCheck >= kNighttimeCheckIntervalSec) {
        struct tm* tm_info = std::localtime(&now);
        if (tm_info) {
            s_isNight = (tm_info->tm_hour >= kNighttimeStartHour || tm_info->tm_hour < kNighttimeEndHour);
        }
        s_lastCheck = now;
    }
    return s_isNight;
}
static constexpr float kFetchCurvatureRange = 200.0f;
static constexpr float kFetchCurvatureCenter = 100.0f;
static constexpr float kFetchCurvatureDivisor = 100.0f;
static constexpr float kTwoPi = 2.0f * PI;
static constexpr float kRadToDeg = 180.0f / PI;
static constexpr float kDegToRad = PI / 180.0f;
static constexpr double kStuckThresholdTime = 3.0;
static constexpr float kStuckMinMovementThreshold = 10.0f;
static constexpr float kStuckRecoveryMargin = 50.0f;

static FILE *s_debugLog = nullptr;

static FILE *GetDebugLog() {
  if (!s_debugLog) {
    s_debugLog = fopen(kDebugLogPath, "w");
    if (!s_debugLog)
      s_debugLog = stderr;
  }
  return s_debugLog;
}

static void DebugLog(const char* fmt, ...) {
    FILE *f = GetDebugLog();
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fflush(f);
}

static void LogTick(double time, const CursorState &cursor) {
  FILE *f = GetDebugLog();
  if (!f)
    return;
  fprintf(f, "[T%.1f] cur=%d", time, g_world.cursorGrabberId);
  if (cursor.hasPos())
    fprintf(f, " c(%.0f,%.0f)", cursor.position.x, cursor.position.y);
  else
    fprintf(f, " c(-,-)");
  fprintf(f, " geese:");
  for (auto* g : ActorManager::Instance().getGeese()) {
    const char* stateNames[] = {"W", "F", "R", "C", "S"};
    fprintf(f, " %d%s@(%d,%d)d%dv(%d,%d)s%d", g->id, stateNames[static_cast<int>(g->state)], (int)g->pos.x,
            (int)g->pos.y, (int)g->dir, (int)g->vel.x, (int)g->vel.y,
            (int)g->currentSpeed);
    if (g->state == GooseState::SNATCH_CURSOR)
      fprintf(f, " a=%.1f r=%.0f", g->snatchAngle, g->snatchRadius);
  }
  fprintf(f, "\n");
}

static bool s_stateChanged = true;
static double s_lastLogTime = 0;

static void CloseDebugLog() {
    if (s_debugLog && s_debugLog != stderr) {
        fclose(s_debugLog);
    }
    s_debugLog = nullptr;
}

Goose::Goose(int id_, const std::string &name_, int screenW, int screenH)
    : id(id_), name(name_) {
  pos = {(float)(rand() % (int)(screenW - g_config.spawn.marginX * 2) +
                 g_config.spawn.marginX),
         (float)(rand() % (int)(screenH - g_config.spawn.marginY * 2) +
                 g_config.spawn.marginY)};
  target = pos;
  dir = (float)(rand() % (int)g_config.movement.initDirectionMax);
  currentSpeed = g_config.movement.baseWalkSpeed;

  attackMouseBias = 0;
  memeFetchBias = rand() % g_config.item.memeFetchBiasMax;
  noteFetchBias = rand() % g_config.item.noteFetchBiasMax;

  cursorChaseEnabled = g_config.cursor.chaseEnabled;
  cursorChaseChance = 5;
  snatchDuration = g_config.snatch.duration;
  mudEnabled = g_config.mud.enabled;
  mudChance = g_config.mud.chance;
  mudLifetime = g_config.mud.lifetime;
  ISO_SCALE = {g_config.physics.isoScaleX, g_config.physics.isoScaleY};
  rig.lFoot.currentPos = {0, 0};
  rig.rFoot.currentPos = {0, 0};
  rig.lFoot.moveStartTime = -1.0;
  rig.rFoot.moveStartTime = -1.0;
  PickNewTarget(screenW, screenH);
}

Vector2 Goose::GetBeakTipDevice() {
  Vector2 rawFwd = Vector2::FromAngleDegrees(dir);
  Vector2 fwd{rawFwd.x * ISO_SCALE.x, rawFwd.y * ISO_SCALE.y};

  Vector2 neckHeadDev = WorldCoord::RigNeckHead(*this);
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
  float stepTrigger = Lerp(g_config.step.stepTriggerWalk,
                           g_config.step.stepTriggerRun, speed01);
  float overshoot =
      Lerp(g_config.step.overshootWalk, g_config.step.overshootRun, speed01);
  float baseDur =
      Lerp(g_config.step.durationWalk, g_config.step.durationRun, speed01);
  float liftAmt = Lerp(g_config.step.liftWalk, g_config.step.liftRun, speed01);

  auto UpdateFoot = [&](FootState &f, Vector2 home) {
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

      if (dist > stepTrigger) {
        if (rig.lFoot.moveStartTime >= 0.0 || rig.rFoot.moveStartTime >= 0.0)
          return;

        f.moveOrigin = f.currentPos;
        f.moveDir = Vector2::Normalize(home - f.currentPos);
        f.moveStartTime = time;

        float distFactor =
            Clamp(dist / g_config.step.distFactorBase,
                  g_config.step.distFactorMin, g_config.step.distFactorMax);
        f.moveDuration = Clamp(baseDur * distFactor, g_config.step.durationMin,
                               g_config.step.durationMax);
      }
    } else {
      Vector2 target = home + f.moveDir * overshoot;
      float p = (float)(time - f.moveStartTime) /
                std::max(g_config.step.minDuration, f.moveDuration);

      if (p >= 1.0f) {
        f.currentPos = home;
        f.moveStartTime = -1;

        if (mudEnabled && (rand() % 100) < mudChance) {
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
        float lift = std::sin((float)PI * p) * liftAmt;
        f.currentPos = base + Vector2{0.0f, -lift};
      }
    }
  };

  UpdateFoot(rig.lFoot, lHome);
  UpdateFoot(rig.rFoot, rHome);
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
      target = Vector2((float)(rand() % (int)w), (float)(rand() % (int)h));
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
    const char *stateNames[] = {"W", "F", "R", "C", "S"};
    DebugLog("!! t=%.1f g%d %s->%s tgt(%.0f,%.0f) c(%.0f,%.0f)\n", time, id,
            stateNames[static_cast<int>(prevState)], stateNames[static_cast<int>(state)], target.x, target.y, cursor.position.x,
            cursor.position.y);
    prevState = state;
    s_stateChanged = true;
  }

  s_lastLogTime += dt;
  if (s_lastLogTime > kLogInterval || s_stateChanged) {
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

void Goose::ForceFetch(int type, int w, int h, double time) {
  DebugLog("[FF] g%d ForceFetch type=%d w=%d h=%d\n", id, type, w, h);
  forceItemFetch = type;
  StartFetch(w, h, time);
  DebugLog("[FF] after StartFetch state=%d heldItem=%p\n", (int)state, (void*)heldItem);
}

void Goose::ForceFetchText(const std::string &text, int w, int h) {
  forceItemFetch = 1;
  forcedText = text;
  StartFetch(w, h);
}

void Goose::ForceWander(int w, int h) {
  state = GooseState::WANDER;
  heldItem = nullptr;
  dragInit = false;
  forcedText.clear();
  PickNewTarget(w, h);
}

void Goose::StartFetch(int w, int h, double time) {
  state = GooseState::FETCHING;
  if (time > 0) fetchStartTime = time;

  int side = rand() % 4;
  float edgeMargin = g_config.spawn.fetchEdgeMargin;
  switch (side) {
  case 0:
    target = {-edgeMargin, (float)(rand() % h)};
    break;
  case 1:
    target = {(float)w + edgeMargin, (float)(rand() % h)};
    break;
  case 2:
    target = {(float)(rand() % w), -edgeMargin};
    break;
  case 3:
    target = {(float)(rand() % w), (float)h + edgeMargin};
    break;
  }

  parabolicCurvature = ((rand() % (int)kFetchCurvatureRange) - (int)kFetchCurvatureCenter) / kFetchCurvatureDivisor;
}

void Goose::PickNewTarget(int w, int h) {
  if (state == GooseState::FETCHING || state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING) {
    return;
  }
  parabolicCurvature = 0;

  float margin = g_config.spawn.marginX;
  target = {(float)(rand() % (int)(w - margin * 2) + margin),
            (float)(rand() % (int)(h - margin * 2) + margin)};
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
      
      float rotDiff = targetRot - dragRot;
      while (rotDiff > PI) rotDiff -= kTwoPi;
      while (rotDiff < -PI) rotDiff += kTwoPi;
      
      dragRot += rotDiff * g_config.physics.dragRotationSpeed * (float)dt;
  }
}

void Goose::tick(double dt, double time) {
    CursorState cursor = {};
    if (g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }

    CursorAction action = Update(dt, time, g_world.screenWidth, g_world.screenHeight, cursor);

    if (g_cursorProvider && !action.isNone()) {
        g_cursorProvider->Execute(action);
    }

    BehaviorContext ctx;
    ctx.goose = this;
    ctx.time = time;
    ctx.isJailed = false;
    BehaviorRegistry::Instance().TickAll(this, dt, time);
}

void Goose::render(IRenderer* renderer) {
#ifdef __APPLE__
    CGContextRef ctx = (CGContextRef)renderer->nativeContext();
    if (!ctx) return;

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, pos.x, pos.y);
    CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
    CGContextTranslateCTM(ctx, -pos.x, -pos.y);
    BehaviorRegistry::Instance().RenderPass(this, ctx, true);
    CGContextRestoreGState(ctx);

    DrawGoose(this, ctx);
    if (heldItem) {
        DrawHeldItem(this, ctx);
    }

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, pos.x, pos.y);
    CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
    CGContextTranslateCTM(ctx, -pos.x, -pos.y);
    BehaviorRegistry::Instance().RenderPass(this, ctx, false);
    CGContextRestoreGState(ctx);
#else
    (void)renderer;
#endif
}