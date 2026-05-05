#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "assets.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

static inline double Rand01() { return static_cast<double>(rand() % 1000) / 1000.0; }

static FILE* GetDebugLog() {
    static FILE* f = nullptr;
    if (!f) {
        f = fopen("/tmp/goose_debug.log", "a");
        if (!f) f = stderr;
    }
    return f;
}

Vector2 GetSnatchForward(float dir, const Vector2& isoScale) {
    float rad = dir * static_cast<float>(PI) / 180.0f;
    return {std::cos(rad) * isoScale.x, std::sin(rad) * isoScale.y};
}

void triggerHonk(Goose::HonkState& hs, double time, double cd, double& lastBucket) {
    if ((time - hs.lastAny) < g_config.honk.minGap) return;
    if ((time - lastBucket) < cd) return;
    g_assets.Honk();
    hs.lastAny = time;
    lastBucket = time;
}

void initHonkState(Goose::HonkState& hs, double time) {
    if (hs.init) return;
    hs.init = true;
    hs.lastAny = -1e9;
    hs.lastChase = -1e9;
    hs.lastFetch = -1e9;
    hs.lastGeneric = -1e9;
    hs.nextIdleHonk = time + g_config.honk.idleMin + Rand01() * (g_config.honk.idleMax - g_config.honk.idleMin);
}

void updateIdleHonk(Goose::HonkState& hs, double time, double cd, double& lastGeneric) {
    if (hs.nextIdleHonk > time + g_config.honk.idleCheckAhead) return;
    triggerHonk(hs, time, cd, lastGeneric);
    hs.nextIdleHonk = time + g_config.honk.idleMin + Rand01() * (g_config.honk.idleMax - g_config.honk.idleMin);
}

void Goose::StartSnatch(double time, const Vector2& cursorPos) {
    g_cursorGrabberId = id;
    state = SNATCH_CURSOR;
    snatchStartTime = time;
    snatchAnchor = pos;
    snatchFwd = GetSnatchForward(dir, ISO_SCALE);

    Vector2 catchRight{-snatchFwd.y, snatchFwd.x};
    Vector2 cursorDelta = cursorPos - pos;
    snatchOffset.x = Clamp(Dot(cursorDelta, snatchFwd), -g_config.snatch.offsetMax, g_config.snatch.offsetMax);
    snatchOffset.y = Clamp(Dot(cursorDelta, catchRight), -g_config.snatch.offsetMax, g_config.snatch.offsetMax);

    snatchAngle = 0.0f;
    snatchRadius = g_config.snatch.radiusBase + (rand() % (int)g_config.snatch.radiusRange);
    snatchAngularSpeed = ((rand() % 2) ? 1.0f : -1.0f) * (g_config.snatch.angularSpeedBase + (rand() % (int)g_config.snatch.angularSpeedRandomRange) / 100.0f);
    currentSpeed = g_config.movement.baseRunSpeed * g_config.snatch.speedMultiplier;
    stepTime = g_config.step.timeSnatch;

    g_assets.Bite();
    triggerHonk(honkState, time, g_config.honk.chaseCooldown, honkState.lastChase);
}

void Goose::EndSnatch(double time, int w, int h) {
    FILE* f = GetDebugLog();
    fprintf(f, "[ENDSNATCH] t=%.1f g%d: was state=%d grabId=%d\n", time, id, state, g_cursorGrabberId);
    stepTime = g_config.step.timeWander;
    if (g_cursorGrabberId == id) {
        fprintf(f, "[ENDSNATCH] g%d: releasing cursor grab (was %d)\n", id, g_cursorGrabberId);
        g_cursorGrabberId = -1;
    }
    state = WANDER;
    fprintf(f, "[ENDSNATCH] g%d: now state=%d\n", id, state);
    PickNewTarget(w, h);
    triggerHonk(honkState, time, g_config.honk.genericCooldown, honkState.lastGeneric);
}

static CursorAction handleChaseCursor(Goose& g, double time, const CursorState& cursor, int w, int h) {
    if (g_cursorGrabberId != -1 && g_cursorGrabberId != g.id) {
        g.state = WANDER;
        g.PickNewTarget(w, h);
        return {};
    }

    if (!cursor.hasPos()) {
        g.state = WANDER;
        g.PickNewTarget(w, h);
        return {};
    }

    g.target = cursor.position;

    // BEHAVIOR.md line 228: Catch when beak tip reaches cursor
    Vector2 btPoint = g.GetBeakTipDevice();
    float catchThreshold = std::max(22.0f * g_config.general.globalScale, 15.0f);
    float dist = Vector2::Distance(btPoint, g.target);
    FILE* f = GetDebugLog();
    fprintf(f, "[CHASE] t=%.1f g%d: dir=%.0f body=(%.0f,%.0f) neck=(%.0f,%.0f) beak=(%.0f,%.0f) cursor=(%.0f,%.0f) dist=%.1f thr=%.1f grab=%d\n",
            time, g.id, g.dir, g.pos.x, g.pos.y, g.rig.neckHead.x, g.rig.neckHead.y,
            btPoint.x, btPoint.y, g.target.x, g.target.y, dist, catchThreshold, g_cursorGrabberId);
    if (dist < catchThreshold) {
        if (g_cursorGrabberId == -1) {
            g.StartSnatch(time, g.target);
        } else {
            g.state = WANDER;
            g.PickNewTarget(w, h);
        }
    }
    return {};
}

