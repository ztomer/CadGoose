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

constexpr size_t kMaxFootprints = 500;
constexpr size_t kMaxCrumbs = 200;
constexpr size_t kMaxJails = 10;

struct WorldContext {
    // Geese are now owned by ActorManager — query via ActorManager::Instance().getGeese()
    std::list<MonitorInfo> monitors;
    std::list<DroppedItem> droppedItems;
    RingBuffer<Footprint, kMaxFootprints> footprints;
    RingBuffer<Crumbs, kMaxCrumbs> crumbs;
    std::list<LeafPile> leafPiles;
    
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


struct InteractivePuddle {
    Vector2 pos{0, 0};
    Vector2 vel{0, 0};
    float radius = 15.0f;
    double spawnTime = 0;
    bool splashed = false;
    float maxRadius = 40.0f;
    float alpha = 0.6f;
};

struct InteractiveFlower {
    Vector2 pos{0, 0};
    double spawnTime = 0;
    float growth = 0.0f;
    float stemHeight = 0.0f;
    float petalSize = 0.0f;
    float hue = 0.0f;
};

struct Toy {
    Vector2 pos{0, 0};
    float angle = 0;
    double time{0};
    bool active = false;
    enum class Type : int { Stick = 0, Ball = 1 } type = Type::Stick;
};

#endif // WORLD_H
