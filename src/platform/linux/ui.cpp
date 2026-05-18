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
    if (!g_config.multiMonitorEnabled && (m.x != 0 || m.y != 0)) {
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

    for (auto& item : g_world.droppedItems) {
        float localX = item.pos.x - m.x;
        float localY = item.pos.y - m.y;
        if (localX > -kItemInputRegionMargin && localX < m.width + kItemInputRegionMargin && localY > -kItemInputRegionMargin && localY < m.height + kItemInputRegionMargin) {
            cairo_rectangle_int_t r = { (int)localX, (int)localY, (int)(item.data->w * g_config.globalScale), (int)(item.data->h * g_config.globalScale) };
            cairo_region_union_rectangle(region, &r);
        }
    }

    GdkSurface* s = gtk_native_get_surface(GTK_NATIVE(window));
    if (s) {
        gdk_surface_set_input_region(s, region);
    }
    cairo_region_destroy(region);
}

void draw_overlay(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer data) {
    MonitorInfo* m = (MonitorInfo*)data;
    if (!m || cairo_status(cr) != CAIRO_STATUS_SUCCESS) return;

    if (!g_config.multiMonitorEnabled && (m->x != 0 || m->y != 0)) {
        cairo_save(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr);
        cairo_restore(cr);
        return;
    }

    cairo_save(cr); 
    cairo_translate(cr, -m->x, -m->y);

    DrawFootprints(cr);
    DrawDroppedItems(cr);

    // Draw Geese
    for (auto* g : ActorManager::Instance().getGeese()) {
        if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) break;
        g->Draw(cr);

        // Minecraft-style name tag
        if (!g->name.empty()) {
            cairo_save(cr);
            PangoLayout* layout = pango_cairo_create_layout(cr);
            std::string tagText = "[#" + std::to_string(g->id) + "] " + g->name;
            pango_layout_set_text(layout, tagText.c_str(), -1);
            
            PangoFontDescription* desc = pango_font_description_from_string("Sans Bold 10");
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);

            float tagX = g.pos.x - tw / 2.0f;
            float tagY = g.pos.y - kLinuxNametagYOffset * g_config.globalScale; // Position above head

            // Background rectangle (translucent dark)
            cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
            cairo_rectangle(cr, tagX - kLinuxNametagPadX, tagY - kLinuxNametagPadY, tw + kLinuxNametagPadW, th + kLinuxNametagPadH);
            cairo_fill(cr);

            // White text
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_move_to(cr, tagX, tagY);
            pango_cairo_show_layout(cr, layout);

            g_object_unref(layout);
            cairo_restore(cr);
        }

        // Visual debug: highlight all geese when enabled
        if (g_config.debugVisuals) {
            cairo_save(cr);
            
            // 1. Highlight circle around goose
            cairo_set_source_rgba(cr, 1, 1, 0, 0.15);
            cairo_set_line_width(cr, 2.0);
            cairo_arc(cr, g.pos.x, g.pos.y, kDebugCircleRadius, 0, G_PI * 2);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 1, 1, 0, 0.4);
            cairo_stroke(cr);

            // 2. ID label near goose
            char idBuf[16];
            snprintf(idBuf, sizeof(idBuf), "ID %d", g.id);
            cairo_set_source_rgba(cr, 1, 1, 0, 0.9);
            cairo_move_to(cr, g.pos.x + kDebugIdLabelOffsetX, g.pos.y + kDebugIdLabelOffsetY);
            PangoLayout* idLayout = pango_cairo_create_layout(cr);
            pango_layout_set_text(idLayout, idBuf, -1);
            pango_cairo_show_layout(cr, idLayout);
            g_object_unref(idLayout);

            // 3. Target point and line to target
            // Note: Use btPoint if we're in a beak-targeting state, otherwise pos
            Vector2 origin = g.pos;
            if (g.state == GooseState::FETCHING || g.state == GooseState::RETURNING || g.state == GooseState::CHASE_CURSOR) {
                origin = g.GetBeakTipDevice();
            }

            // Draw line to target (dashed)
            cairo_set_source_rgba(cr, 0, 1, 0, 0.5);
            cairo_set_line_width(cr, 1.5);
            double dashes[] = {kDebugDashPattern1, kDebugDashPattern2};
            cairo_set_dash(cr, dashes, 2, 0);
            cairo_move_to(cr, origin.x, origin.y);
            cairo_line_to(cr, g.target.x, g.target.y);
            cairo_stroke(cr);
            cairo_set_dash(cr, NULL, 0, 0); // reset dash

            // Draw target dot
            cairo_set_source_rgba(cr, 0, 1, 0, 0.8);
            if (g.state == GooseState::RETURNING) cairo_set_source_rgba(cr, 1, 0, 1, 0.8); // Purple for drop point
            if (g.state == GooseState::FETCHING) cairo_set_source_rgba(cr, 0, 1, 1, 0.8);  // Cyan for fetch point
            
            cairo_arc(cr, g.target.x, g.target.y, kDebugTargetDotRadius, 0, G_PI * 2);
            cairo_fill(cr);

            // 4. Threshold circle (Drop Zone / Catch Zone)
            float threshold = std::max(30.0f * g_config.globalScale, 25.0f);
            if (g.state == GooseState::RETURNING) threshold = std::max(50.0f * g_config.globalScale, 40.0f);
            if (g.state == GooseState::CHASE_CURSOR) threshold = std::max(22.0f * g_config.globalScale, 15.0f);

            cairo_set_source_rgba(cr, 1, 1, 1, 0.2); // Faint white circle
            cairo_set_line_width(cr, 1.0);
            cairo_arc(cr, g.target.x, g.target.y, threshold, 0, G_PI * 2);
            cairo_stroke(cr);

            // 5. Text info under ID
            const char* stateName = "UNKNOWN";
            switch(g.state) {
                case WANDER: stateName = "WANDER"; break;
                case FETCHING: stateName = "FETCHING"; break;
                case RETURNING: stateName = "RETURNING"; break;
                case CHASE_CURSOR: stateName = "CHASE"; break;
                case SNATCH_CURSOR: stateName = "SNATCH"; break;
            }
            float dist = Vector2::Distance(origin, g.target);
            char infoBuf[64];
            snprintf(infoBuf, sizeof(infoBuf), "%s (d:%.0f/%.0f)", stateName, dist, threshold);
            
            cairo_set_source_rgba(cr, 1, 1, 1, 0.7);
            cairo_move_to(cr, g.pos.x + 15, g.pos.y - 10);
            PangoLayout* infoLayout = pango_cairo_create_layout(cr);
            pango_layout_set_text(infoLayout, infoBuf, -1);
            pango_cairo_show_layout(cr, infoLayout);
            g_object_unref(infoLayout);

            // 6. Draw Beak Tip (btPoint) clearly
            Vector2 bt = g.GetBeakTipDevice();
            cairo_set_source_rgba(cr, kDebugOrangeR, kDebugOrangeG, kDebugOrangeB, kDebugOrangeAlpha);
            cairo_arc(cr, bt.x, bt.y, 4, 0, G_PI * 2);
            cairo_fill(cr);

            // Enhanced Path Visualization: Predictive Curve Simulation
            Vector2 simPos = origin;
            Vector2 simVel = (Vector2::Length(g.vel) < 1.0f) ? (Vector2::Normalize(g.target - origin) * g.currentSpeed) : g.vel;
            float simDt = kSimDt; // Simulation step
            
            cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
            for (int i = 0; i < kSimSteps; i++) {
                Vector2 toTarget = g.target - simPos;
                float d = Vector2::Length(toTarget);
                if (d < kSimBreakDist) break;

                Vector2 desired = Vector2::Normalize(toTarget) * g.currentSpeed;
                Vector2 seek = (desired - simVel) * 2.0f;
                
                Vector2 tangent{ -Vector2::Normalize(simVel).y, Vector2::Normalize(simVel).x };
                float curveFade = std::min(1.0f, d / 200.0f);
                Vector2 curve = tangent * (g.parabolicCurvature * g.currentSpeed * 0.8f * curveFade);
                
                Vector2 force = seek + curve;
                simVel = simVel + force * simDt;
                // Limit speed
                float s = Vector2::Length(simVel);
                if (s > g.currentSpeed) simVel = simVel * (g.currentSpeed / s);
                
                Vector2 nextSimPos = simPos + simVel * simDt;
                cairo_move_to(cr, simPos.x, simPos.y);
                cairo_line_to(cr, nextSimPos.x, nextSimPos.y);
                cairo_stroke(cr);
                
                simPos = nextSimPos;
                
                // Avoid infinite simulation if something goes wrong
                if (Vector2::Distance(simPos, origin) > kSimMaxDist) break;
            }

            cairo_restore(cr);
        }
    }

    cairo_restore(cr); 

    // Debug overlay (only on the primary monitor window)
    if (m->x == 0 && m->y == 0) {
        draw_debug_overlay(cr);
    }
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

        g_timeout_add(16, on_tick, canvas);
        gtk_window_present(overlay);
    }

    // Set globally unified screen dimensions.
    // If multi-monitor is disabled, clamp world bounds to the primary monitor
    // so geese/cursor logic won't drift into hidden monitors and crash at edges.
    if (!g_config.multiMonitorEnabled && !g_world.monitors.empty()) {
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
