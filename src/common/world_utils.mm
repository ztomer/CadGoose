// ===========================
// world_utils.cpp
// World maintenance utilities
// ===========================
#include "world.h"
#include "config.h"

#ifdef __APPLE__
#import <AppKit/AppKit.h>
#endif

void World_CleanupExpired(double currentTime) {
    g_droppedItems.remove_if([&](DroppedItem& i) {
        bool exp = i.isExpired(currentTime);
        if (exp) delete i.data;
        return exp;
    });

    while (!g_footprints.empty()) {
        Footprint& fp = g_footprints.front();
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        if ((currentTime - fp.timeSpawned) > life) {
            g_footprints.pop();
        } else {
            break;
        }
    }

    g_leafPiles.remove_if([&](const LeafPile& p) {
        return (p.timeSinceKicked > 0.0f && currentTime - p.timeSinceKicked > 10.0f);
    });
}

void World_TickLeafPiles(double currentTime, float dt, Goose* nearestGoose) {
    if (!g_config.behaviors.fun.autumnLeaves) return;
    for (auto& pile : g_leafPiles) {
        pile.Tick(nearestGoose, currentTime, dt);
    }
}

void World_SpawnRandomLeafPile(float screenWidth, float screenHeight, double currentTime) {
    if (!g_config.behaviors.fun.autumnLeaves) return;
    if (g_leafPiles.size() >= 10) return;
    LeafPile pile;
    pile.Init(Vector2{
        (float)(rand() % (int)std::max(1.0f, screenWidth)),
        (float)(rand() % (int)std::max(1.0f, screenHeight))
    }, 50.0f, 100.0f, currentTime);
    g_leafPiles.push_back(pile);
}

bool ItemHitTest(NSPoint p, float viewHeight, DroppedItem** hitItem, float closeButtonSize) {
    float worldX = p.x;
    float worldY = viewHeight - p.y;

    for (auto it = g_droppedItems.rbegin(); it != g_droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
        float cx = item.pos.x;
        float cy = item.pos.y;
        float dx = worldX - cx;
        float dy = worldY - cy;
        float cosA = cos(item.rotation);
        float sinA = sin(item.rotation);
        float lx = dx * cosA - dy * sinA;
        float ly = dx * sinA + dy * cosA;
        if (lx >= -item.data->w/2.0f && lx <= item.data->w/2.0f &&
            ly >= -item.data->h/2.0f && ly <= item.data->h/2.0f) {
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
    for (auto it = g_droppedItems.begin(); it != g_droppedItems.end(); ++it) {
        if (&(*it) == item) {
            g_droppedItems.erase(it);
            break;
        }
    }
}

void MoveItemToFront(DroppedItem* item) {
    for (auto it = g_droppedItems.begin(); it != g_droppedItems.end(); ++it) {
        if (&(*it) == item) {
            g_droppedItems.splice(g_droppedItems.end(), g_droppedItems, it);
            break;
        }
    }
}

bool ShouldAcceptMouseEvents() {
    return !g_droppedItems.empty();
}

bool Config_IsSystemDarkTheme() {
#ifdef __APPLE__
    return [[[NSApplication sharedApplication] effectiveAppearance] name] == NSAppearanceNameDarkAqua;
#else
    return false;
#endif
}