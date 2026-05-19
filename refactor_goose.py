import re

# 1. Update goose.h
with open('include/goose.h', 'r') as f:
    goose_h = f.read()

# Add method declarations to goose.h
add_methods = """    void UpdatePhysics(double dt, int w, int h);
    void UpdateDetection(double time, int w, int h);
    void UpdateAnimation(double dt, double time);
    void UpdateDebug(double time, const CursorState& cursor);
"""
goose_h = goose_h.replace('    void UpdateChaseCursor(double time, const Vector2& cursorPos);', '    void UpdateChaseCursor(double time, const Vector2& cursorPos);\n' + add_methods)

with open('include/goose.h', 'w') as f:
    f.write(goose_h)


# 2. Update goose.cpp
with open('src/common/goose.cpp', 'r') as f:
    goose_cpp = f.read()

goose_cpp = '#include <cstdarg>\n' + goose_cpp

# Extract DebugLog helper
debug_log_helper = """static void DebugLog(const char* fmt, ...) {
    FILE *f = GetDebugLog();
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fflush(f);
}
"""

goose_cpp = goose_cpp.replace('static FILE *GetDebugLog() {', debug_log_helper + '\nstatic FILE *GetDebugLog() {')

# Replace GetBeakTipDevice logging
bt_old = """  FILE *f = GetDebugLog();
  if (f && state == GooseState::SNATCH_CURSOR) {
    fprintf(f, "[BTDEV] g%d: dir=%.0f neck(%.0f,%.0f) btDev(%.0f,%.0f)\\n", id,
            dir, neckHeadDev.x, neckHeadDev.y, beakTipDevice.x,
            beakTipDevice.y);
    fflush(f);
  }"""
bt_new = """  if (state == GooseState::SNATCH_CURSOR) {
    DebugLog("[BTDEV] g%d: dir=%.0f neck(%.0f,%.0f) btDev(%.0f,%.0f)\\n", id,
            dir, neckHeadDev.x, neckHeadDev.y, beakTipDevice.x,
            beakTipDevice.y);
  }"""
goose_cpp = goose_cpp.replace(bt_old, bt_new)

# ForceFetch logging
goose_cpp = goose_cpp.replace('fprintf(stderr, "[FF] g%d ForceFetch type=%d w=%d h=%d\\n", id, type, w, h);', 'DebugLog("[FF] g%d ForceFetch type=%d w=%d h=%d\\n", id, type, w, h);')
goose_cpp = goose_cpp.replace('fprintf(stderr, "[FF] after StartFetch state=%d heldItem=%p\\n", (int)state, (void*)heldItem);', 'DebugLog("[FF] after StartFetch state=%d heldItem=%p\\n", (int)state, (void*)heldItem);')

# Re-write Update and insert new methods
update_old = """CursorAction Goose::Update(double dt, double time, int w, int h,
                           const CursorState &cursor) {
  lastUpdateTime = time;
  if (state != prevState) {
    FILE *f = GetDebugLog();
    const char *stateNames[] = {"W", "F", "R", "C", "S"};
    fprintf(f, "!! t=%.1f g%d %s->%s tgt(%.0f,%.0f) c(%.0f,%.0f)\\n", time, id,
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
      FILE *f = GetDebugLog();
      fprintf(f, "[SHUDDER] t=%.1f g%d pos(%.0f,%.0f) dir=%.0f dirChanges=%d distMoved=%.0f state=%d\\n",
              time, id, pos.x, pos.y, dir, shudderDirChanges, distSinceLastShudderCheck, (int)state);
      fflush(f);
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
      FILE *f = GetDebugLog();
      fprintf(f, "[STUCK] t=%.1f g%d pos(%.0f,%.0f) state=%d vel(%.1f,%.1f)\\n",
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

  UpdateDirection();
  UpdateRig();

  if (debugSnatch && state == GooseState::SNATCH_CURSOR &&
      time - lastDebugLog > debugLogInterval) {
    lastDebugLog = time;
    Vector2 btDev = GetBeakTipDevice();
    FILE *f = GetDebugLog();
    if (f) {
      fprintf(f,
              "[S%d] t=%.2f pos(%.1f,%.1f) dir=%.1f vel(%.1f,%.1f) spd=%.0f "
              "tgt(%.0f,%.0f) btDev(%.0f,%.0f) angle=%.2f fwd(%.2f,%.2f) "
              "anchor(%.0f,%.0f) radius=%.0f cursor(%.0f,%.0f) grabber=%d\\n",
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
}"""

update_new = """void Goose::UpdatePhysics(double dt, int w, int h) {
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
      DebugLog("[SHUDDER] t=%.1f g%d pos(%.0f,%.0f) dir=%.0f dirChanges=%d distMoved=%.0f state=%d\\n",
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
      DebugLog("[STUCK] t=%.1f g%d pos(%.0f,%.0f) state=%d vel(%.1f,%.1f)\\n",
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
            "anchor(%.0f,%.0f) radius=%.0f cursor(%.0f,%.0f) grabber=%d\\n",
            id, time, pos.x, pos.y, dir, vel.x, vel.y, currentSpeed, target.x,
            target.y, btDev.x, btDev.y, snatchAngle, snatchFwd.x, snatchFwd.y,
            snatchAnchor.x, snatchAnchor.y, snatchRadius, cursor.position.x,
            cursor.position.y, g_cursorGrabberId);
  }
}

CursorAction Goose::Update(double dt, double time, int w, int h,
                           const CursorState &cursor) {
  lastUpdateTime = time;
  if (state != prevState) {
    const char *stateNames[] = {"W", "F", "R", "C", "S"};
    DebugLog("!! t=%.1f g%d %s->%s tgt(%.0f,%.0f) c(%.0f,%.0f)\\n", time, id,
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
}"""

goose_cpp = goose_cpp.replace(update_old, update_new)

with open('src/common/goose.cpp', 'w') as f:
    f.write(goose_cpp)
