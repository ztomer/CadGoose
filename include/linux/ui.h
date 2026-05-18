#pragma once
#include <gtk/gtk.h>
#ifdef GTK4_LAYER_SHELL_ENABLED
#include <gtk4-layer-shell.h>
#endif

void setup_overlay_window(GtkApplication* app);
struct MonitorInfo;
void UpdateInputRegion(GtkWindow* window, const MonitorInfo& m);
void draw_overlay(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer data);
gboolean on_tick(gpointer data);
