#pragma once
#include <gtk/gtk.h>

void DrawingInit();
void DrawingShutdown();
void draw_overlay(GtkDrawingArea*, cairo_t* cr, int width, int height, gpointer data);
void draw_debug_overlay(cairo_t* cr, int width, int height);