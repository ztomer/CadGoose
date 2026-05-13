#include "ui_control_panel.h"
#include "ui_controls.h"
#include "ui_factory.h"
#include "ui_escape.h"
#include "world.h"
#include "config.h"
#include "goose.h"

static gboolean cb_control_close(GtkWindow* window, gpointer) {
    gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
    return TRUE;
}

void activate_control_panel(GtkApplication* app) {
    g_uiApp = app;
    GtkWidget* win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win), "Goose Control Panel");
    gtk_window_set_default_size(GTK_WINDOW(win), 360, 680);
    AttachEmergencyEscController(win);

    GtkWidget* notebook = gtk_notebook_new();
    gtk_window_set_child(GTK_WINDOW(win), notebook);

    // PAGE 1: GEESE
    GtkWidget* rootGeese = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* scrollGeese = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollGeese), rootGeese);

    GtkWidget* geeseInner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(geeseInner, 12);
    gtk_widget_set_margin_bottom(geeseInner, 12);
    gtk_widget_set_margin_start(geeseInner, 12);
    gtk_widget_set_margin_end(geeseInner, 12);
    gtk_box_append(GTK_BOX(rootGeese), geeseInner);

    GtkWidget* mngBox = nullptr;
    gtk_box_append(GTK_BOX(geeseInner), make_section("Population", &mngBox));

    gtk_box_append(GTK_BOX(mngBox), gtk_label_new("New Goose Name:"));
    g_entryGooseName = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_entryGooseName), "Leave blank for random");
    gtk_box_append(GTK_BOX(mngBox), g_entryGooseName);

    GtkWidget* spawnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* btnSpawn = gtk_button_new_with_label("Spawn Goose");
    g_signal_connect(btnSpawn, "clicked", G_CALLBACK(cb_spawn), NULL);
    gtk_box_append(GTK_BOX(spawnBox), btnSpawn);
    GtkWidget* btnClear = gtk_button_new_with_label("Remove All");
    g_signal_connect(btnClear, "clicked", G_CALLBACK(cb_clear), NULL);
    gtk_box_append(GTK_BOX(spawnBox), btnClear);
    gtk_box_append(GTK_BOX(mngBox), spawnBox);

    g_chkRandomizeBias = gtk_check_button_new_with_label("Randomize tendencies on spawn");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(g_chkRandomizeBias), TRUE);
    gtk_box_append(GTK_BOX(mngBox), g_chkRandomizeBias);

    GtkWidget* selBox = nullptr;
    gtk_box_append(GTK_BOX(geeseInner), make_section("Targeted Goose", &selBox));

    GtkWidget* idRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(idRow), gtk_label_new("Goose ID:"));
    GtkWidget* spinGoose = gtk_spin_button_new_with_range(0, 999, 1);
    g_signal_connect(spinGoose, "value-changed", G_CALLBACK(cb_select_goose), NULL);
    gtk_box_append(GTK_BOX(idRow), spinGoose);
    gtk_box_append(GTK_BOX(selBox), idRow);

    g_labelSelectedInfo = gtk_label_new("No goose selected");
    gtk_label_set_wrap(GTK_LABEL(g_labelSelectedInfo), TRUE);
    gtk_box_append(GTK_BOX(selBox), g_labelSelectedInfo);

    gtk_box_append(GTK_BOX(selBox), gtk_label_new("Quick Actions:"));
    GtkWidget* actRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* btnMeme = gtk_button_new_with_label("Fetch Meme");
    g_signal_connect(btnMeme, "clicked", G_CALLBACK(cb_fetch_meme), NULL);
    gtk_box_append(GTK_BOX(actRow), btnMeme);
    GtkWidget* btnNote = gtk_button_new_with_label("Fetch Note");
    g_signal_connect(btnNote, "clicked", G_CALLBACK(cb_fetch_text), NULL);
    gtk_box_append(GTK_BOX(actRow), btnNote);
    gtk_box_append(GTK_BOX(selBox), actRow);

    GtkWidget* btnAtk = gtk_button_new_with_label("Force Cursor Attack");
    g_signal_connect(btnAtk, "clicked", G_CALLBACK(cb_attack_cursor), NULL);
    gtk_box_append(GTK_BOX(selBox), btnAtk);

    gtk_box_append(GTK_BOX(selBox), gtk_label_new("Individual Tendencies:"));
    gtk_box_append(GTK_BOX(selBox), make_scale_row("Attack Mouse", 0, 100, 1, 0, G_CALLBACK(cb_set_mouse_bias), &g_mouseSlider, &g_labelMouseVal, "0%"));
    gtk_box_append(GTK_BOX(selBox), make_scale_row("Fetch Notes", 0, 100, 1, 0, G_CALLBACK(cb_set_note_bias), &g_noteSlider, &g_labelNoteVal, "0%"));
    gtk_box_append(GTK_BOX(selBox), make_scale_row("Fetch Memes", 0, 100, 1, 0, G_CALLBACK(cb_set_meme_bias), &g_memeSlider, &g_labelMemeVal, "0%"));

    gtk_box_append(GTK_BOX(selBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(selBox), gtk_label_new("Individual Mud Controls:"));
    g_chkGooseMudEnabled = gtk_check_button_new_with_label("Enable Mud for this Goose");
    g_signal_connect(g_chkGooseMudEnabled, "toggled", G_CALLBACK(cb_goose_mud_toggle), NULL);
    gtk_box_append(GTK_BOX(selBox), g_chkGooseMudEnabled);
    gtk_box_append(GTK_BOX(selBox), make_scale_row("Mud Chance", 0, 100, 1, 0, G_CALLBACK(cb_goose_mud_chance), &g_sliderGooseMudChance, &g_labelGooseMudChance, "0%"));
    gtk_box_append(GTK_BOX(selBox), make_scale_row("Mud Lifetime", 5, 120, 1, 0, G_CALLBACK(cb_goose_mud_life), &g_sliderGooseMudLife, &g_labelGooseMudLife, "0s"));

    gtk_box_append(GTK_BOX(selBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(GTK_BOX(selBox), gtk_label_new("Individual Cursor Controls:"));
    g_chkGooseCursorEnabled = gtk_check_button_new_with_label("Enable Cursor Chase for this Goose");
    g_signal_connect(g_chkGooseCursorEnabled, "toggled", G_CALLBACK(cb_goose_cursor_toggle), NULL);
    gtk_box_append(GTK_BOX(selBox), g_chkGooseCursorEnabled);
    gtk_box_append(GTK_BOX(selBox), make_scale_row("Base Chance", 0, 100, 1, 0, G_CALLBACK(cb_goose_cursor_chance), &g_sliderGooseCursorChance, &g_labelGooseCursorChance, "0%"));
    gtk_box_append(GTK_BOX(selBox), make_scale_row("Snatch Duration", 0.5, 15, 0.5, 0, G_CALLBACK(cb_goose_snatch_dur), &g_sliderGooseSnatchDur, &g_labelGooseSnatchDur, "0s"));

    GtkWidget* perRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* btnCopyDefaults = gtk_button_new_with_label("Copy Global Defaults");
    g_signal_connect(btnCopyDefaults, "clicked", G_CALLBACK(cb_goose_copy_defaults), NULL);
    gtk_box_append(GTK_BOX(perRow), btnCopyDefaults);
    GtkWidget* btnApplyAll = gtk_button_new_with_label("Apply To All Geese");
    g_signal_connect(btnApplyAll, "clicked", G_CALLBACK(cb_goose_apply_to_all), NULL);
    gtk_box_append(GTK_BOX(perRow), btnApplyAll);
    gtk_box_append(GTK_BOX(selBox), perRow);

    GtkWidget* biasRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget* btnRand = gtk_button_new_with_label("Randomize Biases");
    g_signal_connect(btnRand, "clicked", G_CALLBACK(cb_randomize_biases_selected), NULL);
    gtk_box_append(GTK_BOX(biasRow), btnRand);
    GtkWidget* btnReset = gtk_button_new_with_label("Reset Biases");
    g_signal_connect(btnReset, "clicked", G_CALLBACK(cb_reset_biases_selected), NULL);
    gtk_box_append(GTK_BOX(biasRow), btnReset);
    gtk_box_append(GTK_BOX(selBox), biasRow);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollGeese, gtk_label_new("Geese"));

    // PAGE 2: GLOBAL
    GtkWidget* rootGlobal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* scrollGlobal = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollGlobal), rootGlobal);

    GtkWidget* globalInner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(globalInner, 12);
    gtk_widget_set_margin_bottom(globalInner, 12);
    gtk_widget_set_margin_start(globalInner, 12);
    gtk_widget_set_margin_end(globalInner, 12);
    gtk_box_append(GTK_BOX(rootGlobal), globalInner);

    GtkWidget* behBox = nullptr;
    gtk_box_append(GTK_BOX(globalInner), make_section("Goose Behavior", &behBox));
    PopulateConfigSection(behBox, "Behavior");

    GtkWidget* movBox = nullptr;
    gtk_box_append(GTK_BOX(globalInner), make_section("Movement & Scale", &movBox));
    PopulateConfigSection(movBox, "Movement");

    GtkWidget* genBox = nullptr;
    gtk_box_append(GTK_BOX(globalInner), make_section("General", &genBox));
    PopulateConfigSection(genBox, "General");

    GtkWidget* curBox = nullptr;
    gtk_box_append(GTK_BOX(globalInner), make_section("Cursor", &curBox));
    PopulateConfigSection(curBox, "Cursor");

    GtkWidget* aiBox = nullptr;
    gtk_box_append(GTK_BOX(globalInner), make_section("AI Settings", &aiBox));
    PopulateConfigSection(aiBox, "AI");

    GtkWidget* colorBox = nullptr;
    gtk_box_append(GTK_BOX(globalInner), make_section("Colors", &colorBox));
    PopulateConfigSection(colorBox, "Colors");

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollGlobal, gtk_label_new("Global"));

    // PAGE 3: ENGINE
    GtkWidget* rootWhisp = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* whInner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(whInner, 12);
    gtk_widget_set_margin_bottom(whInner, 12);
    gtk_widget_set_margin_start(whInner, 12);
    gtk_widget_set_margin_end(whInner, 12);
    gtk_box_append(GTK_BOX(rootWhisp), whInner);

    GtkWidget* dbgBox = nullptr;
    gtk_box_append(GTK_BOX(whInner), make_section("System Diagnostics", &dbgBox));
    PopulateConfigSection(dbgBox, "Debug");

    GtkWidget* btnLog = gtk_button_new_with_label("Clear Runtime Log");
    g_signal_connect(btnLog, "clicked", G_CALLBACK(cb_clear_log), NULL);
    gtk_box_append(GTK_BOX(dbgBox), btnLog);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), rootWhisp, gtk_label_new("Engine"));

    g_signal_connect(win, "close-request", G_CALLBACK(cb_control_close), NULL);

    RefreshSelectedGooseUi();
    gtk_window_present(GTK_WINDOW(win));
}