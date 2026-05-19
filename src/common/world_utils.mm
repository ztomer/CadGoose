// ===========================
// world_utils.cpp
// World maintenance utilities
// ===========================
#include "world.h"
#include "config.h"
#include "actor.h"
#include "actor_leafpile.h"

#ifdef __APPLE__
#import <AppKit/AppKit.h>
#endif

static constexpr float kLeafPileExpiry = 10.0f;
static constexpr int kMaxLeafPiles = 10;
static constexpr float kLeafPileSizeMin = 50.0f;
static constexpr float kLeafPileSizeMax = 100.0f;

void World_CleanupExpired(double currentTime) {
    g_world.droppedItems.remove_if([&](DroppedItem& i) {
        float lifetime = g_config.item.itemLifetime;
        if (i.data->type == ItemData::MEME) lifetime = g_config.item.memeLifetime;
        else if (i.data->type == ItemData::TEXT) lifetime = g_config.item.textLifetime;

        bool exp = !i.pinned && ((currentTime - i.timeDropped) > lifetime);
        if (exp) delete i.data;
        return exp;
    });

    int memeCount = 0, textCount = 0;
    for (const auto& item : g_world.droppedItems) {
        if (item.data->type == ItemData::MEME) memeCount++;
        else if (item.data->type == ItemData::TEXT) textCount++;
    }

    if (memeCount > g_config.item.maxDroppedMemes) {
        int toRemove = memeCount - g_config.item.maxDroppedMemes;
        for (auto it = g_world.droppedItems.begin(); it != g_world.droppedItems.end() && toRemove > 0; ) {
            if (it->data->type == ItemData::MEME && !it->pinned) {
                delete it->data;
                it = g_world.droppedItems.erase(it);
                toRemove--;
            } else {
                ++it;
            }
        }
    }

    if (textCount > g_config.item.maxDroppedTexts) {
        int toRemove = textCount - g_config.item.maxDroppedTexts;
        for (auto it = g_world.droppedItems.begin(); it != g_world.droppedItems.end() && toRemove > 0; ) {
            if (it->data->type == ItemData::TEXT && !it->pinned) {
                delete it->data;
                it = g_world.droppedItems.erase(it);
                toRemove--;
            } else {
                ++it;
            }
        }
    }

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
        if (nearestGoose && Vector2::Distance(nearestGoose->pos, pile->position.toVector2()) < pile->radius + g_config.render.footSize) {
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
        (float)(rand() % (int)std::max(1.0f, screenWidth)),
        (float)(rand() % (int)std::max(1.0f, screenHeight))
    };
    float radius = kLeafPileSizeMin + (rand() % (int)(kLeafPileSizeMax - kLeafPileSizeMin));
    LeafPileActor* pile = new LeafPileActor(pos, radius, kLeafPileSizeMax, currentTime);
    mgr.add(pile);
}

bool ItemHitTest(NSPoint p, float viewHeight, DroppedItem** hitItem, float closeButtonSize) {
    // p is in VIEW coords (isFlipped=YES → same as DEVICE coords)
    DevicePoint mousePt = {(float)p.x, (float)p.y};

    for (auto it = g_world.droppedItems.rbegin(); it != g_world.droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
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
    delete item->data;
    for (auto it = g_world.droppedItems.begin(); it != g_world.droppedItems.end(); ++it) {
        if (&(*it) == item) {
            g_world.droppedItems.erase(it);
            break;
        }
    }
}

void MoveItemToFront(DroppedItem* item) {
    for (auto it = g_world.droppedItems.begin(); it != g_world.droppedItems.end(); ++it) {
        if (&(*it) == item) {
            g_world.droppedItems.splice(g_world.droppedItems.end(), g_world.droppedItems, it);
            break;
        }
    }
}

bool ShouldAcceptMouseEvents() {
    return !g_world.droppedItems.empty();
}

bool Config_IsSystemDarkTheme() {
#ifdef __APPLE__
    return [[[NSApplication sharedApplication] effectiveAppearance] name] == NSAppearanceNameDarkAqua;
#else
    return false;
#endif
}