// goose_behaviors_wander.cpp
// Wander behavior logic
#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "assets.h"
#include "goose_behaviors.h"
#include <cmath>
#include <cstdio>

static FILE* GetDebugLog() {
    static FILE* f = nullptr;
    if (!f) {
        f = fopen("/tmp/goose_debug.log", "a");
        if (!f) f = stderr;
    }
    return f;
}

CursorAction handleChaseCursor(Goose& g, double time, const CursorState& cursor, int w, int h) {
    extern int g_cursorGrabberId;
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

void handleWander(Goose& g, double time, const CursorState& cursor, int w, int h) {
    extern int g_cursorGrabberId;
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
            extern void triggerHonk(Goose::HonkState& hs, double time, double cd, double& lastBucket);
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
        extern std::list<Goose> g_geese;
        for (auto& other : g_geese) if (other.state == GooseState::FETCHING) fetchCount++;

        int fetchRoll = rand() % 100;
        fprintf(f, "[FETCH] t=%.1f g%d: trigger=%d roll=%d fetchCount=%d maxGeese=%d\n",
                time, g.id, trigger, fetchRoll, fetchCount, g_config.item.maxFetchGeese);

        if (canFetch && fetchCount < g_config.item.maxFetchGeese && fetchRoll < trigger) {
            int fetchType;
            if (g_config.general.memesEnabled && rand() % 100 < 70) {
                fetchType = 0;
                fprintf(f, "[FETCH] t=%.1f g%d: TRIGGERED fetch type=MEME\n", time, g.id);
                g.ForceFetch(fetchType, w, h, time);
            } else {
                fetchType = 1;
                fprintf(f, "[FETCH] t=%.1f g%d: TRIGGERED fetch type=TEXT\n", time, g.id);
                g.ForceFetch(fetchType, w, h, time);
            }
        } else {
            if (!canFetch) fprintf(f, "[FETCH] g%d: skipped (cooldown, lastDrop=%.1f)\n", g.id, g.lastDropTime);
            else if (fetchCount >= g_config.item.maxFetchGeese) fprintf(f, "[FETCH] g%d: skipped (max geese fetching=%d)\n", g.id, fetchCount);
            else if (fetchRoll >= trigger) fprintf(f, "[FETCH] g%d: skipped (roll=%d >= trigger=%d)\n", g.id, fetchRoll, trigger);
            g.PickNewTarget(w, h);

            if (g_config.general.memesEnabled && (rand() % 100) < g_config.item.heistChancePercent && !g_droppedItems.empty()) {
                std::vector<std::list<DroppedItem>::iterator> validItems;
                for (auto it = g_droppedItems.begin(); it != g_droppedItems.end(); ++it) {
                    if (!it->pinned) validItems.push_back(it);
                }
                if (!validItems.empty()) {
                    auto it = validItems[rand() % validItems.size()];
                    Vector2 centerDevice = WorldCoord::ItemCenter(*it);
                    Vector2 gooseScreen = g.pos;
                    Vector2 toCenter = centerDevice - gooseScreen;
                    float len = Vector2::Length(toCenter);
                    Vector2 approachDir = (len < 1e-4f) ? Vector2{0.0f, -1.0f} : Vector2{toCenter.x / len, toCenter.y / len};
                    float halfDim = std::max(WorldCoord::ItemSize(it->data).x, WorldCoord::ItemSize(it->data).y) * 0.5f;
                    float margin = WorldCoord::Scale((float)g_config.item.heistApproachMargin);
                    Vector2 approachDevice = centerDevice - approachDir * (halfDim + margin);
                    g.target = approachDevice;
                }
            }

            if ((rand() % g_config.honk.wanderHonkDivisor) == 0) {
                extern void triggerHonk(Goose::HonkState& hs, double time, double cd, double& lastBucket);
                triggerHonk(g.honkState, time, g_config.honk.genericCooldown, g.honkState.lastGeneric);
            }
        }
    }
}
