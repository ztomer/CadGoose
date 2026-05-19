#include "ui.h"
#include "ui_controls.h"
#include "ui_escape.h"
#include "world.h"
#include "config.h"
#include "goose.h"
#include "actor.h"
#include <string>

// Widgets exposed for control panel
GtkWidget* g_entryGooseName = nullptr;
GtkWidget* g_chkRandomizeBias = nullptr;
GtkWidget* g_mouseSlider = nullptr;
GtkWidget* g_labelMouseVal = nullptr;
GtkWidget* g_noteSlider = nullptr;
GtkWidget* g_labelNoteVal = nullptr;
GtkWidget* g_memeSlider = nullptr;
GtkWidget* g_labelMemeVal = nullptr;
GtkWidget* g_labelSelectedInfo = nullptr;
GtkWidget* g_chkGooseMudEnabled = nullptr;
GtkWidget* g_sliderGooseMudChance = nullptr;
GtkWidget* g_labelGooseMudChance = nullptr;
GtkWidget* g_sliderGooseMudLife = nullptr;
GtkWidget* g_labelGooseMudLife = nullptr;
GtkWidget* g_chkGooseCursorEnabled = nullptr;
GtkWidget* g_sliderGooseCursorChance = nullptr;
GtkWidget* g_labelGooseCursorChance = nullptr;
GtkWidget* g_sliderGooseSnatchDur = nullptr;
GtkWidget* g_labelGooseSnatchDur = nullptr;

// Forward decls
void cb_clear(GtkButton*, gpointer);
void RefreshSelectedGooseUi();
void ClearAllGooseState();

static void ApplyDefaultsToGoose(Goose* g) {
    if (!g) return;
    g->mudEnabled = g_config.mud.enabled;
    g->mudChance = g_config.mudChance;
    g->mudLifetime = g_config.mud.lifetime;
    g->cursorChaseEnabled = g_config.cursorChaseEnabled;
    g->cursorChaseChance = g_config.cursorChaseChance;
    g->snatchDuration = g_config.snatchDuration;
}

void cb_goose_copy_defaults(GtkButton*, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    ApplyDefaultsToGoose(g);
    RefreshSelectedGooseUi();
}

void cb_goose_apply_to_all(GtkButton*, gpointer) {
    Goose* src = GetGooseById(g_world.selectedGooseId);
    if (!src) return;
    for (auto* g : ActorManager::Instance().getGeese()) {
        g->mudEnabled = src->mudEnabled;
        g->mudChance = src->mudChance;
        g->mudLifetime = src->mudLifetime;
        g->cursorChaseEnabled = src->cursorChaseEnabled;
        g->cursorChaseChance = src->cursorChaseChance;
        g->snatchDuration = src->snatchDuration;
    }
    RefreshSelectedGooseUi();
}

// --- Callbacks ---------------------------------------------------------------
void cb_spawn(GtkButton*, gpointer) {
    std::string name = "";
    if (g_entryGooseName) {
        const char* txt = gtk_editable_get_text(GTK_EDITABLE(g_entryGooseName));
        if (txt) name = txt;
        gtk_editable_set_text(GTK_EDITABLE(g_entryGooseName), "");
    }
    if (name.empty()) name = "Goose " + std::to_string(g_world.nextId);
    Goose* gptr = new Goose(g_world.nextId++, name, g_world.screenWidth, g_world.screenHeight);
    ActorManager::Instance().add(gptr);
    Goose& g = *gptr;
    bool randize = true;
    if (g_chkRandomizeBias) randize = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_chkRandomizeBias));
    if (randize) {
        g.attackMouseBias = rand() % 51;
        g.memeFetchBias = rand() % 61;
        g.noteFetchBias = rand() % 41;
    } else {
        g.attackMouseBias = 0;
        g.memeFetchBias = 0;
        g.noteFetchBias = 0;
    }
}
void cb_clear(GtkButton*, gpointer) { ClearAllGooseState(); }
void cb_clear_log(GtkButton*, gpointer) { g_world.uiLog.clear(); }

