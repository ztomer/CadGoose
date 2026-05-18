#include "ui_tick.h"
#include "ui.h"
#include "ui_escape.h"
#include "world.h"
#include "config.h"
#include "actor.h"
#include "cursor_io.h"

gboolean on_tick(gpointer data) {
    g_time += kTickDt;
    MaybeTriggerEscapeKill();
    UpdateEscapeHoldHud();

    CursorState cursor = {};
    CursorAction action = {};
    if (g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }

    for (auto* g : ActorManager::Instance().getGeese()) {
        CursorAction a = g->Update(kTickDt, g_time, g_world.screenWidth, g_world.screenHeight, cursor);
        if (!a.isNone()) action = a;
    }

    if (g_cursorProvider && !action.isNone()) {
        g_cursorProvider->Execute(action);
    }

    g_world.droppedItems.remove_if([](DroppedItem& i) {
        bool exp = i.isExpired(g_time);
        if (exp) delete i.data;
        return exp;
    });

    while (!g_world.footprints.empty()) {
        Footprint& fp = g_world.footprints.front();
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mudLifetime;
        if ((g_time - fp.timeSpawned) > life) {
            g_world.footprints.pop();
        } else {
            break;
        }
    }

    // We pass a window to setup_overlay, but we need to update ALL overlays.
    // However, on_tick is associated with a specific canvas.
    // To keep it simple, we queue draw on all monitors.
    // This is handled by setup_overlay_window which spawns timers or we can use a global signal.
    gtk_widget_queue_draw(GTK_WIDGET(data));
    
    // Find matching MonitorInfo for this canvas to update its input region
    for (auto& mi : g_world.monitors) {
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
