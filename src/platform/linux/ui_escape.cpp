#include "ui_escape.h"
#include "world.h"
#include "config.h"
#include "goose.h"
#include "actor.h"
#include "actor_dropped_item.h"
#include <cmath>

static constexpr double ESC_KILL_HOLD_SECONDS = 1.0;
static bool g_escapeHeld = false;
static gint64 g_escapeHeldSinceUs = 0;
static bool g_escapeKillTriggered = false;

static GtkWidget* g_escapeHoldHudWindow = nullptr;
static GtkWidget* g_escapeHoldHudBar = nullptr;

void UiLogPush(const char* msg);

static double GetEscapeHoldProgress() {
    if (!g_escapeHeld || g_escapeHeldSinceUs == 0) return 0.0;
    const gint64 nowUs = g_get_monotonic_time();
    const double heldSeconds = (double)(nowUs - g_escapeHeldSinceUs) / 1000000.0;
    return std::clamp(heldSeconds / ESC_KILL_HOLD_SECONDS, 0.0, 1.0);
}

static void EnsureEscapeHoldHud() {
    if (g_escapeHoldHudWindow || !g_uiApp) return;

    GtkWindow* win = GTK_WINDOW(gtk_application_window_new(g_uiApp));
    g_escapeHoldHudWindow = GTK_WIDGET(win);
    gtk_window_set_decorated(win, FALSE);
    gtk_window_set_resizable(win, FALSE);
    gtk_window_set_default_size(win, 320, 64);

    gtk_layer_init_for_window(win);
    gtk_layer_set_layer(win, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_LEFT, 1);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_RIGHT, 1);
    gtk_layer_set_anchor(win, GTK_LAYER_SHELL_EDGE_BOTTOM, 1);
    gtk_layer_set_keyboard_mode(win, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    gtk_layer_set_margin(win, GTK_LAYER_SHELL_EDGE_BOTTOM, 20);

    GtkWidget* outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(outer, GTK_ALIGN_CENTER);
    gtk_window_set_child(win, outer);

    GtkWidget* frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_size_request(frame, 320, -1);
    gtk_widget_set_margin_top(frame, 10);
    gtk_widget_set_margin_bottom(frame, 10);
    gtk_widget_set_margin_start(frame, 12);
    gtk_widget_set_margin_end(frame, 12);
    gtk_box_append(GTK_BOX(outer), frame);

    GtkWidget* label = gtk_label_new("Hold Esc to clear geese");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(frame), label);

    g_escapeHoldHudBar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(g_escapeHoldHudBar), FALSE);
    gtk_widget_set_hexpand(g_escapeHoldHudBar, TRUE);
    gtk_widget_set_size_request(g_escapeHoldHudBar, 280, 16);
    gtk_box_append(GTK_BOX(frame), g_escapeHoldHudBar);

    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        ".escape-hold-hud { background: rgba(18, 20, 24, 0.88); border-radius: 12px; padding: 8px; }"
        ".escape-hold-hud progressbar trough { min-height: 16px; background: rgba(255, 255, 255, 0.12); border-radius: 999px; }"
        ".escape-hold-hud progressbar progress { min-height: 16px; background: rgba(236, 72, 52, 0.96); border-radius: 999px; }"
        ".escape-hold-hud label { color: white; font-weight: 700; }");
    gtk_widget_add_css_class(frame, "escape-hold-hud");
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css), 801);
    g_object_unref(css);

    gtk_widget_set_visible(g_escapeHoldHudWindow, FALSE);
}

static void UpdateEscapeHoldHud() {
    EnsureEscapeHoldHud();
    if (!g_escapeHoldHudWindow || !g_escapeHoldHudBar) return;

    const double progress = GetEscapeHoldProgress();
    if (progress <= 0.0 || g_escapeKillTriggered) {
        gtk_widget_set_visible(g_escapeHoldHudWindow, FALSE);
        return;
    }

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(g_escapeHoldHudBar), progress);
    gtk_widget_set_visible(g_escapeHoldHudWindow, TRUE);
    gtk_window_present(GTK_WINDOW(g_escapeHoldHudWindow));
}

void ClearAllGooseState() {
    ActorManager::Instance().removeAllDroppedItems();
    g_world.footprints.clear();
    ActorManager::Instance().destroyAllOfType("goose");
    g_world.cursorGrabberId = -1;
    g_world.selectedGooseId = 0;
    g_world.nextId = 0;
}

static void MaybeTriggerEscapeKill() {
    if (!g_escapeHeld || g_escapeKillTriggered) return;

    const gint64 nowUs = g_get_monotonic_time();
    const double heldSeconds = (double)(nowUs - g_escapeHeldSinceUs) / 1000000.0;
    if (heldSeconds < ESC_KILL_HOLD_SECONDS) return;

    ClearAllGooseState();
    UiLogPush("Emergency clear: held Esc for 1 second removed all geese.");
    g_escapeKillTriggered = true;
    UpdateEscapeHoldHud();
}

static gboolean cb_window_key_pressed(GtkEventControllerKey*, guint keyval, guint, GdkModifierType, gpointer) {
    if (keyval != GDK_KEY_Escape) return FALSE;

    if (g_world.cursorGrabberId != -1) {
        for (auto* g : ActorManager::Instance().getGeese()) {
            if (g->id == g_world.cursorGrabberId && g->state == GooseState::SNATCH_CURSOR) {
                UiLogPush("ESC pressed: ending snatch");
                g->state = GooseState::WANDER;
                g->PickNewTarget(g_config.screen.width, g_config.screen.height);
                g->stepTime = g_config.step.timeWander;
                g_world.cursorGrabberId = -1;
            }
        }
    }

    if (!g_escapeHeld) {
        g_escapeHeld = true;
        g_escapeHeldSinceUs = g_get_monotonic_time();
        g_escapeKillTriggered = false;
    }
    UpdateEscapeHoldHud();
    return FALSE;
}

static void cb_window_key_released(GtkEventControllerKey*, guint keyval, guint, GdkModifierType, gpointer) {
    if (keyval != GDK_KEY_Escape) return;

    g_escapeHeld = false;
    g_escapeHeldSinceUs = 0;
    g_escapeKillTriggered = false;
    UpdateEscapeHoldHud();
}

void ResetEscapeHoldState() {
    g_escapeHeld = false;
    g_escapeHeldSinceUs = 0;
    g_escapeKillTriggered = false;
    UpdateEscapeHoldHud();
}

static void cb_window_focus_leave(GtkEventControllerFocus*, gpointer) {
    ResetEscapeHoldState();
}

void AttachEmergencyEscController(GtkWidget* window) {
    GtkEventController* controller = gtk_event_controller_key_new();
    g_signal_connect(controller, "key-pressed", G_CALLBACK(cb_window_key_pressed), NULL);
    g_signal_connect(controller, "key-released", G_CALLBACK(cb_window_key_released), NULL);
    gtk_widget_add_controller(window, controller);

    GtkEventController* focus = gtk_event_controller_focus_new();
    g_signal_connect(focus, "leave", G_CALLBACK(cb_window_focus_leave), NULL);
    gtk_widget_add_controller(window, focus);
}