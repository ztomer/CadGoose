#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "assets.h"
#include "ai_text_meme.h"
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
    state = GooseState::SNATCH_CURSOR;
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
    state = GooseState::WANDER;
    fprintf(f, "[ENDSNATCH] g%d: now state=%d\n", id, state);
    PickNewTarget(w, h);
    triggerHonk(honkState, time, g_config.honk.genericCooldown, honkState.lastGeneric);
}

static CursorAction handleChaseCursor(Goose& g, double time, const CursorState& cursor, int w, int h) {
    if (g_cursorGrabberId != -1 && g_cursorGrabberId != g.id) {
        g.state = GooseState::WANDER;
        g.PickNewTarget(w, h);
        return {};
    }

    if (!cursor.hasPos()) {
        g.state = GooseState::WANDER;
        g.PickNewTarget(w, h);
        return {};
    }

    double chaseDuration = time - g.chaseStartTime;
    if (chaseDuration > g_config.item.fetchCooldown * 2.0f) {
        FILE* f = GetDebugLog();
        fprintf(f, "[CHASE] t=%.1f g%d: chase timeout (%.1fs)\n", time, g.id, chaseDuration);
        g.state = GooseState::WANDER;
        g.PickNewTarget(w, h);
        return {};
    }

    g.target = cursor.position;

    // BEHAVIOR.md line 228: Catch when beak tip reaches cursor
    Vector2 btPoint = g.GetBeakTipDevice();
    float catchThreshold = std::max(WorldCoord::Scale(22.0f), 15.0f);
    float dist = Vector2::Distance(btPoint, g.target);
    FILE* f = GetDebugLog();
    Vector2 bodyDev = WorldCoord::RigBody(g);
    Vector2 neckDev = WorldCoord::RigNeckHead(g);
    fprintf(f, "[CHASE] t=%.1f g%d: dir=%.0f body=(%.0f,%.0f) neck=(%.0f,%.0f) beak=(%.0f,%.0f) cursor=(%.0f,%.0f) dist=%.1f thr=%.1f grab=%d\n",
            time, g.id, g.dir, bodyDev.x, bodyDev.y, neckDev.x, neckDev.y,
            btPoint.x, btPoint.y, g.target.x, g.target.y, dist, catchThreshold, g_cursorGrabberId);
    if (dist < catchThreshold) {
        if (g_cursorGrabberId == -1) {
            g.StartSnatch(time, g.target);
        } else {
            g.state = GooseState::WANDER;
            g.PickNewTarget(w, h);
        }
    }
    return {};
}

static bool shouldPickupItem(Goose& g) {
    return !g.heldItem && (g.state == GooseState::WANDER || g.state == GooseState::FETCHING);
}

static bool canPickupItem(double timeSinceDropped) {
    return timeSinceDropped > g_config.item.pickupCooldown;
}

