#ifndef UI_DEBUG_H
#define UI_DEBUG_H

#include <gtk/gtk.h>
#include <pango/pango.h>

extern bool g_debugOverlayVerbose;
extern bool g_debugOverlaySelectedOnly;

void draw_debug_overlay(cairo_t* cr, int width, int height, bool verbose, bool selectedOnly);

#endif