void cb_fetch_meme(GtkButton*, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (g) g->ForceFetch(0, g_world.screenWidth, g_world.screenHeight);
}
void cb_fetch_text(GtkButton*, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (g) g->ForceFetch(1, g_world.screenWidth, g_world.screenHeight);
}
void cb_fetch_text_custom(GtkButton*, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) return;
    const char* txt = gtk_editable_get_text(GTK_EDITABLE(g_world.entryNote));
    std::string s = txt ? txt : "";
    if (!s.empty()) g->ForceFetchText(s, g_world.screenWidth, g_world.screenHeight);
}

void cb_select_goose(GtkSpinButton* spin, gpointer) {
    g_world.selectedGooseId = (int)gtk_spin_button_get_value(spin);
    RefreshSelectedGooseUi();
}

void RefreshSelectedGooseUi() {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (g_labelSelectedInfo) {
        if (!g) {
            gtk_label_set_text(GTK_LABEL(g_labelSelectedInfo), "Selected: (none)");
        } else {
            const char* stateStr;
            switch (g->state) {
                case WANDER: stateStr = "WANDER"; break;
                case FETCHING: stateStr = "FETCHING"; break;
                case RETURNING: stateStr = "RETURNING"; break;
                case CHASE_CURSOR: stateStr = "CHASE"; break;
                case SNATCH_CURSOR: stateStr = "SNATCH"; break;
                default: stateStr = "???"; break;
            }
            const char* heldStr = g->heldItem ? (g->heldItem->type == ItemData::MEME ? "MEME" : "NOTE") : "none";
            char buf[256];
            snprintf(buf, sizeof(buf), "Selected: ID %d | %s | held:%s | pos:(%.0f,%.0f) | cursorGrab:%s",
                     g->id, stateStr, heldStr, g->pos.x, g->pos.y, (g_world.cursorGrabberId == g->id) ? "Y" : "N");
            gtk_label_set_text(GTK_LABEL(g_labelSelectedInfo), buf);
        }
    }
    if (g) {
        if (g_mouseSlider) gtk_range_set_value(GTK_RANGE(g_mouseSlider), g->attackMouseBias);
        if (g_noteSlider) gtk_range_set_value(GTK_RANGE(g_noteSlider), g->noteFetchBias);
        if (g_memeSlider) gtk_range_set_value(GTK_RANGE(g_memeSlider), g->memeFetchBias);
        if (g_labelMouseVal) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->attackMouseBias); gtk_label_set_text(GTK_LABEL(g_labelMouseVal), buf); }
        if (g_labelNoteVal) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->noteFetchBias); gtk_label_set_text(GTK_LABEL(g_labelNoteVal), buf); }
        if (g_labelMemeVal) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->memeFetchBias); gtk_label_set_text(GTK_LABEL(g_labelMemeVal), buf); }
        if (g_chkGooseMudEnabled) gtk_check_button_set_active(GTK_CHECK_BUTTON(g_chkGooseMudEnabled), g->mudEnabled);
        if (g_chkGooseCursorEnabled) gtk_check_button_set_active(GTK_CHECK_BUTTON(g_chkGooseCursorEnabled), g->cursorChaseEnabled);
        if (g_sliderGooseMudChance) gtk_range_set_value(GTK_RANGE(g_sliderGooseMudChance), g->mudChance);
        if (g_labelGooseMudChance) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->mudChance); gtk_label_set_text(GTK_LABEL(g_labelGooseMudChance), buf); }
        if (g_sliderGooseMudLife) gtk_range_set_value(GTK_RANGE(g_sliderGooseMudLife), g->mudLifetime);
        if (g_labelGooseMudLife) { char buf[32]; snprintf(buf, sizeof(buf), "%.1fs", g->mudLifetime); gtk_label_set_text(GTK_LABEL(g_labelGooseMudLife), buf); }
        if (g_sliderGooseCursorChance) gtk_range_set_value(GTK_RANGE(g_sliderGooseCursorChance), g->cursorChaseChance);
        if (g_labelGooseCursorChance) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->cursorChaseChance); gtk_label_set_text(GTK_LABEL(g_labelGooseCursorChance), buf); }
        if (g_sliderGooseSnatchDur) gtk_range_set_value(GTK_RANGE(g_sliderGooseSnatchDur), g->snatchDuration);
        if (g_labelGooseSnatchDur) { char buf[32]; snprintf(buf, sizeof(buf), "%.1fs", g->snatchDuration); gtk_label_set_text(GTK_LABEL(g_labelGooseSnatchDur), buf); }
    }
}