static void tryPickupItem(Goose& g, double time, int w, int h) {
    if (!shouldPickupItem(g)) return;
    if (g_droppedItems.empty()) return;

    Vector2 btPoint = g.GetBeakTipDevice();

    for (auto it = g_droppedItems.begin(); it != g_droppedItems.end(); ++it) {
        if (it->pinned) continue;
        Vector2 itemCenter = WorldCoord::ItemCenter(*it);
        if (Vector2::Distance(btPoint, itemCenter) < WorldCoord::Scale(g_config.spawn.itemPickupDistance)) {
            if (!canPickupItem(time - it->timeDropped)) continue;

            g.heldItem = it->data;
            g_droppedItems.erase(it);
            g.state = GooseState::RETURNING;
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
            g.state = GooseState::CHASE_CURSOR;
            g.chaseStartTime = time;
            g.target = cursor.position;
            triggerHonk(g.honkState, time, g_config.honk.chaseCooldown, g.honkState.lastChase);
            chased = true;
        }
    }

    if (!chased) {
        bool canFetch = (time - g.lastDropTime) > g_config.item.fetchCooldown && !g.isResting;
        FILE* f = GetDebugLog();
        fprintf(f, "[FETCH] t=%.1f g%d: lastDrop=%.1f cooldown=%.1f canFetch=%d resting=%d\n",
                time, g.id, g.lastDropTime, g_config.item.fetchCooldown, canFetch, g.isResting);

        int memeProb = g_config.general.memesEnabled ? g.memeFetchBias : 0;
        int noteProb = g.noteFetchBias;
        int trigger = g_config.item.fetchBaseChance + memeProb + noteProb;
        if (trigger > g_config.item.maxFetchBias) trigger = g_config.item.maxFetchBias;

        int fetchCount = 0;
        for (auto& other : g_geese) if (other.state == GooseState::FETCHING) fetchCount++;

        int fetchRoll = rand() % 100;
        fprintf(f, "[FETCH] t=%.1f g%d: trigger=%d roll=%d\n",
                time, g.id, trigger, fetchRoll);

        if (canFetch && fetchCount < g_config.item.maxFetchGeese && fetchRoll < trigger) {
            // 70% meme, 30% text when memes enabled
            if (g_config.general.memesEnabled && rand() % 100 < 70) {
                g.ForceFetch(0, w, h, time); // meme
            } else {
                g.ForceFetch(1, w, h, time); // text
            }
        } else {
            g.PickNewTarget(w, h);

            if (g_config.general.memesEnabled && (rand() % 100) < g_config.item.heistChancePercent && !g_droppedItems.empty()) {
                std::vector<std::list<DroppedItem>::iterator> validItems;
                for (auto it = g_droppedItems.begin(); it != g_droppedItems.end(); ++it) {
                    if (!it->pinned) validItems.push_back(it);
                }
                if (!validItems.empty()) {
                    auto it = validItems[rand() % validItems.size()];
                    Vector2 centerDevice = WorldCoord::ItemCenter(*it);
                    Vector2 gooseScreen = WorldCoord::GoosePos(g);
                    Vector2 toCenter = centerDevice - gooseScreen;
                    float len = Vector2::Length(toCenter);
                    Vector2 approachDir = (len < 1e-4f) ? Vector2{0.0f, -1.0f} : Vector2{toCenter.x / len, toCenter.y / len};
                    float halfDim = std::max(WorldCoord::DeviceSize(it->data->w).x, WorldCoord::DeviceSize(it->data->h).y) * 0.5f;
                    float margin = WorldCoord::Scale((float)g_config.item.heistApproachMargin);
                    Vector2 approachDevice = centerDevice - approachDir * (halfDim + margin);
                    g.target = approachDevice;
                }
            }

            if ((rand() % g_config.honk.wanderHonkDivisor) == 0) {
                triggerHonk(g.honkState, time, g_config.honk.genericCooldown, g.honkState.lastGeneric);
            }
        }
    }
}

