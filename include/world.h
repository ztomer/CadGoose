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

void UiLogPush(const std::string& s);
Goose* GetGooseById(int id);

// WorldCoord transforms used to live here; they're now in world_coord.h.
// Re-include for backward compatibility so existing translation units
// don't have to update their includes.
#include "world_coord.h"

#endif // WORLD_H
