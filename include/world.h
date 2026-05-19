// ===========================
// world.h
// ===========================
#ifndef WORLD_H
#define WORLD_H

#include <list>
#include <string>
#include <deque>
#include <vector>

#include "ring_buffer.h"
#include "coordinate_system.h"

#ifdef __linux__
#include <gtk/gtk.h>
#endif

#include "config.h"
#include "goose.h"
#include "items.h"

struct MonitorInfo {
    int x, y, width, height;
#ifdef __APPLE__
    void* monitor;
#elif defined(__linux__)
    GdkMonitor* monitor;
#endif
};

struct Footprint {
    Vector2 pos;
    float dir;
    double timeSpawned;
    float lifetime;
};

struct Crumbs {
    Vector2 pos;
    double time;
    float lifetime;
    bool eaten = false;
};

// Leaf / LeafPile types now live in actor_leafpile.h (LeafParticle) — the
// LeafPileActor owns particle simulation; world.h no longer carries the legacy
// value-type Leaf / LeafPile.

constexpr size_t kMaxFootprints = 500;
constexpr size_t kMaxCrumbs = 200;
constexpr size_t kMaxJails = 10;

struct WorldContext {
    // Geese are now owned by ActorManager — query via ActorManager::Instance().getGeese()
    std::list<MonitorInfo> monitors;
    std::list<DroppedItem> droppedItems;
    RingBuffer<Footprint, kMaxFootprints> footprints;
    RingBuffer<Crumbs, kMaxCrumbs> crumbs;
    // LeafPiles are owned by ActorManager (LeafPileActor) — query via countByType("leafpile") etc.


    int nextId = 0;
    int screenWidth = 0;
    int screenHeight = 0;
    int selectedGooseId = 0;
    int cursorGrabberId = -1;
    int frameId = 0;

    std::deque<std::string> uiLog;

#ifdef __linux__
    GtkWidget* entryNote = nullptr;
    std::vector<GtkWidget*> overlayCanvases;
#endif
};

extern WorldContext g_world;

// ============================================================
// WorldCoord — Coordinate transformation helpers
// ============================================================
// All methods use typed coordinates to prevent space mixing bugs.
// See coordinate_system.h for full documentation.

class WorldCoord {
public:
    // goose-local world coords → device coords
    static DevicePoint WorldToDevice(WorldPoint worldPos, DevicePoint goosePos, float globalScale) {
        return CoordTransform::WorldToDevice(worldPos, goosePos, globalScale);
    }

    static DevicePoint WorldToDevice(WorldPoint worldPos, const Goose& goose) {
        return CoordTransform::WorldToDevice(worldPos, DevicePoint{goose.pos.x, goose.pos.y}, g_config.general.globalScale);
    }

    // origin-relative world coords → device coords
    static DevicePoint OriginToDevice(WorldPoint worldPos) {
        return {worldPos.x * g_config.general.globalScale, worldPos.y * g_config.general.globalScale};
    }

    // rig parts are in goose-local world space
    static DevicePoint RigNeckHead(const Goose& goose) {
        return WorldToDevice(WorldPoint{goose.rig.neckHead.x, goose.rig.neckHead.y}, goose);
    }

    static DevicePoint RigBody(const Goose& goose) {
        return WorldToDevice(WorldPoint{goose.rig.body.x, goose.rig.body.y}, goose);
    }

    // item coordinate helpers (all return DEVICE coords)
    static DevicePoint ItemCenter(const DroppedItem& item) {
        return ItemCoords::Center({item.pos.x, item.pos.y}, item.data->w, item.data->h, g_config.general.globalScale);
    }

    static DevicePoint ItemHalfSize(const ItemData* item) {
        return ItemCoords::HalfSize(item->w, item->h, g_config.general.globalScale);
    }

    static DevicePoint ItemSize(const ItemData* item) {
        return ItemCoords::Size(item->w, item->h, g_config.general.globalScale);
    }

    // scalar scaling
    static float Scale(float worldValue) {
        return CoordTransform::Scale(worldValue, g_config.general.globalScale);
    }

    // Y-flip helpers for Linux Cairo
    static DevicePoint FromCairo(float cairoX, float cairoY, float screenHeight) {
        return CoordTransform::CairoToDevice(cairoX, cairoY, screenHeight);
    }

    static Vector2 ToCairo(DevicePoint devicePos, float screenHeight) {
        return CoordTransform::DeviceToCairo(devicePos, screenHeight);
    }
};

void UiLogPush(const std::string& s);
Goose* GetGooseById(int id);

// Pomodoro bed accessor (defined in behavior_pomodoro.cpp)
struct PomodoroBedInfo {
    Vector2 position;
    bool visible;
    void* bedImage; // CGImageRef
};
PomodoroBedInfo Pomodoro_GetBedInfo(int gooseId);


#endif // WORLD_H
