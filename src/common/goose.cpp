#include "goose.h"
#include "assets.h"
#include "config.h"
#include "goose_math.h"
#include "world.h"
#include <cmath>
#include <cstdio>

static FILE *s_debugLog = nullptr;

static FILE *GetDebugLog() {
  if (!s_debugLog) {
    s_debugLog = fopen("/tmp/goose_debug.log", "w");
    if (!s_debugLog)
      s_debugLog = stderr;
  }
  return s_debugLog;
}

static void LogTick(double time, const CursorState &cursor) {
  FILE *f = GetDebugLog();
  if (!f)
    return;
  fprintf(f, "[T%.1f] cur=%d", time, g_cursorGrabberId);
  if (cursor.hasPos())
    fprintf(f, " c(%.0f,%.0f)", cursor.position.x, cursor.position.y);
  else
    fprintf(f, " c(-,-)");
  fprintf(f, " geese:");
  for (auto &g : g_geese) {
    const char* stateNames[] = {"W", "F", "R", "C", "S"};
    fprintf(f, " %d%s@(%d,%d)d%dv(%d,%d)s%d", g.id, stateNames[static_cast<int>(g.state)], (int)g.pos.x,
            (int)g.pos.y, (int)g.dir, (int)g.vel.x, (int)g.vel.y,
            (int)g.currentSpeed);
    if (g.state == GooseState::SNATCH_CURSOR)
      fprintf(f, " a=%.1f r=%.0f", g.snatchAngle, g.snatchRadius);
  }
  fprintf(f, "\n");
}

static bool s_stateChanged = true;
static double s_lastLogTime = 0;

static void CloseDebugLog() { s_debugLog = nullptr; }

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

  FILE *f = GetDebugLog();
  if (f && state == GooseState::SNATCH_CURSOR) {
    fprintf(f, "[BTDEV] g%d: dir=%.0f neck(%.0f,%.0f) btDev(%.0f,%.0f)\n", id,
            dir, neckHeadDev.x, neckHeadDev.y, beakTipDevice.x,
            beakTipDevice.y);
    fflush(f);
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
  return pos + side * g_config.step.footSpacing;
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
          fp.pos = WorldCoord::ToDevice(home, *this);
          fp.dir = dir + ((&f == &rig.lFoot) ? g_config.step.leftFootAngle
                                             : g_config.step.rightFootAngle);
          fp.timeSpawned = time;
          fp.lifetime = mudLifetime;
          g_footprints.push_back(fp);
          g_assets.MudSquish();
        } else if (time - lastStepSoundTime > stepSoundCooldown) {
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
    dir = std::atan2(revDir.y, revDir.x) * (float)(180.0 / PI);
  } else if (Vector2::Length(vel) > g_config.physics.directionRotateMinVel) {
    Vector2 curDirVec = Vector2::FromAngleDegrees(dir);
    Vector2 targetDirVec = Vector2::Normalize(vel);

    if (state == GooseState::RETURNING) {
      targetDirVec = targetDirVec * g_config.physics.directionReverseMultiplier;
    }

    Vector2 blend = Vector2::Lerp(curDirVec, targetDirVec,
                                   g_config.movement.directionBlendRate);
    dir = std::atan2(blend.y, blend.x) * (float)(180.0 / PI);
  }
}

