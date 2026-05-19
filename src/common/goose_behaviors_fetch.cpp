// goose_behaviors_fetch.cpp
// Fetch behavior logic
#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "assets.h"
#include "ai_text_meme.h"
#include "goose_behaviors.h"
#include "actor.h"
#include "actor_dropped_item.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

static constexpr float kSnatchAngularSpeedDivisor = 100.0f;

static FILE* GetDebugLog() {
    static FILE* f = nullptr;
    if (!f) {
        f = fopen("/tmp/goose_debug.log", "a");
        if (!f) f = stderr;
    }
    return f;
}

static inline double Rand01() { return static_cast<double>(rand() % 1000) / 1000.0; }

static void triggerHonkLocal(Goose::HonkState& hs, double time, double cd, double& lastBucket) {
    if ((time - hs.lastAny) < g_config.honk.minGap) return;
    if ((time - lastBucket) < cd) return;
    g_assets.Honk();
    hs.lastAny = time;
    lastBucket = time;
}

void Goose::StartSnatch(double time, const Vector2& cursorPos) {

    g_world.cursorGrabberId = id;
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
    snatchAngularSpeed = ((rand() % 2) ? 1.0f : -1.0f) * (g_config.snatch.angularSpeedBase + (rand() % (int)g_config.snatch.angularSpeedRandomRange) / kSnatchAngularSpeedDivisor);
    currentSpeed = g_config.movement.baseRunSpeed * g_config.snatch.speedMultiplier;
    stepTime = g_config.step.timeSnatch;

    g_assets.Bite();
    triggerHonkLocal(honkState, time, g_config.honk.chaseCooldown, honkState.lastChase);
}

void Goose::EndSnatch(double time, int w, int h) {

    FILE* f = GetDebugLog();
    fprintf(f, "[ENDSNATCH] t=%.1f g%d: was state=%d grabId=%d\n", time, id, state, g_world.cursorGrabberId);
    stepTime = g_config.step.timeWander;
    if (g_world.cursorGrabberId == id) {
        fprintf(f, "[ENDSNATCH] g%d: releasing cursor grab (was %d)\n", id, g_world.cursorGrabberId);
        g_world.cursorGrabberId = -1;
    }
    state = GooseState::WANDER;
    fprintf(f, "[ENDSNATCH] g%d: now state=%d\n", id, state);
    PickNewTarget(w, h);
    triggerHonkLocal(honkState, time, g_config.honk.genericCooldown, honkState.lastGeneric);
}

static bool shouldPickupItem(Goose& g) {
    return !g.heldItem && (g.state == GooseState::WANDER || g.state == GooseState::FETCHING);
}

static bool canPickupItem(double timeSinceDropped) {
    return timeSinceDropped > g_config.item.pickupCooldown;
}

