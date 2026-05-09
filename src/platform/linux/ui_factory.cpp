#include "ui_factory.h"
#include "ui_controls.h"
#include "config.h"
#include <cstdio>
#include <string>

static void cb_generic_toggle(GtkCheckButton* btn, gpointer data) {
    ConfigOption* opt = (ConfigOption*)data;
    if (opt->type == CFG_BOOL) {
        *(bool*)opt->ptr = gtk_check_button_get_active(btn);
        if (opt->onChange) opt->onChange();
    }
}

static void cb_generic_spin(GtkSpinButton* spin, gpointer data) {
    ConfigOption* opt = (ConfigOption*)data;
    if (opt->type == CFG_INT) {
        *(int*)opt->ptr = (int)gtk_spin_button_get_value(spin);
        if (opt->onChange) opt->onChange();
    }
}

GtkWidget* make_section(const char* title, GtkWidget** out_box) {
    GtkWidget* frame = gtk_frame_new(title);
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.02f);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);

    gtk_frame_set_child(GTK_FRAME(frame), box);
    if (out_box) *out_box = box;
    return frame;
}

GtkWidget* make_scale_row(const char* title, double min, double max, double step,
                          double initial, GCallback on_changed,
                          GtkWidget** out_scale, GtkWidget** out_value_label,
                          const char* initial_value_text) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget* lbl = gtk_label_new(title);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(row), lbl);

    GtkWidget* scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_range_set_value(GTK_RANGE(scale), initial);
    if (on_changed) g_signal_connect(scale, "value-changed", on_changed, NULL);
    gtk_box_append(GTK_BOX(row), scale);

    GtkWidget* val = gtk_label_new(initial_value_text ? initial_value_text : "");
    gtk_widget_set_halign(val, GTK_ALIGN_END);
    gtk_widget_set_size_request(val, 56, -1);
    gtk_box_append(GTK_BOX(row), val);

    if (out_scale) *out_scale = scale;
    if (out_value_label) *out_value_label = val;
    return row;
}

void PopulateConfigSection(GtkWidget* container, const char* sectionName) {
    for (auto& opt : g_configRegistry) {
        if (std::string(opt.section) != sectionName) continue;

        if (opt.type == CFG_BOOL) {
            GtkWidget* chk = gtk_check_button_new_with_label(opt.label);
            gtk_check_button_set_active(GTK_CHECK_BUTTON(chk), *(bool*)opt.ptr);
            g_signal_connect(chk, "toggled", G_CALLBACK(cb_generic_toggle), &opt);
            gtk_box_append(GTK_BOX(container), chk);
        }
        else if (opt.type == CFG_FLOAT) {
            char valTxt[32];
            snprintf(valTxt, sizeof(valTxt), "%.1f%s", *(float*)opt.ptr, opt.suffix);
            GtkWidget* scale = nullptr;
            GtkWidget* label = nullptr;
            GtkWidget* row = make_scale_row(opt.label, opt.min, opt.max, opt.step, *(float*)opt.ptr, NULL, &scale, &label, valTxt);

            struct Sync { ConfigOption* p; GtkWidget* l; };
            Sync* s = new Sync{&opt, label};
            g_signal_connect(scale, "value-changed", G_CALLBACK(+[](GtkRange* r, gpointer d){
                Sync* sc = (Sync*)d;
                float v = (float)gtk_range_get_value(r);
                *(float*)sc->p->ptr = v;
                if (sc->p->onChange) sc->p->onChange();
                char buf[32];
                snprintf(buf, sizeof(buf), "%.1f%s", v, sc->p->suffix);
                gtk_label_set_text(GTK_LABEL(sc->l), buf);
            }), s);
            gtk_box_append(GTK_BOX(container), row);
        }
        else if (opt.type == CFG_INT) {
            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            gtk_box_append(GTK_BOX(row), gtk_label_new(opt.label));
            GtkWidget* spin = gtk_spin_button_new_with_range(opt.min, opt.max, opt.step);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *(int*)opt.ptr);
            g_signal_connect(spin, "value-changed", G_CALLBACK(cb_generic_spin), &opt);
            gtk_widget_set_hexpand(spin, TRUE);
            gtk_widget_set_halign(spin, GTK_ALIGN_END);
            gtk_box_append(GTK_BOX(row), spin);
            if (opt.suffix && strlen(opt.suffix) > 0) {
                gtk_box_append(GTK_BOX(row), gtk_label_new(opt.suffix));
            }
            gtk_box_append(GTK_BOX(container), row);
        }
        else if (opt.type == CFG_STRING) {
            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
            gtk_box_append(GTK_BOX(row), gtk_label_new(opt.label));
            GtkWidget* entry = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(entry), ((std::string*)opt.ptr)->c_str());
            if (std::string(opt.label).find("Key") != std::string::npos) {
                gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
            }
            g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable* e, gpointer d){
                ConfigOption* o = (ConfigOption*)d;
                *(std::string*)o->ptr = gtk_editable_get_text(e);
                if (o->onChange) o->onChange();
            }), &opt);
            gtk_box_append(GTK_BOX(row), entry);
            gtk_box_append(GTK_BOX(container), row);
        }
    }
}