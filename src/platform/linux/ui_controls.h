#pragma once
#include <gtk/gtk.h>

void activate_control_panel(GtkApplication* app);

extern GtkWidget* g_entryGooseName;
extern GtkWidget* g_chkRandomizeBias;
extern GtkWidget* g_mouseSlider;
extern GtkWidget* g_labelMouseVal;
extern GtkWidget* g_noteSlider;
extern GtkWidget* g_labelNoteVal;
extern GtkWidget* g_memeSlider;
extern GtkWidget* g_labelMemeVal;
extern GtkWidget* g_labelSelectedInfo;
extern GtkWidget* g_chkGooseMudEnabled;
extern GtkWidget* g_sliderGooseMudChance;
extern GtkWidget* g_labelGooseMudChance;
extern GtkWidget* g_sliderGooseMudLife;
extern GtkWidget* g_labelGooseMudLife;
extern GtkWidget* g_chkGooseCursorEnabled;
extern GtkWidget* g_sliderGooseCursorChance;
extern GtkWidget* g_labelGooseCursorChance;
extern GtkWidget* g_sliderGooseSnatchDur;
extern GtkWidget* g_labelGooseSnatchDur;

void RefreshSelectedGooseUi();
void cb_spawn(GtkButton*, gpointer);
void cb_clear(GtkButton*, gpointer);
void cb_clear_log(GtkButton*, gpointer);
void cb_fetch_meme(GtkButton*, gpointer);
void cb_fetch_text(GtkButton*, gpointer);
void cb_fetch_text_custom(GtkButton*, gpointer);
void cb_select_goose(GtkSpinButton*, gpointer);
void cb_action_apply(GtkButton*, gpointer);
void cb_set_mouse_bias(GtkRange*, gpointer);
void cb_set_note_bias(GtkRange*, gpointer);
void cb_set_meme_bias(GtkRange*, gpointer);
void cb_goose_mud_toggle(GtkCheckButton*, gpointer);
void cb_goose_mud_chance(GtkRange*, gpointer);
void cb_goose_mud_life(GtkRange*, gpointer);
void cb_goose_cursor_toggle(GtkCheckButton*, gpointer);
void cb_goose_cursor_chance(GtkRange*, gpointer);
void cb_goose_snatch_dur(GtkRange*, gpointer);
void cb_debug(GtkCheckButton*, gpointer);
void cb_multi_monitor(GtkCheckButton*, gpointer);
void cb_memes(GtkCheckButton*, gpointer);
void cb_scale(GtkRange*, gpointer);
void cb_walk(GtkRange*, gpointer);
void cb_run(GtkRange*, gpointer);
void cb_cursor_toggle(GtkCheckButton*, gpointer);
void cb_cursor_chance(GtkSpinButton*, gpointer);
void cb_snatch_duration(GtkRange*, gpointer);
void cb_audio(GtkCheckButton*, gpointer);
void cb_mud_toggle(GtkCheckButton*, gpointer);
void cb_mud_chance(GtkRange*, gpointer);
void cb_mud_lifetime(GtkRange*, gpointer);
void cb_debug_overlay_verbose(GtkCheckButton*, gpointer);
void cb_debug_overlay_selected_only(GtkCheckButton*, gpointer);

void cb_goose_copy_defaults(GtkButton*, gpointer);
void cb_goose_apply_to_all(GtkButton*, gpointer);
void cb_randomize_biases_selected(GtkButton*, gpointer);
void cb_reset_biases_selected(GtkButton*, gpointer);
void cb_attack_cursor(GtkButton*, gpointer);