static bool shouldPickupItem(Goose& g) {
    return !g.heldItem && (g.state == WANDER || g.state == FETCHING);
}

static bool canPickupItem(double timeSinceDropped) {
    return timeSinceDropped > g_config.item.pickupCooldown;
}

static void tryPickupItem(Goose& g, double time, int w, int h) {
    if (!shouldPickupItem(g)) return;
    if (g_droppedItems.empty()) return;

    Vector2 btPoint = g.GetBeakTipDevice();

    for (auto it = g_droppedItems.begin(); it != g_droppedItems.end(); ++it) {
        Vector2 itemCenter = it->pos + Vector2{static_cast<float>(it->data->w * 0.5f) * g_config.general.globalScale,
                                               static_cast<float>(it->data->h * 0.5f) * g_config.general.globalScale};
        if (Vector2::Distance(btPoint, itemCenter) < g_config.spawn.itemPickupDistance * g_config.general.globalScale) {
            if (!canPickupItem(time - it->timeDropped)) continue;

            g.heldItem = it->data;
            g_droppedItems.erase(it);
            g.state = RETURNING;
            g.target = {static_cast<float>(rand() % (std::max(1, (int)(w - g_config.spawn.itemDropMarginX * 2)) + (int)g_config.spawn.itemDropMarginX)),
                        static_cast<float>(rand() % (std::max(1, (int)(h - g_config.spawn.itemDropMarginY * 2)) + (int)g_config.spawn.itemDropMarginY))};
            g_assets.Bite();
            triggerHonk(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
            return;
        }
    }
}

static void handleWander(Goose& g, double time, const CursorState& cursor, int w, int h) {
    bool chased = false;

    bool canChase = (g_cursorGrabberId == -1);
    bool chaseEnabled = g.cursorChaseEnabled;
    bool cursorValid = cursor.hasPos();
    if (canChase && chaseEnabled && cursorValid) {
        int totalChance = g_config.cursor.chaseChance + g.attackMouseBias;
        int roll = rand() % 100;
        FILE* f = GetDebugLog();
        fprintf(f, "[WALKER] t=%.1f g%d: grab=-%d ena=%d valid=%d chance=%d roll=%d\n",
                time, g.id, g_cursorGrabberId, chaseEnabled, cursorValid, totalChance, roll);
        if (totalChance > 100) totalChance = 100;
        if (roll < totalChance) {
            g.state = CHASE_CURSOR;
            g.target = cursor.position;
            triggerHonk(g.honkState, time, g_config.honk.chaseCooldown, g.honkState.lastChase);
            chased = true;
        }
    }

    if (!chased) {
        int memeProb = g_config.general.memesEnabled ? g.memeFetchBias : 0;
        int noteProb = g.noteFetchBias;
        int trigger = g_config.item.fetchBaseChance + memeProb + noteProb;
        if (trigger > g_config.item.maxFetchBias) trigger = g_config.item.maxFetchBias;

        int fetchCount = 0;
        for (auto& other : g_geese) if (other.state == FETCHING) fetchCount++;

        if (fetchCount < g_config.item.maxFetchGeese && (rand() % 100) < trigger) {
            int total = memeProb + noteProb;
            if (total <= 0) {
                g.ForceFetch(rand() % 2, w, h);
            } else {
                int pick = rand() % total;
                if (pick < memeProb) g.ForceFetch(0, w, h);
                else g.ForceFetch(1, w, h);
            }
        } else {
            g.PickNewTarget(w, h);

            if (g_config.general.memesEnabled && (rand() % 100) < g_config.item.heistChancePercent && !g_droppedItems.empty()) {
                auto it = g_droppedItems.begin();
                std::advance(it, rand() % g_droppedItems.size());
                Vector2 centerDevice = it->pos + Vector2{static_cast<float>(it->data->w) * 0.5f * g_config.general.globalScale,
                                                         static_cast<float>(it->data->h) * 0.5f * g_config.general.globalScale};
                Vector2 gooseScreen = WorldToDevice(g.pos, g.pos, g_config.general.globalScale);
                Vector2 toCenter = centerDevice - gooseScreen;
                float len = Vector2::Length(toCenter);
                Vector2 approachDir = (len < 1e-4f) ? Vector2{0.0f, -1.0f} : Vector2{toCenter.x / len, toCenter.y / len};
                float halfDim = std::max(it->data->w, it->data->h) * 0.5f * g_config.general.globalScale;
                float margin = (float)g_config.item.heistApproachMargin * g_config.general.globalScale;
                Vector2 approachDevice = centerDevice - approachDir * (halfDim + margin);
                g.target = approachDevice;
            }

            if ((rand() % g_config.honk.wanderHonkDivisor) == 0) {
                triggerHonk(g.honkState, time, g_config.honk.genericCooldown, g.honkState.lastGeneric);
            }
        }
    }
}

static void handleFetching(Goose& g, double time, int w, int h) {
    if (g.heldItem) {
        delete g.heldItem;
        g.heldItem = nullptr;
    }

    if (!g.forcedText.empty()) {
        g.heldItem = g_assets.CreateTextItem(g.forcedText);
    } else if (g.forceItemFetch == 0) {
        g.heldItem = g_assets.GetRandomMeme();
    } else if (g.forceItemFetch == 1) {
        g.heldItem = g_assets.GetRandomText();
    } else {
        g.heldItem = (rand() % 2 == 0) ? g_assets.GetRandomMeme() : g_assets.GetRandomText();
    }

    g.forceItemFetch = -1;
    g.forcedText.clear();

    if (g.heldItem) {
        g.state = RETURNING;
        g.target = {static_cast<float>(rand() % (std::max(1, w - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset),
                    static_cast<float>(rand() % (std::max(1, h - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset)};
        triggerHonk(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
    } else {
        g.state = WANDER;
        g.PickNewTarget(w, h);
    }
}

static void handleReturning(Goose& g, double time, int w, int h) {
    if (g.heldItem) {
        DroppedItem drop;
        drop.data = g.heldItem;

        Vector2 btPoint = g.GetBeakTipDevice();
        drop.pos = btPoint - Vector2{static_cast<float>(g.heldItem->w * 0.5f) * g_config.general.globalScale,
                                     static_cast<float>(g.heldItem->h * 0.5f) * g_config.general.globalScale};
        drop.rotation = g.dragRot;
        drop.timeDropped = time;

        if (std::isfinite(drop.pos.x) && std::isfinite(drop.pos.y) && std::isfinite(drop.rotation)) {
            float minX = 0.0f, minY = 0.0f;
            float maxX = static_cast<float>(w) - g.heldItem->w * g_config.general.globalScale;
            float maxY = static_cast<float>(h) - g.heldItem->h * g_config.general.globalScale;

            if (!g_config.cursor.multiMonitorEnabled) {
            }

            if (drop.pos.x < minX) drop.pos.x = minX;
            if (drop.pos.y < minY) drop.pos.y = minY;
            if (drop.pos.x > maxX) drop.pos.x = maxX;
            if (drop.pos.y > maxY) drop.pos.y = maxY;

            g_droppedItems.push_back(drop);
        } else {
            delete g.heldItem;
        }

        g.heldItem = nullptr;
        g.dragInit = false;
        g_assets.Bite();
    }

    g.state = WANDER;
    g.PickNewTarget(w, h);
    g.stepTime = g_config.step.timeWander;
    triggerHonk(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
}

static bool isTargetReached(Goose& g, float threshold) {
    // goose.pos is already in device coordinates
    float dist = Vector2::Distance(g.pos, g.target);

    // Also check if goose has passed/overshot target (velocity pointing away from target)
    Vector2 toTarget = g.target - g.pos;
    float dot = Vector2::Length(toTarget) > 0.001f ? Dot(Vector2::Normalize(toTarget), Vector2::Normalize(g.vel)) : 0.0f;

    // If dot < 0, goose is moving away from target (overshot)
    // Use body distance + check for overshoot
    return dist < threshold || (dist < threshold * 3.0f && dot < 0.0f);
}

CursorAction Goose::UpdateBehaviors(double dt, double time, int w, int h, const CursorState& cursor) {
    initHonkState(honkState, time);

    if (heldItem != nullptr && state == WANDER) {
        state = RETURNING;
        if (target.x < 0 || target.x > w || target.y < 0 || target.y > h) {
            target = {static_cast<float>(rand() % (std::max(1, w - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset),
                      static_cast<float>(rand() % (std::max(1, h - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset)};
        }
    }

    if (state == SNATCH_CURSOR) {
        snatchAngle += snatchAngularSpeed * (float)dt;

        // BEHAVIOR.md line 228: Cursor moved to beak tip every frame
        Vector2 btDevice = GetBeakTipDevice();
        btDevice.x = std::clamp(btDevice.x, 0.0f, (float)std::max(0, w - 1));
        btDevice.y = std::clamp(btDevice.y, 0.0f, (float)std::max(0, h - 1));

        int moveX = std::lround(btDevice.x);
        int moveY = std::lround(btDevice.y);

        FILE* f = GetDebugLog();
        fprintf(f, "[SNATCH] t=%.1f g%d: dir=%.0f body=(%.0f,%.0f) neck=(%.0f,%.0f) beak=(%.0f,%.0f) moveTo=(%d,%d) cursor=(%.0f,%.0f)\n",
                time, id, dir, pos.x, pos.y, rig.neckHead.x, rig.neckHead.y,
                btDevice.x, btDevice.y, moveX, moveY, cursor.position.x, cursor.position.y);

        if (cursor.caps & CAP_MOVE_ABS) {
            return CursorAction::MoveAbs(moveX, moveY);
        } else if (cursor.caps & CAP_MOVE_REL) {
            Vector2 delta = btDevice - cursor.position;
            float maxStep = g_config.snatch.tier3MaxStep * (float)dt;
            if (Vector2::Length(delta) > maxStep) {
                delta = Vector2::Normalize(delta) * maxStep;
            }
            if (std::abs(delta.x) >= g_config.snatch.tier3MinDelta || std::abs(delta.y) >= g_config.snatch.tier3MinDelta) {
                return CursorAction::MoveRel((int)delta.x, (int)delta.y);
            }
        }

        Vector2 right{-snatchFwd.y, snatchFwd.x};
        float lateralBias = Clamp(snatchOffset.y, -snatchPullDistance * g_config.snatch.lateralBiasLimit, snatchPullDistance * g_config.snatch.lateralBiasLimit);
        float forwardBias = Clamp(snatchOffset.x * g_config.snatch.forwardBiasScale, snatchPullDistance * g_config.snatch.forwardBiasMin, snatchPullDistance * g_config.snatch.forwardBiasMax);
        Vector2 endpoint = pos - snatchFwd * snatchPullDistance + right * lateralBias + snatchFwd * forwardBias;
        endpoint += right * std::cos(snatchAngle) * snatchRadius + snatchFwd * std::sin(snatchAngle) * snatchRadius;
        endpoint.x = std::clamp(endpoint.x, 0.0f, (float)std::max(0, w - 1));
        endpoint.y = std::clamp(endpoint.y, 0.0f, (float)std::max(0, h - 1));
        target = endpoint;

        if (time - snatchStartTime > snatchDuration) {
            EndSnatch(time, w, h);
        }
        return {};
    }

    if (state == CHASE_CURSOR) {
        return handleChaseCursor(*this, time, cursor, w, h);
    }

    if (state == WANDER && time >= honkState.nextIdleHonk) {
        if ((rand() % g_config.honk.idleChanceDivisor) == 0) {
            updateIdleHonk(honkState, time, g_config.honk.genericCooldown, honkState.lastGeneric);
        } else {
            honkState.nextIdleHonk = time + g_config.honk.idleMin + Rand01() * (g_config.honk.idleMax - g_config.honk.idleMin);
        }
    }

    float threshold = (state == RETURNING) ? std::max(g_config.spawn.targetReachedThresholdReturn * g_config.general.globalScale, g_config.spawn.targetReachedMinReturn)
                                           : std::max(g_config.spawn.targetReachedThresholdNormal * g_config.general.globalScale, g_config.spawn.targetReachedMinNormal);

    bool reached = isTargetReached(*this, threshold);
    FILE* f = GetDebugLog();
    if (state == WANDER) {
        Vector2 btPoint = GetBeakTipDevice();
        fprintf(f, "[TARGET] t=%.1f g%d: state=%d bt(%.0f,%.0f) tgt(%.0f,%.0f) dist=%.1f thr=%.1f reached=%d\n",
                time, id, state, btPoint.x, btPoint.y, target.x, target.y,
                Vector2::Distance(btPoint, target), threshold, reached);
    }

    if (reached) {
        if (state == WANDER) handleWander(*this, time, cursor, w, h);
        else if (state == FETCHING) handleFetching(*this, time, w, h);
        else if (state == RETURNING) handleReturning(*this, time, w, h);
    }

    tryPickupItem(*this, time, w, h);
    return {};
}

void Goose::UpdateChaseCursor(double time, const Vector2& cursorPos) {
}