void cb_action_apply(GtkButton*, gpointer user_data) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) return;
    GtkWidget* combo = GTK_WIDGET(user_data);
    int idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(combo));
    if (idx == 0) g->ForceWander(g_world.screenWidth, g_world.screenHeight);
    else if (idx == 1) g->ForceFetch(0, g_world.screenWidth, g_world.screenHeight);
    else if (idx == 2) g->ForceFetch(1, g_world.screenWidth, g_world.screenHeight);
}

void cb_set_mouse_bias(GtkRange* r, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) return;
    g->attackMouseBias = (int)gtk_range_get_value(r);
    if (g_labelMouseVal) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->attackMouseBias); gtk_label_set_text(GTK_LABEL(g_labelMouseVal), buf); }
}
void cb_set_note_bias(GtkRange* r, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) return;
    g->noteFetchBias = (int)gtk_range_get_value(r);
    if (g_labelNoteVal) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->noteFetchBias); gtk_label_set_text(GTK_LABEL(g_labelNoteVal), buf); }
}
void cb_set_meme_bias(GtkRange* r, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) return;
    g->memeFetchBias = (int)gtk_range_get_value(r);
    if (g_labelMemeVal) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->memeFetchBias); gtk_label_set_text(GTK_LABEL(g_labelMemeVal), buf); }
}
void cb_goose_mud_toggle(GtkCheckButton* b, gpointer) { Goose* g = GetGooseById(g_world.selectedGooseId); if (g) g->mudEnabled = gtk_check_button_get_active(b); }
void cb_goose_mud_chance(GtkRange* r, gpointer) { Goose* g = GetGooseById(g_world.selectedGooseId); if (!g) return; g->mudChance = (int)gtk_range_get_value(r); if (g_labelGooseMudChance) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->mudChance); gtk_label_set_text(GTK_LABEL(g_labelGooseMudChance), buf); } }
void cb_goose_mud_life(GtkRange* r, gpointer) { Goose* g = GetGooseById(g_world.selectedGooseId); if (!g) return; g->mudLifetime = (float)gtk_range_get_value(r); if (g_labelGooseMudLife) { char buf[32]; snprintf(buf, sizeof(buf), "%.1fs", g->mudLifetime); gtk_label_set_text(GTK_LABEL(g_labelGooseMudLife), buf); } }
void cb_goose_cursor_toggle(GtkCheckButton* b, gpointer) { Goose* g = GetGooseById(g_world.selectedGooseId); if (g) g->cursorChaseEnabled = gtk_check_button_get_active(b); }
void cb_goose_cursor_chance(GtkRange* r, gpointer) { Goose* g = GetGooseById(g_world.selectedGooseId); if (!g) return; g->cursorChaseChance = (int)gtk_range_get_value(r); if (g_labelGooseCursorChance) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g->cursorChaseChance); gtk_label_set_text(GTK_LABEL(g_labelGooseCursorChance), buf); } }
void cb_goose_snatch_dur(GtkRange* r, gpointer) { Goose* g = GetGooseById(g_world.selectedGooseId); if (!g) return; g->snatchDuration = (float)gtk_range_get_value(r); if (g_labelGooseSnatchDur) { char buf[32]; snprintf(buf, sizeof(buf), "%.1fs", g->snatchDuration); gtk_label_set_text(GTK_LABEL(g_labelGooseSnatchDur), buf); } }