CursorAction Goose::Update(double dt, double time, int w, int h,
                           const CursorState &cursor) {
  if (state != prevState) {
    FILE *f = GetDebugLog();
    const char *stateNames[] = {"W", "F", "R", "C", "S"};
    fprintf(f, "!! t=%.1f g%d %s->%s tgt(%.0f,%.0f) c(%.0f,%.0f)\n", time, id,
            stateNames[static_cast<int>(prevState)], stateNames[static_cast<int>(state)], target.x, target.y, cursor.position.x,
            cursor.position.y);
    prevState = state;
    s_stateChanged = true;
  }

  s_lastLogTime += dt;
  if (s_lastLogTime > 0.1 || s_stateChanged) {
    LogTick(time, cursor);
    s_lastLogTime = 0;
    s_stateChanged = false;
  }

  UpdateRig();
  CursorAction action = UpdateBehaviors(dt, time, w, h, cursor);

  float dist = Vector2::Length(target - pos);
  float tSpeed = (dist > g_config.movement.runDistanceThreshold ||
                  state == GooseState::FETCHING || state == GooseState::CHASE_CURSOR ||
                  state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING)
                     ? g_config.movement.baseRunSpeed
                     : g_config.movement.baseWalkSpeed;
  currentSpeed = Lerp(currentSpeed, tSpeed, g_config.movement.speedLerpRate);

  Vector2 steerForce = CalculateSeekForce();
  steerForce += CalculateCurveForce(dist);
  steerForce += CalculateSeparationForce();

  Vector2 avoidance = CalculateEdgeAvoidance(w, h);
  if (Vector2::Length(avoidance) > 0.1f) {
    steerForce += (avoidance - vel) * g_config.physics.edgeAvoidForce;
  }

  float steerMag = Vector2::Length(steerForce);
  if (steerMag > g_config.movement.maxForce) {
    steerForce = steerForce * (g_config.movement.maxForce / steerMag);
  }

  vel = vel + steerForce * (float)dt;

  float speed = Vector2::Length(vel);
  if (speed > currentSpeed && speed > 1e-4f) {
    vel = vel * (currentSpeed / speed);
  }

  pos = pos + vel * (float)dt;
  ClampToScreen(w, h);
  UpdateDirection();

  if (debugSnatch && state == GooseState::SNATCH_CURSOR &&
      time - lastDebugLog > debugLogInterval) {
    lastDebugLog = time;
    Vector2 btDev = GetBeakTipDevice();
    FILE *f = GetDebugLog();
    if (f) {
      fprintf(f,
              "[S%d] t=%.2f pos(%.1f,%.1f) dir=%.1f vel(%.1f,%.1f) spd=%.0f "
              "tgt(%.0f,%.0f) btDev(%.0f,%.0f) angle=%.2f fwd(%.2f,%.2f) "
              "anchor(%.0f,%.0f) radius=%.0f cursor(%.0f,%.0f) grabber=%d\n",
              id, time, pos.x, pos.y, dir, vel.x, vel.y, currentSpeed, target.x,
              target.y, btDev.x, btDev.y, snatchAngle, snatchFwd.x, snatchFwd.y,
              snatchAnchor.x, snatchAnchor.y, snatchRadius, cursor.position.x,
              cursor.position.y, g_cursorGrabberId);
      fflush(f);
    }
  }

  SolveFeet(time);
  UpdateDrag(dt);

  return action;
}

void Goose::ForceFetch(int type, int w, int h) {
  fprintf(stderr, "[FF] g%d ForceFetch type=%d w=%d h=%d\n", id, type, w, h);
  forceItemFetch = type;
  StartFetch(w, h);
  fprintf(stderr, "[FF] after StartFetch state=%d heldItem=%p\n", (int)state, (void*)heldItem);
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

void Goose::StartFetch(int w, int h) {
  state = GooseState::FETCHING;

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

  parabolicCurvature = ((rand() % 200) - 100) / 100.0f;
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
    dragRot = dir * ((float)PI / 180.0f);
    dragVel = {0, 0};
    dragRotVel = 0.0f;
    dragInit = true;
    return;
  }

  Vector2 targetPos = GetBeakTipDevice();
  Vector2 diff = targetPos - dragPos;

  // Spring physics
  float springForce = 150.0f;
  float damping = 0.85f;

  dragVel = dragVel + diff * (springForce * (float)dt);
  dragVel = dragVel * damping;
  dragPos = dragPos + dragVel * (float)dt;

  if (Vector2::Length(dragVel) > g_config.physics.dragVelocityThreshold) {
      float targetRot = std::atan2(dragVel.y, dragVel.x);
      
      float rotDiff = targetRot - dragRot;
      while (rotDiff > PI) rotDiff -= 2 * PI;
      while (rotDiff < -PI) rotDiff += 2 * PI;
      
      dragRot += rotDiff * g_config.physics.dragRotationSpeed * (float)dt;
  }
}