void tryPickupItem(Goose& g, double time, int w, int h) {
    if (!shouldPickupItem(g)) return;

    auto items = ActorManager::Instance().getDroppedItems();
    if (items.empty()) return;

    Vector2 btPoint = g.GetBeakTipDevice();

    for (auto* actor : items) {
        DroppedItem& item = actor->item();
        if (item.pinned) continue;
        Vector2 itemCenter = WorldCoord::ItemCenter(item);
        float dist = Vector2::Distance(btPoint, itemCenter);
        float pickupDist = WorldCoord::Scale(g_config.spawn.itemPickupDistance);
        if (dist < pickupDist) {
            if (!canPickupItem(time - item.timeDropped)) continue;

            fprintf(stderr, "[FETCH] tryPickupItem g%d picking up item at (%.0f,%.0f) dist=%.0f\n",
                    g.id, itemCenter.x, itemCenter.y, dist);
            g.heldItem = item.data;
            ActorManager::Instance().remove(actor);
            g.state = GooseState::RETURNING;
            g.target = {static_cast<float>(rand() % (std::max(1, (int)(w - g_config.spawn.itemDropMarginX * 2)) + (int)g_config.spawn.itemDropMarginX)),
                        static_cast<float>(rand() % (std::max(1, (int)(h - g_config.spawn.itemDropMarginY * 2)) + (int)g_config.spawn.itemDropMarginY))};
            g_assets.Bite();
            triggerHonkLocal(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
            return;
        }
    }
}

void handleFetching(Goose& g, double time, int w, int h) {
    fprintf(stderr, "[FETCH] handleFetching g%d called state=%d forceItemFetch=%d forcedTextEmpty=%d\n",
            g.id, (int)g.state, g.forceItemFetch, g.forcedText.empty());

    if (g.heldItem) {
        fprintf(stderr, "[FETCH] handleFetching g%d deleting existing heldItem\n", g.id);
        delete g.heldItem;
        g.heldItem = nullptr;
    }

    if (!g.forcedText.empty()) {
        fprintf(stderr, "[FETCH] handleFetching g%d creating text item from forcedText (len=%zu)\n", g.id, g.forcedText.size());
        g.heldItem = g_assets.CreateTextItem(g.forcedText);
    } else if (g.forceItemFetch == 0) {
        fprintf(stderr, "[FETCH] handleFetching g%d getting random meme\n", g.id);
        g.heldItem = g_assets.GetRandomMeme(w, h, 0.1f);
    } else if (g.forceItemFetch == 1) {
        fprintf(stderr, "[FETCH] handleFetching g%d dequeuing AI text\n", g.id);
        std::string text = AI_TextMeme_Dequeue();
        if (!text.empty()) {
            fprintf(stderr, "[FETCH] handleFetching g%d AI text dequeued (len=%zu)\n", g.id, text.size());
            g.heldItem = g_assets.CreateTextItem(text);
        } else {
            fprintf(stderr, "[FETCH] handleFetching g%d AI text empty, falling back to file text\n", g.id);
            g.heldItem = g_assets.GetRandomText();
        }
    } else {
        fprintf(stderr, "[FETCH] handleFetching g%d random fetch (forceItemFetch=%d)\n", g.id, g.forceItemFetch);
        g.heldItem = (rand() % 2 == 0) ? g_assets.GetRandomMeme() : g_assets.GetRandomText();
    }

    fprintf(stderr, "[FETCH] handleFetching g%d heldItem=%p image=%p after creation\n",
            g.id, (void*)g.heldItem, g.heldItem ? (void*)g.heldItem->image : nullptr);

    g.forceItemFetch = -1;
    g.forcedText.clear();

    if (g.heldItem) {
        g.state = GooseState::RETURNING;
        fprintf(stderr, "[FETCH] handleFetching g%d -> RETURNING, dragPos=(%.1f,%.1f) dragInit=%d\n", g.id, g.dragPos.x, g.dragPos.y, g.dragInit);
        g.target = {static_cast<float>(rand() % (std::max(1, w - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset),
                    static_cast<float>(rand() % (std::max(1, h - (int)g_config.spawn.wanderTargetMargin)) + (int)g_config.spawn.wanderTargetOffset)};
        triggerHonkLocal(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
    } else {
        fprintf(stderr, "[FETCH] handleFetching g%d heldItem=null, -> WANDER\n", g.id);
        g.state = GooseState::WANDER;
        g.PickNewTarget(w, h);
    }
}

void handleReturning(Goose& g, double time, int w, int h) {
    fprintf(stderr, "[FETCH] handleReturning g%d called heldItem=%p\n", g.id, (void*)g.heldItem);
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

            new DroppedItemActor(drop);
            fprintf(stderr, "[FETCH] handleReturning g%d dropped item at (%.0f,%.0f) rot=%.1f\n",
                    g.id, drop.pos.x, drop.pos.y, drop.rotation);
        } else {
            fprintf(stderr, "[FETCH] handleReturning g%d DISCARDING item (non-finite pos: %.1f,%.1f rot:%.1f)\n",
                    g.id, drop.pos.x, drop.pos.y, drop.rotation);
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
    fprintf(stderr, "[FETCH] handleReturning g%d -> WANDER lastDrop=%.1f\n", g.id, g.lastDropTime);
    triggerHonkLocal(g.honkState, time, g_config.honk.fetchCooldown, g.honkState.lastFetch);
}
