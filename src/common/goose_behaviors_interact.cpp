// goose_behaviors_interact.cpp
// Interaction behaviors (avoidance, main UpdateBehaviors loop)
#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "assets.h"
#include "goose_behaviors.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

static FILE* GetDebugLog() {
    static FILE* f = nullptr;
    if (!f) {
        f = fopen("/tmp/goose_debug.log", "a");
        if (!f) f = stderr;
    }
    return f;
}

bool isTargetReached(Goose& g, float threshold) {
    Vector2 posDev = g.pos;
    float dist = Vector2::Distance(posDev, g.target);

    Vector2 toTarget = g.target - posDev;
    Vector2 velDev = g.vel; // already in device coords (pixels/frame)
    float dot = Vector2::Length(toTarget) > 0.001f ? Dot(Vector2::Normalize(toTarget), Vector2::Normalize(velDev)) : 0.0f;

    return dist < threshold || (dist < threshold * 3.0f && dot < 0.0f);
}

CursorAction Goose::UpdateBehaviors(double dt, double time, int w, int h, const CursorState& cursor) {
    extern void initHonkState(Goose::HonkState& hs, double time);
    extern void updateIdleHonk(Goose::HonkState& hs, double time, double cd, double& lastGeneric);
    initHonkState(honkState, time);

    // --- Joy Suggestions (Dodging) ---
    if (cursor.hasPos() && state != GooseState::SNATCH_CURSOR) {
        Vector2 headPos = GetBeakTipDevice();
        float distToHead = Vector2::Distance(cursor.position, headPos);
        float cursorSpeed = Vector2::Distance(cursor.position, lastCursorPos) / (dt > 0.001 ? dt : 0.016);
        
        if (g_config.behaviors.fun.avoidance && cursorSpeed > 2500.0f && distToHead < 200.0f && !isSurprised) {
            Vector2 cursorVel = cursor.position - lastCursorPos;
            Vector2 toGoose = headPos - cursor.position;
            if (Vector2::Length(cursorVel) > 0 && Vector2::Length(toGoose) > 0) {
                if (Dot(Vector2::Normalize(cursorVel), Vector2::Normalize(toGoose)) > 0.8f) {
                    isSurprised = true;
                    surprisedTime = time;
                    target = pos + Vector2::Normalize(pos - cursor.position) * WorldCoord::Scale(400.0f);
                    state = GooseState::WANDER;
                }
            }
        }
        
        lastCursorPos = cursor.position;
    }
    
    if (isSurprised && time - surprisedTime > 1.5) {
        isSurprised = false;
    }

    if (heldItem != nullptr && state == GooseState::WANDER) {
        state = GooseState::RETURNING;
        if (target.x < 0 || target.x > w || target.y < 0 || target.y > h) {
            target = {static_cast<float>(rand() % (std::max(1, w - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset),
                      static_cast<float>(rand() % (std::max(1, h - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset)};
        }
    }

    if (state == GooseState::SNATCH_CURSOR) {
        extern Vector2 GetSnatchForward(float dir, const Vector2& isoScale);
        snatchAngle += snatchAngularSpeed * (float)dt;

        Vector2 goosePosDev = pos;
        Vector2 btDevice = GetBeakTipDevice();
        btDevice.x = std::clamp(btDevice.x, 0.0f, (float)std::max(0, w - 1));
        btDevice.y = std::clamp(btDevice.y, 0.0f, (float)std::max(0, h - 1));

        int moveX = std::lround(btDevice.x);
        int moveY = std::lround(btDevice.y);

        FILE* f = GetDebugLog();
        fprintf(f, "[SNATCH] t=%.1f g%d: posDev(%.0f,%.0f) vel(%.0f,%.0f) dir=%.0f snt=%.1f dur=%.1f\n",
                time, id, goosePosDev.x, goosePosDev.y, vel.x, vel.y, dir,
                time - snatchStartTime, snatchDuration);
        fprintf(f, "  btDev(%.0f,%.0f) cursor(%.0f,%.0f) caps=%d\n",
                btDevice.x, btDevice.y, cursor.position.x, cursor.position.y, cursor.caps);
        fprintf(f, "  sfwd(%.2f,%.2f) radius=%.0f angle=%.2f angular=%.2f\n",
                snatchFwd.x, snatchFwd.y, snatchRadius, snatchAngle, snatchAngularSpeed);
        fprintf(f, "  offset(%.0f,%.0f) pull=%.0f\n",
                snatchOffset.x, snatchOffset.y, snatchPullDistance);

        Vector2 cursorAction{0, 0};
        if (cursor.caps & CAP_MOVE_ABS) {
            cursorAction = Vector2{(float)moveX, (float)moveY};
            fprintf(f, "  -> MOVEABS to (%d,%d)\n", moveX, moveY);
        } else if (cursor.caps & CAP_MOVE_REL) {
            Vector2 delta = btDevice - cursor.position;
            float maxStep = g_config.snatch.tier3MaxStep * (float)dt;
            if (Vector2::Length(delta) > maxStep) {
                delta = Vector2::Normalize(delta) * maxStep;
            }
            if (std::abs(delta.x) >= g_config.snatch.tier3MinDelta || std::abs(delta.y) >= g_config.snatch.tier3MinDelta) {
                cursorAction = Vector2{delta.x, delta.y};
                fprintf(f, "  -> MOVEREL delta(%.0f,%.0f)\n", delta.x, delta.y);
            } else {
                fprintf(f, "  -> MOVEREL skipped (delta too small)\n");
            }
        }

        Vector2 snatchFwdLocal = GetSnatchForward(dir, ISO_SCALE);
        Vector2 right{-snatchFwdLocal.y, snatchFwdLocal.x};
        float lateralBias = Clamp(snatchOffset.y, -snatchPullDistance * g_config.snatch.lateralBiasLimit, snatchPullDistance * g_config.snatch.lateralBiasLimit);
        float forwardBias = Clamp(snatchOffset.x * g_config.snatch.forwardBiasScale, snatchPullDistance * g_config.snatch.forwardBiasMin, snatchPullDistance * g_config.snatch.forwardBiasMax);
        Vector2 anchorDev = snatchAnchor; // already device coords (set from goose->pos in StartSnatch)
        Vector2 endpointDev = anchorDev - snatchFwdLocal * WorldCoord::Scale(snatchPullDistance) + right * WorldCoord::Scale(lateralBias) + snatchFwdLocal * WorldCoord::Scale(forwardBias);
        endpointDev += right * WorldCoord::Scale(std::cos(snatchAngle) * snatchRadius) + snatchFwdLocal * WorldCoord::Scale(std::sin(snatchAngle) * snatchRadius);
        endpointDev.x = std::clamp(endpointDev.x, 0.0f, (float)std::max(0, w - 1));
        endpointDev.y = std::clamp(endpointDev.y, 0.0f, (float)std::max(0, h - 1));
        target = snatchAnchor + (endpointDev - anchorDev);

        fprintf(f, "  target=(%.0f,%.0f) distToTarget=%.1f\n", target.x, target.y, Vector2::Distance(pos, target));

        if (time - snatchStartTime > snatchDuration) {
            fprintf(f, "  -> END SNATCH (timeout)\n");
            EndSnatch(time, w, h);
        }
        fflush(f);

        if (cursor.caps & CAP_MOVE_ABS) {
            return CursorAction::MoveAbs(moveX, moveY);
        } else if (cursor.caps & CAP_MOVE_REL && Vector2::Length(cursorAction) > 0) {
            return CursorAction::MoveRel((int)cursorAction.x, (int)cursorAction.y);
        }
        return {};
    }

    if (state == GooseState::FETCHING && fetchStartTime > 0) {
        double fetchDuration = time - fetchStartTime;
        if (fetchDuration > g_config.item.fetchCooldown * 4.0f) {
            FILE* df = GetDebugLog();
            fprintf(df, "[FETCH] t=%.1f g%d: fetch TIMEOUT (%.1fs > %.1fs), forceItemFetch=%d -> WANDER\n",
                    time, id, fetchDuration, g_config.item.fetchCooldown * 4.0f, forceItemFetch);
            state = GooseState::WANDER;
            PickNewTarget(w, h);
            forceItemFetch = -1;
            fetchStartTime = -999.0;
        }
    }

    if (state == GooseState::CHASE_CURSOR) {
        extern CursorAction handleChaseCursor(Goose& g, double time, const CursorState& cursor, int w, int h);
        return handleChaseCursor(*this, time, cursor, w, h);
    }

    if (state == GooseState::WANDER && time >= honkState.nextIdleHonk) {
        if ((rand() % g_config.honk.idleChanceDivisor) == 0) {
            updateIdleHonk(honkState, time, g_config.honk.genericCooldown, honkState.lastGeneric);
        } else {
            honkState.nextIdleHonk = time + g_config.honk.idleMin + (static_cast<double>(rand() % 1000) / 1000.0) * (g_config.honk.idleMax - g_config.honk.idleMin);
        }
    }

    float threshold = (state == GooseState::RETURNING) ? std::max(WorldCoord::Scale(g_config.spawn.targetReachedThresholdReturn), g_config.spawn.targetReachedMinReturn)
                                           : std::max(WorldCoord::Scale(g_config.spawn.targetReachedThresholdNormal), g_config.spawn.targetReachedMinNormal);

    bool reached = isTargetReached(*this, threshold);
    FILE* f = GetDebugLog();
    if (state == GooseState::WANDER) {
        Vector2 btPoint = GetBeakTipDevice();
        fprintf(f, "[TARGET] t=%.1f g%d: state=%d bt(%.0f,%.0f) tgt(%.0f,%.0f) dist=%.1f thr=%.1f reached=%d\n",
                time, id, state, btPoint.x, btPoint.y, target.x, target.y,
                Vector2::Distance(btPoint, target), threshold, reached);
    }

    if (reached) {
        if (state == GooseState::WANDER) {
            extern void handleWander(Goose& g, double time, const CursorState& cursor, int w, int h);
            handleWander(*this, time, cursor, w, h);
        }
        else if (state == GooseState::FETCHING) {
            extern void handleFetching(Goose& g, double time, int w, int h);
            handleFetching(*this, time, w, h);
        }
        else if (state == GooseState::RETURNING) {
            extern void handleReturning(Goose& g, double time, int w, int h);
            handleReturning(*this, time, w, h);
        }
    }

    extern void tryPickupItem(Goose& g, double time, int w, int h);
    tryPickupItem(*this, time, w, h);
    return {};
}

void Goose::UpdateChaseCursor(double time, const Vector2& cursorPos) {
}
