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

struct Leaf {
    Vector2 curPosPlanar;
    float curPosZ;
    Vector2 velPlanar;
    float velZ;
    int colorIndex;
    
    Vector2 GetScreenOffset(float zScale = 1.0f) const {
        return curPosPlanar + Vector2{0.0f, -curPosZ * zScale * 0.6f};
    }
};

struct LeafPile {
    Vector2 pos;
    float rad;
    float timeSinceKicked = -1.0f;
    float timeCreated = 0.0f;
    std::vector<Leaf> leaves;
    
    void Init(Vector2 position, float radius, float height, double currentTime);
    void Kick(Vector2 kickVelocity, double currentTime, float gooseSpeedPercentage);
    void Tick(Goose* g, double currentTime, float dt);
};

extern std::list<Goose> g_geese;
extern std::list<MonitorInfo> g_monitors;
extern std::list<DroppedItem> g_droppedItems;
constexpr size_t kMaxFootprints = 500;
extern RingBuffer<Footprint, kMaxFootprints> g_footprints;
extern std::list<LeafPile> g_leafPiles;
extern int g_nextId;
extern int g_screenWidth;
extern int g_screenHeight;
extern int g_selectedGooseId;

#ifdef __linux__
extern GtkWidget* g_entryNote;
extern std::vector<GtkWidget*> g_overlayCanvases;
#endif

extern std::deque<std::string> g_uiLog;
extern int g_cursorGrabberId; // id of goose currently dragging the cursor, -1 = none
extern int g_frameId;          // incrementing frame counter for duplicate update guard

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

#endif // WORLD_H
