// goose_rig.cpp
// Goose rig/feet kinematics: rig pose update, foot-home placement and the
// procedural stepping solver. Split out of goose.cpp along its seams.

#include <cmath>

#include "goose.h"
#include "goose_math.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "random_util.h"

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
