#pragma once
#include <gtk/gtk.h>

GtkWidget* make_section(const char* title, GtkWidget** out_box);
GtkWidget* make_scale_row(const char* title, double min, double max, double step,
                          double initial, GCallback on_changed,
                          GtkWidget** out_scale, GtkWidget** out_value_label,
                          const char* initial_value_text);
void PopulateConfigSection(GtkWidget* container, const char* sectionName);