// ui.cpp
#include "ui.h"
#include "ui_controls.h"
#include "ui_factory.h"
#include "ui_escape.h"
#include "world.h"
#include "config.h"
#include "assets.h"
#include "goose.h"
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

    for (auto& goose : g_geese) {
        // Calculate relative position to this monitor
        float localX = goose.pos.x - m.x;
        float localY = goose.pos.y - m.y;
        
        // Only add to region if it's within/near this monitor's bounds
        if (localX > -100 && localX < m.width + 100 && localY > -100 && localY < m.height + 100) {
            cairo_rectangle_int_t r = { (int)localX - 30, (int)localY - 30, 60, 60 };
            cairo_region_union_rectangle(region, &r);
        }
    }

    for (auto& item : g_droppedItems) {
        float localX = item.pos.x - m.x;
        float localY = item.pos.y - m.y;
        if (localX > -500 && localX < m.width + 500 && localY > -500 && localY < m.height + 500) {
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

static void DrawFootprints(cairo_t* cr) {
    for (auto& fp : g_footprints) {
        double age = g_time - fp.timeSpawned;
        float alpha = 0.5f;
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mudLifetime;
        float fadeStart = life * 0.7f;
        float fadeDur = life - fadeStart;
        if (age > fadeStart) alpha = 0.5f * (1.0f - (float)(age - fadeStart) / fadeDur);
        if (alpha <= 0) continue;

        cairo_save(cr);
        cairo_translate(cr, fp.pos.x, fp.pos.y);
        cairo_scale(cr, g_config.globalScale, g_config.globalScale);
        cairo_rotate(cr, fp.dir * G_PI / 180.0);
        
        cairo_set_source_rgba(cr, 0.45, 0.25, 0.1, alpha);
        
        cairo_save(cr);
        cairo_scale(cr, 6, 8);
        cairo_arc(cr, 0, 0, 1.0, 0, 2*G_PI);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_translate(cr, -3, -5);
        cairo_scale(cr, 3, 5);
        cairo_arc(cr, 0, 0, 1.0, 0, 2*G_PI);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_translate(cr, 3, -5);
        cairo_scale(cr, 3, 5);
        cairo_arc(cr, 0, 0, 1.0, 0, 2*G_PI);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_restore(cr);
    }
}

static void DrawDroppedItems(cairo_t* cr) {
    for (auto& item : g_droppedItems) {
        if (!std::isfinite(item.pos.x) || !std::isfinite(item.pos.y) || !std::isfinite(item.rotation)) {
            continue; // Skip corrupted items
        }

        cairo_save(cr);
        float s = g_config.globalScale;
        // Center the coordinate system on the item, applying global scale
        cairo_translate(cr, item.pos.x + item.data->w * 0.5f * s, item.pos.y + item.data->h * 0.5f * s);
        cairo_scale(cr, s, s);
        cairo_rotate(cr, item.rotation);
        
        if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
            std::cerr << "[UI] Cairo error after item transform: " << cairo_status_to_string(cairo_status(cr)) << std::endl;
            cairo_restore(cr);
            continue;
        }

        // Draw from top-left relative to scaled center
        cairo_translate(cr, -item.data->w * 0.5f, -item.data->h * 0.5f);

        if (item.data->type == ItemData::MEME && item.data->pixbuf) {
            set_source_pixbuf_manual(cr, item.data->pixbuf, 0, 0);
            cairo_paint(cr);
        } else if (item.data->type == ItemData::TEXT) {
            cairo_set_source_rgb(cr, 1, 1, 0.85);
            cairo_rectangle(cr, 0, 0, item.data->w, item.data->h);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_line_width(cr, 2);
            cairo_stroke(cr);

            PangoLayout* layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(layout, item.data->Text().c_str(), -1);
            pango_layout_set_width(layout, (item.data->w - 10) * PANGO_SCALE);
            cairo_move_to(cr, 5, 5);
            pango_cairo_show_layout(cr, layout);
            g_object_unref(layout);
        }
        cairo_restore(cr);

        // Visual debug for items: draw a point at their center
        if (g_config.debugVisuals) {
            cairo_save(cr);
            Vector2 center = item.pos + Vector2{(float)item.data->w * 0.5f, (float)item.data->h * 0.5f} * s;
            cairo_set_source_rgba(cr, 1, 0.5, 0, 0.8); // Orange point
            cairo_arc(cr, center.x, center.y, 4, 0, G_PI * 2);
            cairo_fill(cr);
            
            // Label for time left
            char timeBuf[16];
            snprintf(timeBuf, sizeof(timeBuf), "%.1fs", 15.0 - (g_time - item.timeDropped));
            cairo_move_to(cr, center.x + 8, center.y + 8);
            PangoLayout* tLayout = pango_cairo_create_layout(cr);
            pango_layout_set_text(tLayout, timeBuf, -1);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.6);
            pango_cairo_show_layout(cr, tLayout);
            g_object_unref(tLayout);
            cairo_restore(cr);
        }
    }
}

