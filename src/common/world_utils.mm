// ===========================
// world_utils.cpp
// World maintenance utilities
// ===========================
#include "world.h"
#include "random_util.h"
#include "config.h"
#include "actor.h"
#include "actor_dropped_item.h"
#include "actor_leafpile.h"

#ifdef __APPLE__
#import <AppKit/AppKit.h>
#endif

static constexpr float kLeafPileExpiry = 10.0f;
static constexpr int kMaxLeafPiles = 10;
static constexpr float kLeafPileSizeMin = 50.0f;
static constexpr float kLeafPileSizeMax = 100.0f;

void World_CleanupExpired(double currentTime) {
    // DroppedItemActor::isAlive() checks isExpired(), so cleanup() removes them
    auto& mgr = ActorManager::Instance();

    // Enforce max meme/text counts before cleanup
    int memeCount = 0, textCount = 0;
    auto items = mgr.getDroppedItems();
    for (auto* actor : items) {
        if (actor->data()->type == ItemData::MEME) memeCount++;
        else if (actor->data()->type == ItemData::TEXT) textCount++;
    }

    if (memeCount > g_config.item.maxDroppedMemes) {
        int toRemove = memeCount - g_config.item.maxDroppedMemes;
        for (auto* actor : items) {
            if (toRemove <= 0) break;
            if (actor->data()->type == ItemData::MEME && !actor->pinned()) {
                mgr.remove(actor);
                toRemove--;
            }
        }
    }

    if (textCount > g_config.item.maxDroppedTexts) {
        int toRemove = textCount - g_config.item.maxDroppedTexts;
        for (auto* actor : items) {
            if (toRemove <= 0) break;
            if (actor->data()->type == ItemData::TEXT && !actor->pinned()) {
                mgr.remove(actor);
                toRemove--;
            }
        }
    }

    mgr.cleanup();

    while (!g_world.footprints.empty()) {
        Footprint& fp = g_world.footprints.front();
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        if ((currentTime - fp.timeSpawned) > life) {
            g_world.footprints.pop();
        } else {
            break;
        }
    }

    // Leaf piles are now managed by ActorManager — cleanup handled there
}

void World_TickLeafPiles(double currentTime, float dt, Goose* nearestGoose) {
    if (!g_config.behaviors.fun.autumnLeaves) return;
    (void)dt; (void)currentTime; // tick is done via ActorManager::tickAll

    auto& mgr = ActorManager::Instance();
    for (int i = 0; i < mgr.totalCount(); i++) {
        Actor* a = mgr.getByIndex(i);
        if (!a || strcmp(a->type(), "leafpile") != 0) continue;

        LeafPileActor* pile = static_cast<LeafPileActor*>(a);
        if (!pile->isAlive()) continue;

        // Check if goose is near to kick the pile (the per-frame leaf-particle
        // simulation itself runs from the actor's tick via ActorManager::tickAll).
        if (nearestGoose && Vector2::Distance(nearestGoose->pos, pile->position().toVector2()) < pile->radius() + g_config.render.footSize) {
            float walkSpeed = g_config.movement.baseWalkSpeed;
            float chargeSpeed = g_config.movement.baseRunSpeed;
            float currentSpeed = nearestGoose->currentSpeed;
            float gooseSpeedPercentage = (currentSpeed - walkSpeed) / (chargeSpeed - walkSpeed);
            if (gooseSpeedPercentage < 0.0f) gooseSpeedPercentage = 0.0f;
            if (gooseSpeedPercentage > 1.0f) gooseSpeedPercentage = 1.0f;
            pile->kick(nearestGoose->vel, currentTime, gooseSpeedPercentage);
        }
    }
}

void World_SpawnRandomLeafPile(float screenWidth, float screenHeight, double currentTime) {
    if (!g_config.behaviors.fun.autumnLeaves) return;

    auto& mgr = ActorManager::Instance();
    int activeCount = mgr.countByType("leafpile");
    if (activeCount >= kMaxLeafPiles) return;

    Vector2 pos{
        (float)(rng_util::RandRange((int)std::max(1.0f, screenWidth))),
        (float)(rng_util::RandRange((int)std::max(1.0f, screenHeight)))
    };
    float radius = kLeafPileSizeMin + (rng_util::RandRange((int)(kLeafPileSizeMax - kLeafPileSizeMin)));
    LeafPileActor* pile = new LeafPileActor(pos, radius, kLeafPileSizeMax, currentTime);
    mgr.add(pile);
}

bool ItemHitTest(NSPoint p, float viewHeight, DroppedItem** hitItem, float closeButtonSize) {
    // p is in VIEW coords (isFlipped=YES → same as DEVICE coords)
    DevicePoint mousePt = {(float)p.x, (float)p.y};

    auto& mgr = ActorManager::Instance();
    auto items = mgr.getDroppedItems();
    // Iterate in reverse for z-order (last added = on top)
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        DroppedItemActor* actor = *it;
        DroppedItem& item = actor->item();
        DevicePoint itemPos = {item.pos.x, item.pos.y};
        if (HitTest::PointInItem(mousePt, itemPos, item.data->w, item.data->h, item.rotation, g_config.general.globalScale)) {
            *hitItem = &item;
            return true;
        }
    }
    return false;
}

bool CheckCloseButton(float lx, float ly, float w, float h, float closeButtonSize) {
    float closeX = -w/2.0f;
    float closeY = -h/2.0f;
    return lx >= closeX && lx <= closeX + closeButtonSize && ly >= closeY && ly <= closeY + closeButtonSize;
}

void DeleteDroppedItem(DroppedItem* item) {
    auto& mgr = ActorManager::Instance();
    auto items = mgr.getDroppedItems();
    for (auto* actor : items) {
        if (&actor->item() == item) {
            mgr.remove(actor);
            break;
        }
    }
}

void MoveItemToFront(DroppedItem* item) {
    // With per-item windows, z-order is controlled by window level, not list order.
    // This function is kept for API compatibility but is now a no-op.
    (void)item;
}

bool ShouldAcceptMouseEvents() {
    return !ActorManager::Instance().getDroppedItems().empty();
}

bool Config_IsSystemDarkTheme() {
#ifdef __APPLE__
    return [[[NSApplication sharedApplication] effectiveAppearance] name] == NSAppearanceNameDarkAqua;
#else
    return false;
#endif
}