static void handleFetching(Goose& g, double time, int w, int h) {
    fprintf(stderr, "[HF] g%d handleFetching called state=%d\n", g.id, (int)g.state);

    if (g.heldItem) {
        fprintf(stderr, "[HF] g%d deleting existing heldItem\n", g.id);
        delete g.heldItem;
        g.heldItem = nullptr;
    }

    if (!g.forcedText.empty()) {
        g.heldItem = g_assets.CreateTextItem(g.forcedText);
    } else if (g.forceItemFetch == 0) {
        g.heldItem = g_assets.GetRandomMeme(w, h, 0.1f);
    } else if (g.forceItemFetch == 1) {
        if (AI_TextMeme_HasAvailable()) {
            std::string aiText = AI_TextMeme_Dequeue();
            g.heldItem = g_assets.CreateTextItem(aiText);
        } else {
            g.heldItem = g_assets.GetRandomText();
        }
    } else {
        g.heldItem = (rand() % 2 == 0) ? g_assets.GetRandomMeme() : g_assets.GetRandomText();
    }

    fprintf(stderr, "[HF] g%d heldItem=%p image=%p after creation\n", g.id, (void*)g.heldItem, g.heldItem ? (void*)g.heldItem->image : nullptr);

    g.forceItemFetch = -1;
    g.forcedText.clear();

if (g.heldItem) {
        g.state = GooseState::RETURNING;
        fprintf(stderr, "[HF] g%d -> RETURNING, dragPos=(%.1f,%.1f) dragInit=%d\n", g.id, g.dragPos.x, g.dragPos.y, g.dragInit);
        g.target = {static_cast<float>(rand() % (std::max(1, w - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset),
                    static_cast<float>(rand() % (std::max(1, h - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset)};
        triggerHonk(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
    } else {
        fprintf(stderr, "[HF] g%d heldItem=null, staying WANDER\n", g.id);
        g.state = GooseState::WANDER;
        g.PickNewTarget(w, h);
    }
}

static void handleReturning(Goose& g, double time, int w, int h) {
    if (g.heldItem) {
        DroppedItem drop;
        drop.data = g.heldItem;

        Vector2 btPoint = g.GetBeakTipDevice();
        drop.pos = btPoint - WorldCoord::ItemHalfSize(g.heldItem);
        drop.rotation = g.dragRot;
        drop.timeDropped = time;

        if (std::isfinite(drop.pos.x) && std::isfinite(drop.pos.y) && std::isfinite(drop.rotation)) {
            float minX = 0.0f, minY = 0.0f;
            Vector2 itemHalf = WorldCoord::ItemHalfSize(g.heldItem);
            float maxX = static_cast<float>(w) - itemHalf.x * 2.0f;
            float maxY = static_cast<float>(h) - itemHalf.y * 2.0f;

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

    g.lastDropTime = time;
    g.state = GooseState::WANDER;
    g.PickNewTarget(w, h);
    g.stepTime = g_config.step.timeWander;
    triggerHonk(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
}

static bool isTargetReached(Goose& g, float threshold) {
    Vector2 posDev = WorldCoord::GoosePos(g);
    float dist = Vector2::Distance(posDev, g.target);

    // Also check if goose has passed/overshot target (velocity pointing away from target)
    Vector2 toTarget = g.target - posDev;
    Vector2 velDev = g.vel * g_config.general.globalScale;
    float dot = Vector2::Length(toTarget) > 0.001f ? Dot(Vector2::Normalize(toTarget), Vector2::Normalize(velDev)) : 0.0f;

    // If dot < 0, goose is moving away from target (overshot)
    // Use body distance + check for overshoot
    return dist < threshold || (dist < threshold * 3.0f && dot < 0.0f);
}

CursorAction Goose::UpdateBehaviors(double dt, double time, int w, int h, const CursorState& cursor) {
    initHonkState(honkState, time);

    // --- Joy Suggestions (Petting & Dodging) ---
    if (cursor.hasPos() && state != GooseState::SNATCH_CURSOR) {
        Vector2 headPos = GetBeakTipDevice();
        float distToHead = Vector2::Distance(cursor.position, headPos);
        float cursorSpeed = Vector2::Distance(cursor.position, lastCursorPos) / (dt > 0.001 ? dt : 0.016);
        
        // 1. Petting
        if (distToHead < 60.0f) {
            cursorWiggleAmount += cursorSpeed * dt;
            if (cursorWiggleAmount > 600.0f && !isPetted) {
                isPetted = true;
                pettedTime = time;
                g_assets.Honk(); // Purr
                vel = {0, 0};    // Stop to enjoy the pets
            }
        } else {
            cursorWiggleAmount *= 0.95f; // Decay
        }
        
        // 2. Cursor Avoidance (Dodging)
        if (cursorSpeed > 2500.0f && distToHead < 200.0f && !isSurprised && !isPetted) {
            Vector2 cursorVel = cursor.position - lastCursorPos;
            Vector2 toGoose = headPos - cursor.position;
            if (Vector2::Length(cursorVel) > 0 && Vector2::Length(toGoose) > 0) {
                if (Dot(Vector2::Normalize(cursorVel), Vector2::Normalize(toGoose)) > 0.8f) {
                    isSurprised = true;
                    surprisedTime = time;
                    target = pos + Vector2::Normalize(pos - cursor.position) * 400.0f;
                    state = GooseState::WANDER;
                }
            }
        }
        
        lastCursorPos = cursor.position;
    }
    
    if (isPetted && time - pettedTime > 2.0) {
        isPetted = false;
        cursorWiggleAmount = 0.0f;
    }
    if (isSurprised && time - surprisedTime > 1.5) {
        isSurprised = false;
    }
    // ---------------------------------------------

    if (heldItem != nullptr && state == GooseState::WANDER) {
        state = GooseState::RETURNING;
        if (target.x < 0 || target.x > w || target.y < 0 || target.y > h) {
            target = {static_cast<float>(rand() % (std::max(1, w - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset),
                      static_cast<float>(rand() % (std::max(1, h - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset)};
        }
    }

    if (state == GooseState::SNATCH_CURSOR) {
        snatchAngle += snatchAngularSpeed * (float)dt;

        Vector2 goosePosDev = WorldCoord::GoosePos(*this);
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

        Vector2 right{-snatchFwd.y, snatchFwd.x};
        float lateralBias = Clamp(snatchOffset.y, -snatchPullDistance * g_config.snatch.lateralBiasLimit, snatchPullDistance * g_config.snatch.lateralBiasLimit);
        float forwardBias = Clamp(snatchOffset.x * g_config.snatch.forwardBiasScale, snatchPullDistance * g_config.snatch.forwardBiasMin, snatchPullDistance * g_config.snatch.forwardBiasMax);
        // Compute target in device space relative to ANCHOR (not current position)
        Vector2 anchorDev = WorldCoord::ToDevice(snatchAnchor, *this);
        Vector2 endpointDev = anchorDev - snatchFwd * WorldCoord::Scale(snatchPullDistance) + right * WorldCoord::Scale(lateralBias) + snatchFwd * WorldCoord::Scale(forwardBias);
        endpointDev += right * WorldCoord::Scale(std::cos(snatchAngle) * snatchRadius) + snatchFwd * WorldCoord::Scale(std::sin(snatchAngle) * snatchRadius);
        endpointDev.x = std::clamp(endpointDev.x, 0.0f, (float)std::max(0, w - 1));
        endpointDev.y = std::clamp(endpointDev.y, 0.0f, (float)std::max(0, h - 1));
        // Convert device offset to world offset using ANCHOR
        target = snatchAnchor + (endpointDev - anchorDev);

        fprintf(f, "  target=(%.0f,%.0f) distToTarget=%.1f\n", target.x, target.y, Vector2::Distance(pos, target));

        if (time - snatchStartTime > snatchDuration) {
            fprintf(f, "  -> END SNATCH (timeout)\n");
            EndSnatch(time, w, h);
        }
        fflush(f);

        // Return cursor action if any
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
            fprintf(df, "[FETCH] t=%.1f g%d: fetch timeout (%.1fs)\n", time, id, fetchDuration);
            state = GooseState::WANDER;
            PickNewTarget(w, h);
            forceItemFetch = -1;
        }
    }

    if (state == GooseState::CHASE_CURSOR) {
        return handleChaseCursor(*this, time, cursor, w, h);
    }

    if (state == GooseState::WANDER && time >= honkState.nextIdleHonk) {
        if ((rand() % g_config.honk.idleChanceDivisor) == 0) {
            updateIdleHonk(honkState, time, g_config.honk.genericCooldown, honkState.lastGeneric);
        } else {
            honkState.nextIdleHonk = time + g_config.honk.idleMin + Rand01() * (g_config.honk.idleMax - g_config.honk.idleMin);
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
        if (state == GooseState::WANDER) handleWander(*this, time, cursor, w, h);
        else if (state == GooseState::FETCHING) handleFetching(*this, time, w, h);
        else if (state == GooseState::RETURNING) handleReturning(*this, time, w, h);
    }

    tryPickupItem(*this, time, w, h);
    return {};
}

void Goose::UpdateChaseCursor(double time, const Vector2& cursorPos) {
}
