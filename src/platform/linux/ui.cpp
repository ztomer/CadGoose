#include "ui_drawing.h"
#include "ui_tick.h"
#include "ui_debug.h"
// ui.cpp
#include "ui.h"
#include "ui_controls.h"
#include "ui_factory.h"
#include "ui_escape.h"
#include "world.h"
#include "config.h"
#include "assets.h"
#include "goose.h"
#include "actor.h"
#include "actor_dropped_item.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <string>
#include <pango/pangocairo.h>
#include <chrono>
#include "cursor_io.h"
#include "ram_tracker.h"

namespace fs = std::filesystem;

// --- Input region constants ---
static constexpr int kGooseInputRegionMargin = 100;
static constexpr int kGooseInputRegionPad = 30;
static constexpr int kGooseInputRegionSize = 60;
static constexpr int kItemInputRegionMargin = 500;

// --- Footprint constants ---
static constexpr float kFootprintAlpha = 0.5f;
static constexpr float kFootprintPadR = 0.45f;
static constexpr float kFootprintPadG = 0.25f;
static constexpr float kFootprintPadB = 0.1f;
static constexpr float kFootprintPadScaleX = 6;
static constexpr float kFootprintPadScaleY = 8;
static constexpr float kFootprintToeScaleX = 3;
static constexpr float kFootprintToeScaleY = 5;
static constexpr float kFootprintToeOffsetX = 3;
static constexpr float kFootprintToeOffsetY = 5;

// --- Debug overlay constants ---
static constexpr float kDebugOrangeR = 1.0f;
static constexpr float kDebugOrangeG = 0.5f;
static constexpr float kDebugOrangeB = 0.0f;
static constexpr float kDebugOrangeAlpha = 0.8f;
static constexpr float kDebugPointRadius = 4;
static constexpr int kDebugCircleRadius = 40;
static constexpr float kDebugIdLabelOffsetX = 15;
static constexpr float kDebugIdLabelOffsetY = -25;
static constexpr float kDebugDashPattern1 = 4.0;
static constexpr float kDebugDashPattern2 = 2.0;
static constexpr float kDebugTargetDotRadius = 6;
static constexpr float kSimDt = 0.1f;
static constexpr int kSimSteps = 15;
static constexpr float kSimBreakDist = 10.0f;
static constexpr float kSimMaxDist = 2000.0f;
static constexpr float kTickDt = 1.0f / 60.0f;
static constexpr float kLinuxFootprintFadeStart = 0.7f;
static constexpr float kLinuxTextPadding = 10;
static constexpr float kLinuxNametagYOffset = 75.0f;
static constexpr float kLinuxNametagPadX = 4;
static constexpr float kLinuxNametagPadY = 2;
static constexpr float kLinuxNametagPadW = 8;
static constexpr float kLinuxNametagPadH = 4;
static constexpr float kLinuxInfoLabelOffsetY = -10;
static constexpr float kLinuxBeakTipRadius = 4;

// --- Input region ------------------------------------------------------------
// Make overlay click-through except where geese/items are drawn.
// Now handles drawing relative to the specific monitor this window belongs to.
void UpdateInputRegion(GtkWindow* window, const MonitorInfo& m) {
    cairo_region_t* region = cairo_region_create();

    // Respect multi-monitor toggle for input as well
    if (!g_config.cursor.multiMonitorEnabled && (m.x != 0 || m.y != 0)) {
        GdkSurface* s = gtk_native_get_surface(GTK_NATIVE(window));
        if (s) gdk_surface_set_input_region(s, region);
        cairo_region_destroy(region);
        return;
    }

    for (auto* goose : ActorManager::Instance().getGeese()) {
        // Calculate relative position to this monitor
        float localX = goose->pos.x - m.x;
        float localY = goose->pos.y - m.y;
        
        // Only add to region if it's within/near this monitor's bounds
        if (localX > -kGooseInputRegionMargin && localX < m.width + kGooseInputRegionMargin && localY > -kGooseInputRegionMargin && localY < m.height + kGooseInputRegionMargin) {
            cairo_rectangle_int_t r = { (int)localX - kGooseInputRegionPad, (int)localY - kGooseInputRegionPad, kGooseInputRegionSize, kGooseInputRegionSize };
            cairo_region_union_rectangle(region, &r);
        }
    }

    auto items = ActorManager::Instance().getDroppedItems();
    for (auto* actor : items) {
        DroppedItem& item = actor->item();
        float localX = item.pos.x - m.x;
        float localY = item.pos.y - m.y;
        if (localX > -kItemInputRegionMargin && localX < m.width + kItemInputRegionMargin && localY > -kItemInputRegionMargin && localY < m.height + kItemInputRegionMargin) {
            cairo_rectangle_int_t r = { (int)localX, (int)localY, (int)(item.data->w * g_config.general.globalScale), (int)(item.data->h * g_config.general.globalScale) };
            cairo_region_union_rectangle(region, &r);
        }
    }

    GdkSurface* s = gtk_native_get_surface(GTK_NATIVE(window));
    if (s) {
        gdk_surface_set_input_region(s, region);
    }
    cairo_region_destroy(region);
}