// --- Drawing (see ui_drawing.cpp) -----------------------------------------
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
    for (auto& g : g_geese) {
        if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) break;
        g.Draw(cr);

        // Minecraft-style name tag
        if (!g.name.empty()) {
            cairo_save(cr);
            PangoLayout* layout = pango_cairo_create_layout(cr);
            std::string tagText = "[#" + std::to_string(g.id) + "] " + g.name;
            pango_layout_set_text(layout, tagText.c_str(), -1);
            
            PangoFontDescription* desc = pango_font_description_from_string("Sans Bold 10");
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);

            float tagX = g.pos.x - tw / 2.0f;
            float tagY = g.pos.y - 75.0f * g_config.globalScale; // Position above head

            // Background rectangle (translucent dark)
            cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
            cairo_rectangle(cr, tagX - 4, tagY - 2, tw + 8, th + 4);
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
            cairo_arc(cr, g.pos.x, g.pos.y, 40, 0, G_PI * 2);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 1, 1, 0, 0.4);
            cairo_stroke(cr);

            // 2. ID label near goose
            char idBuf[16];
            snprintf(idBuf, sizeof(idBuf), "ID %d", g.id);
            cairo_set_source_rgba(cr, 1, 1, 0, 0.9);
            cairo_move_to(cr, g.pos.x + 15, g.pos.y - 25);
            PangoLayout* idLayout = pango_cairo_create_layout(cr);
            pango_layout_set_text(idLayout, idBuf, -1);
            pango_cairo_show_layout(cr, idLayout);
            g_object_unref(idLayout);

            // 3. Target point and line to target
            // Note: Use btPoint if we're in a beak-targeting state, otherwise pos
            Vector2 origin = g.pos;
            if (g.state = GooseState::= GooseState:: FETCHING || g.state = GooseState::= GooseState:: RETURNING || g.state = GooseState::= GooseState:: CHASE_CURSOR) {
                origin = g.GetBeakTipDevice();
            }

            // Draw line to target (dashed)
            cairo_set_source_rgba(cr, 0, 1, 0, 0.5);
            cairo_set_line_width(cr, 1.5);
            double dashes[] = {4.0, 2.0};
            cairo_set_dash(cr, dashes, 2, 0);
            cairo_move_to(cr, origin.x, origin.y);
            cairo_line_to(cr, g.target.x, g.target.y);
            cairo_stroke(cr);
            cairo_set_dash(cr, NULL, 0, 0); // reset dash

            // Draw target dot
            cairo_set_source_rgba(cr, 0, 1, 0, 0.8);
            if (g.state = GooseState::= GooseState:: RETURNING) cairo_set_source_rgba(cr, 1, 0, 1, 0.8); // Purple for drop point
            if (g.state = GooseState::= GooseState:: FETCHING) cairo_set_source_rgba(cr, 0, 1, 1, 0.8);  // Cyan for fetch point
            
            cairo_arc(cr, g.target.x, g.target.y, 6, 0, G_PI * 2);
            cairo_fill(cr);

            // 4. Threshold circle (Drop Zone / Catch Zone)
            float threshold = std::max(30.0f * g_config.globalScale, 25.0f);
            if (g.state = GooseState::= GooseState:: RETURNING) threshold = std::max(50.0f * g_config.globalScale, 40.0f);
            if (g.state = GooseState::= GooseState:: CHASE_CURSOR) threshold = std::max(22.0f * g_config.globalScale, 15.0f);

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
            cairo_set_source_rgba(cr, 1, 0.5, 0, 0.8);
            cairo_arc(cr, bt.x, bt.y, 4, 0, G_PI * 2);
            cairo_fill(cr);

            // Enhanced Path Visualization: Predictive Curve Simulation
            Vector2 simPos = origin;
            Vector2 simVel = (Vector2::Length(g.vel) < 1.0f) ? (Vector2::Normalize(g.target - origin) * g.currentSpeed) : g.vel;
            float simDt = 0.1f; // Simulation step
            
            cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
            for (int i = 0; i < 15; i++) {
                Vector2 toTarget = g.target - simPos;
                float d = Vector2::Length(toTarget);
                if (d < 10.0f) break;

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
                if (Vector2::Distance(simPos, origin) > 2000.0f) break;
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

gboolean on_tick(gpointer data) {
    g_time += 1.0 / 60.0;
    MaybeTriggerEscapeKill();
    UpdateEscapeHoldHud();

    CursorState cursor = {};
    CursorAction action = {};
    if (g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }

    for (auto& g : g_geese) {
        CursorAction a = g.Update(1.0 / 60.0, g_time, g_screenWidth, g_screenHeight, cursor);
        if (!a.isNone()) action = a;
    }

    if (g_cursorProvider && !action.isNone()) {
        g_cursorProvider->Execute(action);
    }

    g_droppedItems.remove_if([](DroppedItem& i) {
        bool exp = i.isExpired(g_time);
        if (exp) delete i.data;
        return exp;
    });

    g_footprints.remove_if([](Footprint& fp) {
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mudLifetime;
        return (g_time - fp.timeSpawned) > life;
    });

    // We pass a window to setup_overlay, but we need to update ALL overlays.
    // However, on_tick is associated with a specific canvas.
    // To keep it simple, we queue draw on all monitors.
    // This is handled by setup_overlay_window which spawns timers or we can use a global signal.
    gtk_widget_queue_draw(GTK_WIDGET(data));
    
    // Find matching MonitorInfo for this canvas to update its input region
    for (auto& mi : g_monitors) {
        // This is a bit hacky but works: find window that contains this canvas
        GtkRoot* root = gtk_widget_get_root(GTK_WIDGET(data));
        if (root && GTK_IS_WINDOW(root)) {
            // Need to match monitor to window. Let's store window in MonitorInfo.
            // Simplified: just update input region for this specific window.
            UpdateInputRegion(GTK_WINDOW(root), mi);
            break; 
        }
    }

    static int tick = 0;
    if ((++tick % 10) == 0) RefreshSelectedGooseUi();
    return G_SOURCE_CONTINUE;
}

// --- Callbacks (in ui_callbacks.cpp) ------------------------------------

// --- Registry UI (in ui_factory.cpp) -----------------------------------

// --- Control panel (see ui_control_panel.cpp) -------------------------------

// --- Overlay window ----------------------------------------------------------
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
    g_monitors.clear();
    g_overlayCanvases.clear();

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
        g_monitors.push_back(mi);

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
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(canvas), draw_overlay, &g_monitors.back(), NULL);
        gtk_window_set_child(overlay, canvas);
        g_overlayCanvases.push_back(canvas);

        g_timeout_add(16, on_tick, canvas);
        gtk_window_present(overlay);
    }

    // Set globally unified screen dimensions.
    // If multi-monitor is disabled, clamp world bounds to the primary monitor
    // so geese/cursor logic won't drift into hidden monitors and crash at edges.
    if (!g_config.multiMonitorEnabled && !g_monitors.empty()) {
        const MonitorInfo& primary = g_monitors.front();
        g_screenWidth = primary.width;
        g_screenHeight = primary.height;
    } else {
        g_screenWidth = maxX;
        g_screenHeight = maxY;
    }
    
    // Global style for transparent windows
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, "window { background: transparent; }");
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(css), 800);
    // Initialize RAM tracker (pushes samples to the in-game UI log once per second)
    RamTracker_Init();
}
