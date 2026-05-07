// ===========================
// world.h
// ===========================
#ifndef WORLD_H
#define WORLD_H

#include <list>
#include <string>
#include <deque>
#include <vector>

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
extern std::list<Footprint> g_footprints;
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

class WorldCoord {
public:
    static Vector2 ToDevice(const Vector2& worldPos, const Vector2& goosePos, float globalScale) {
        return goosePos + (worldPos - goosePos) * globalScale;
    }

    static Vector2 ToDevice(const Vector2& worldPos, const Goose& goose) {
        return ToDevice(worldPos, goose.pos, g_config.general.globalScale);
    }

    static Vector2 GoosePos(const Goose& goose) {
        return ToDevice(goose.pos, goose.pos, g_config.general.globalScale);
    }

    static Vector2 RigNeckHead(const Goose& goose) {
        return ToDevice(goose.rig.neckHead, goose.pos, g_config.general.globalScale);
    }

    static Vector2 RigBody(const Goose& goose) {
        return ToDevice(goose.rig.body, goose.pos, g_config.general.globalScale);
    }

    static Vector2 ToDevice(const Vector2& worldPos) {
        return worldPos * g_config.general.globalScale;
    }

    static Vector2 DeviceSize(int pixelSize) {
        return Vector2{pixelSize * g_config.general.globalScale, pixelSize * g_config.general.globalScale};
    }

    static float Scale(float worldValue) {
        return worldValue * g_config.general.globalScale;
    }

    static Vector2 ItemCenter(const DroppedItem& item) {
        return item.pos + DeviceSize(item.data->w) * 0.5f;
    }

    static Vector2 ItemHalfSize(const ItemData* item) {
        return DeviceSize(item->w) * 0.5f;
    }

    static Vector2 FromScreen(const Vector2& screenPos, float viewHeight) {
        return Vector2{screenPos.x, viewHeight - screenPos.y};
    }

    static Vector2 ToScreen(const Vector2& worldPos, float viewHeight) {
        return Vector2{worldPos.x, viewHeight - worldPos.y};
    }
};

void UiLogPush(const std::string& s);
Goose* GetGooseById(int id);

#endif // WORLD_H
