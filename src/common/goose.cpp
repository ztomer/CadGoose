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
  const char *ss[] = {"W", "F", "R", "C", "S"};
  fprintf(f, "[T%.1f] cur=%d", time, g_cursorGrabberId);
  if (cursor.hasPos())
    fprintf(f, " c(%.0f,%.0f)", cursor.position.x, cursor.position.y);
  else
    fprintf(f, " c(-,-)");
  fprintf(f, " geese:");
  for (auto &g : g_geese) {
    fprintf(f, " %d%s@(%d,%d)d%dv(%d,%d)s%d", g.id, ss[g.state], (int)g.pos.x,
            (int)g.pos.y, (int)g.dir, (int)g.vel.x, (int)g.vel.y,
            (int)g.currentSpeed);
    if (g.state == 4)
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

  attackMouseBias = 0; // start with no attack bias
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

// BEHAVIOR.md line 387-392: Beak calculation
// beakBase = neckHead + fwd * BEAK_BASE_OFFSET (4.0f)
// beakTip = beakBase + fwd * BEAK_LEN (12.0f)
Vector2 Goose::GetBeakTipDevice() {
  Vector2 rawFwd = Vector2::FromAngleDegrees(dir);
  Vector2 fwd{rawFwd.x * ISO_SCALE.x, rawFwd.y * ISO_SCALE.y};

  Vector2 neckHeadDev = WorldCoord::RigNeckHead(*this);
  float totalBeakOffset =
      WorldCoord::Scale(g_config.rig.beakBaseOffset + g_config.rig.beakLen);
  Vector2 beakTipDevice = neckHeadDev + fwd * totalBeakOffset;

  FILE *f = GetDebugLog();
  if (f && state == SNATCH_CURSOR) {
    fprintf(f, "[BTDEV] g%d: dir=%.0f neck(%.0f,%.0f) btDev(%.0f,%.0f)\n", id,
            dir, neckHeadDev.x, neckHeadDev.y, beakTipDevice.x,
            beakTipDevice.y);
    fflush(f);
  }

  return beakTipDevice;
}

void Goose::UpdateRig() {
  // All rig positions are in WORLD coordinates (relative to pos)
  // Consumers requiring DEVICE coordinates apply WorldToDevice() transform
  Vector2 rawFwd = Vector2::FromAngleDegrees(dir);
  Vector2 fwd{rawFwd.x * ISO_SCALE.x, rawFwd.y * ISO_SCALE.y};
  Vector2 up{0, -1};

  float facing = Dot(Vector2::Normalize(fwd), Vector2{0, 1});
  float back = Clamp(-facing, 0.0f, 1.0f);

  rig.underbody = pos + up * g_config.rig.underbodyY;
  rig.body = pos + up * g_config.rig.bodyY;

  int targetState =
      (state == WANDER)
          ? 0
          : ((currentSpeed >= g_config.rig.runSpeedThreshold) ? 1 : 0);
  // fprintf(stderr, "Goose %d state %d currentSpeed %.1f targetState %d\n", id,
  //         (int)state, currentSpeed, targetState);
  // fflush(stderr);
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
          // Convert world coordinates to device coordinates using WorldCoord
          // utility
          fp.pos = WorldCoord::ToDevice(home, *this);
          fp.dir = dir + ((&f == &rig.lFoot) ? g_config.step.leftFootAngle
                                             : g_config.step.rightFootAngle);
          fp.timeSpawned = time;
          fp.lifetime = mudLifetime;
          // fprintf(stderr,
          //         "Footprint created at device (%.1f, %.1f), foot home world
          //         "
          //         "(%.1f, "
          //         "%.1f), goose pos (%.1f, %.1f), globalScale %.3f\n",
          //         fp.pos.x, fp.pos.y, home.x, home.y, pos.x, pos.y,
          //         g_config.general.globalScale);
          // fflush(stderr);
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

CursorAction Goose::Update(double dt, double time, int w, int h,
                           const CursorState &cursor) {
  if (state != prevState) {
    FILE *f = GetDebugLog();
    const char *ss[] = {"W", "F", "R", "C", "S"};
    fprintf(f, "!! t=%.1f g%d %s->%s tgt(%.0f,%.0f) c(%.0f,%.0f)\n", time, id,
            ss[prevState], ss[state], target.x, target.y, cursor.position.x,
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

  // Update rig BEFORE behaviors so beak tip calculation is correct
  UpdateRig();

  CursorAction action = UpdateBehaviors(dt, time, w, h, cursor);

  // Physics uses world coordinates - target is now in world coordinates
  Vector2 toTarget = target - pos;
  float dist = Vector2::Length(toTarget);

  float tSpeed =
      (dist > g_config.movement.runDistanceThreshold || state == FETCHING ||
       state == CHASE_CURSOR || state == SNATCH_CURSOR || state == RETURNING)
          ? g_config.movement.baseRunSpeed
          : g_config.movement.baseWalkSpeed;
  currentSpeed = Lerp(currentSpeed, tSpeed, g_config.movement.speedLerpRate);

  float arrivalRadius = g_config.movement.arrivalRadius;
  Vector2 moveDir = (dist > 0.01f) ? toTarget / dist : Vector2{0, 1};
  Vector2 desiredVel = moveDir * currentSpeed;
  if (dist < arrivalRadius) {
    desiredVel = desiredVel * (dist / arrivalRadius);
  }

  Vector2 steerForce = (desiredVel - vel) * g_config.physics.steerSeekForce;

  float curveFade = std::min(1.0f, dist / g_config.physics.curveFadeDistance);
  if (Vector2::Length(vel) > g_config.physics.curveFadeMinVel) {
    Vector2 normVel = Vector2::Normalize(vel);
    Vector2 tangent = {-normVel.y, normVel.x};
    steerForce += tangent * (parabolicCurvature * currentSpeed *
                             g_config.physics.curveTangentForce * curveFade);
  }

  if (state == WANDER || state == FETCHING) {
    for (auto &other : g_geese) {
      if (other.id == id)
        continue;
      float d = Vector2::Distance(pos, other.pos);
      if (d > g_config.spawn.separationMinDistance &&
          d < g_config.spawn.separationMaxDistance) {
        float strength = (g_config.spawn.separationMaxDistance - d) /
                         g_config.spawn.separationMaxDistance;
        Vector2 away = Vector2::Normalize(pos - other.pos);
        steerForce += away * (strength * g_config.movement.maxForce *
                              g_config.spawn.separationForceMultiplier);
      }
    }
  }

  if (state != FETCHING) {
    float lookAhead = currentSpeed * g_config.physics.edgeLookAheadSpeed +
                      g_config.physics.edgeLookAheadBase;
    Vector2 probePos = pos + Vector2::Normalize(vel) * lookAhead;

    float margin = g_config.physics.edgeAvoidMargin;
    Vector2 avoidance{0, 0};

    float bMinX = 0, bMinY = 0, bMaxX = (float)w, bMaxY = (float)h;
    if (!g_config.cursor.multiMonitorEnabled && !g_monitors.empty()) {
      for (auto &m : g_monitors) {
        if (m.x == 0 && m.y == 0) {
          bMaxX = (float)m.width;
          bMaxY = (float)m.height;
          break;
        }
      }
    }

    if (probePos.x < bMinX + margin)
      avoidance.x = currentSpeed;
    else if (probePos.x > bMaxX - margin)
      avoidance.x = -currentSpeed;
    if (probePos.y < bMinY + margin)
      avoidance.y = currentSpeed;
    else if (probePos.y > bMaxY - margin)
      avoidance.y = -currentSpeed;

    if (Vector2::Length(avoidance) > 0.1f) {
      steerForce += (avoidance - vel) * g_config.physics.edgeAvoidForce;
    }
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

  float minX = 0.0f, minY = 0.0f, maxX = (float)w, maxY = (float)h;
  if (!g_config.cursor.multiMonitorEnabled && !g_monitors.empty()) {
    for (auto &m : g_monitors) {
      if (m.x == 0 && m.y == 0) {
        maxX = (float)m.width;
        maxY = (float)m.height;
        break;
      }
    }
  }

  if (state == FETCHING) {
    minX -= g_config.physics.screenClampExpanded;
    maxX += g_config.physics.screenClampExpanded;
    minY -= g_config.physics.screenClampExpanded;
    maxY += g_config.physics.screenClampExpanded;
  } else {
    minX += g_config.physics.screenClampTight;
    maxX -= g_config.physics.screenClampTight;
    minY += g_config.physics.screenClampTight;
    maxY -= g_config.physics.screenClampTight;
  }

  if (pos.x < minX) {
    pos.x = minX + g_config.physics.snapDistance;
    if ((state == SNATCH_CURSOR || state == RETURNING || state == FETCHING) &&
        vel.x < 0) {
      vel.x = std::abs(vel.x) + g_config.physics.screenClampBounce;
    }
  } else if (pos.x > maxX) {
    pos.x = maxX - g_config.physics.snapDistance;
    if ((state == SNATCH_CURSOR || state == RETURNING || state == FETCHING) &&
        vel.x > 0) {
      vel.x = -std::abs(vel.x) - g_config.physics.screenClampBounce;
    }
  }

  if (pos.y < minY) {
    pos.y = minY + g_config.physics.snapDistance;
    if ((state == SNATCH_CURSOR || state == RETURNING || state == FETCHING) &&
        vel.y < 0) {
      vel.y = std::abs(vel.y) + g_config.physics.screenClampBounce;
    }
  } else if (pos.y > maxY) {
    pos.y = maxY - g_config.physics.snapDistance;
    if ((state == SNATCH_CURSOR || state == RETURNING || state == FETCHING) &&
        vel.y > 0) {
      vel.y = -std::abs(vel.y) - g_config.physics.screenClampBounce;
    }
  }

  if (state == SNATCH_CURSOR) {
    Vector2 revDir = snatchFwd * g_config.physics.directionReverseMultiplier;
    dir = std::atan2(revDir.y, revDir.x) * (float)(180.0 / PI);
  } else if (Vector2::Length(vel) > g_config.physics.directionRotateMinVel) {
    Vector2 curDirVec = Vector2::FromAngleDegrees(dir);
    Vector2 targetDirVec = Vector2::Normalize(vel);

    if (state == RETURNING) {
      targetDirVec = targetDirVec * g_config.physics.directionReverseMultiplier;
    }

    Vector2 blend = Vector2::Lerp(curDirVec, targetDirVec,
                                  g_config.movement.directionBlendRate);
    dir = std::atan2(blend.y, blend.x) * (float)(180.0 / PI);
  }

  if (debugSnatch && state == SNATCH_CURSOR &&
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
  forceItemFetch = type;
  StartFetch(w, h);
}

void Goose::ForceFetchText(const std::string &text, int w, int h) {
  forceItemFetch = 1;
  forcedText = text;
  StartFetch(w, h);
}

void Goose::ForceWander(int w, int h) {
  state = WANDER;
  heldItem = nullptr;
  dragInit = false;
  forcedText.clear();
  PickNewTarget(w, h);
}

void Goose::StartFetch(int w, int h) {
  state = FETCHING;

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
  if (state == FETCHING || state == SNATCH_CURSOR || state == RETURNING) {
    return;
  }
  parabolicCurvature = 0;

  float margin = g_config.spawn.marginX;
  target = {(float)(rand() % (int)(w - margin * 2) + margin),
            (float)(rand() % (int)(h - margin * 2) + margin)};
}

void Goose::UpdateDrag(double dt) {
  if (!heldItem || dragInit)
    return;

  dragPos = GetBeakTipDevice();
  dragRot = dir;
  dragInit = true;
}