void cb_debug(GtkCheckButton* b, gpointer) { bool v = gtk_check_button_get_active(b); g_config.debug.toTerminal = v; g_config.debug.visuals = v; }
void cb_multi_monitor(GtkCheckButton* b, gpointer) { g_config.cursor.multiMonitorEnabled = gtk_check_button_get_active(b); }
void cb_memes(GtkCheckButton* b, gpointer) { g_config.memesEnabled = gtk_check_button_get_active(b); }
void cb_scale(GtkRange* r, gpointer) { g_config.general.globalScale = (float)gtk_range_get_value(r); if (g_labelScaleVal) { char buf[32]; snprintf(buf, sizeof(buf), "%.2f", g_config.general.globalScale); gtk_label_set_text(GTK_LABEL(g_labelScaleVal), buf); } }
void cb_walk(GtkRange* r, gpointer) { g_config.baseWalkSpeed = (float)gtk_range_get_value(r); if (g_labelWalkVal) { char buf[32]; snprintf(buf, sizeof(buf), "%.0f", g_config.baseWalkSpeed); gtk_label_set_text(GTK_LABEL(g_labelWalkVal), buf); } }
void cb_run(GtkRange* r, gpointer) { g_config.baseRunSpeed = (float)gtk_range_get_value(r); if (g_labelRunVal) { char buf[32]; snprintf(buf, sizeof(buf), "%.0f", g_config.baseRunSpeed); gtk_label_set_text(GTK_LABEL(g_labelRunVal), buf); } }
void cb_cursor_toggle(GtkCheckButton* b, gpointer) { g_config.cursorChaseEnabled = gtk_check_button_get_active(b); }
void cb_cursor_chance(GtkSpinButton* spin, gpointer) { g_config.cursorChaseChance = (int)gtk_spin_button_get_value(spin); }
void cb_snatch_duration(GtkRange* r, gpointer) { g_config.snatchDuration = (float)gtk_range_get_value(r); if (g_labelSnatchVal) { char buf[32]; snprintf(buf, sizeof(buf), "%.1fs", g_config.snatchDuration); gtk_label_set_text(GTK_LABEL(g_labelSnatchVal), buf); } }
void cb_audio(GtkCheckButton* b, gpointer) { g_config.audioEnabled = gtk_check_button_get_active(b); }
void cb_mud_toggle(GtkCheckButton* b, gpointer) { g_config.mud.enabled = gtk_check_button_get_active(b); }
void cb_mud_chance(GtkRange* r, gpointer) { g_config.mudChance = (int)gtk_range_get_value(r); if (g_labelMudChanceVal) { char buf[32]; snprintf(buf, sizeof(buf), "%d%%", g_config.mudChance); gtk_label_set_text(GTK_LABEL(g_labelMudChanceVal), buf); } }
void cb_mud_lifetime(GtkRange* r, gpointer) { g_config.mud.lifetime = (float)gtk_range_get_value(r); if (g_labelMudLifetimeVal) { char buf[32]; snprintf(buf, sizeof(buf), "%.1fs", g_config.mud.lifetime); gtk_label_set_text(GTK_LABEL(g_labelMudLifetimeVal), buf); } }
void cb_debug_overlay_verbose(GtkCheckButton* b, gpointer) { g_debugOverlayVerbose = gtk_check_button_get_active(b); }
void cb_debug_overlay_selected_only(GtkCheckButton* b, gpointer) { g_debugOverlaySelectedOnly = gtk_check_button_get_active(b); }

static void cb_reset_biases_selected(GtkButton*, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) return;
    g->attackMouseBias = 0;
    g->noteFetchBias = 0;
    g->memeFetchBias = 0;
    ApplyDefaultsToGoose(g);
    RefreshSelectedGooseUi();
}
static void cb_randomize_biases_selected(GtkButton*, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) return;
    g->attackMouseBias = rand() % 101;
    g->noteFetchBias = rand() % 101;
    g->memeFetchBias = rand() % 101;
    g->mudEnabled = (rand() % 2) == 0;
    g->mudChance = rand() % 101;
    g->mudLifetime = 5.0f + (float)(rand() % 56);
    g->cursorChaseEnabled = (rand() % 2) == 0;
    g->cursorChaseChance = rand() % 26;
    g->snatchDuration = 1.0f + (float)(rand() % 10);
    RefreshSelectedGooseUi();
}
void cb_attack_cursor(GtkButton*, gpointer) {
    Goose* g = GetGooseById(g_world.selectedGooseId);
    if (!g) {
        auto geese = ActorManager::Instance().getGeese();
        if (geese.empty()) {
            g = new Goose(g_world.nextId++, "", g_world.screenWidth, g_world.screenHeight);
            ActorManager::Instance().add(g);
        } else {
            g = geese.front();
        }
    }
    g->state = CHASE_CURSOR;
    if (g_cursorProvider) { CursorState cs = g_cursorProvider->Read(); if (cs.caps & CAP_GET_POS && cs.position.x >= 0) g->target = cs.position; }
}