void setup_overlay_window(GtkApplication* app) {
    g_uiApp = app;
    ASSET_ROOT = fs::current_path() / ASSET_ROOT_NAME;
    if (!fs::exists(ASSET_ROOT))
        ASSET_ROOT = fs::canonical("/proc/self/exe").parent_path() / ASSET_ROOT_NAME;

    g_assets.Init();

    // Multi-monitor detection
    GdkDisplay* display = gdk_display_get_default();
    GListModel* monitors = gdk_display_get_monitors(display);
    
    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    g_world.monitors.clear();
    g_world.overlayCanvases.clear();

    for (unsigned int i = 0; i < g_list_model_get_n_items(monitors); i++) {
        GdkMonitor* monitor = (GdkMonitor*)g_list_model_get_item(monitors, i);
        GdkRectangle geom;
        gdk_monitor_get_geometry(monitor, &geom);
        
        MonitorInfo mi;
        mi.x = geom.x;
        mi.y = geom.y;
        mi.width = geom.width;
        mi.height = geom.height;
        mi.monitor = monitor;
        g_world.monitors.push_back(mi);

        if (mi.x < minX) minX = mi.x;
        if (mi.y < minY) minY = mi.y;
        if (mi.x + mi.width > maxX) maxX = mi.x + mi.width;
        if (mi.y + mi.height > maxY) maxY = mi.y + mi.height;

        // Create overlay for this monitor
        GtkWindow* overlay = GTK_WINDOW(gtk_application_window_new(app));
        gtk_layer_init_for_window(overlay);
        gtk_layer_set_monitor(overlay, monitor);
        gtk_layer_set_layer(overlay, GTK_LAYER_SHELL_LAYER_OVERLAY);
        gtk_layer_set_anchor(overlay, GTK_LAYER_SHELL_EDGE_TOP, 1);
        gtk_layer_set_anchor(overlay, GTK_LAYER_SHELL_EDGE_BOTTOM, 1);
        gtk_layer_set_anchor(overlay, GTK_LAYER_SHELL_EDGE_LEFT, 1);
        gtk_layer_set_anchor(overlay, GTK_LAYER_SHELL_EDGE_RIGHT, 1);
        gtk_layer_set_keyboard_mode(overlay, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);

        GtkWidget* canvas = gtk_drawing_area_new();
        // Pass the MonitorInfo reference
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(canvas), draw_overlay, &g_world.monitors.back(), NULL);
        gtk_window_set_child(overlay, canvas);
        g_world.overlayCanvases.push_back(canvas);

        // Store monitor index in canvas for on_tick to use
        int monitorIndex = (int)g_world.monitors.size() - 1;
        g_object_set_data(G_OBJECT(canvas), "monitor-index", GINT_TO_POINTER(monitorIndex));

        g_timeout_add(16, on_tick, canvas);
        gtk_window_present(overlay);
    }

    // Set globally unified screen dimensions.
    // If multi-monitor is disabled, clamp world bounds to the primary monitor
    // so geese/cursor logic won't drift into hidden monitors and crash at edges.
    if (!g_config.cursor.multiMonitorEnabled && !g_world.monitors.empty()) {
        const MonitorInfo& primary = g_world.monitors.front();
        g_world.screenWidth = primary.width;
        g_world.screenHeight = primary.height;
    } else {
        g_world.screenWidth = maxX;
        g_world.screenHeight = maxY;
    }
    
    // Global style for transparent windows
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, "window { background: transparent; }");
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css), 800);
    // Initialize RAM tracker (pushes samples to the in-game UI log once per second)
    RamTracker_Init();
}
