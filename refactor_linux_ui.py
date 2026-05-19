import re

# Read ui.cpp
with open('src/platform/linux/ui.cpp', 'r') as f:
    ui_cpp = f.read()

# Extract DrawFootprints
draw_footprints_start = ui_cpp.find('static void DrawFootprints(cairo_t* cr) {')
draw_dropped_items_start = ui_cpp.find('static void DrawDroppedItems(cairo_t* cr) {')
draw_overlay_start = ui_cpp.find('void draw_overlay(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer data) {')
on_tick_start = ui_cpp.find('gboolean on_tick(gpointer data) {')
setup_overlay_start = ui_cpp.find('void setup_overlay_window(GtkApplication* app) {')

draw_footprints_code = ui_cpp[draw_footprints_start:draw_dropped_items_start]
draw_dropped_items_code = ui_cpp[draw_dropped_items_start:draw_overlay_start]

# We need to extract draw_overlay up to on_tick
draw_overlay_code = ui_cpp[draw_overlay_start:on_tick_start]

# Extract on_tick up to setup_overlay_window
on_tick_code = ui_cpp[on_tick_start:setup_overlay_start]

# --- 1. ui_tick.cpp & h ---
with open('src/platform/linux/ui_tick.h', 'w') as f:
    f.write('#pragma once\n#include <gtk/gtk.h>\n\ngboolean on_tick(gpointer data);\n')

with open('src/platform/linux/ui_tick.cpp', 'w') as f:
    f.write('#include "ui_tick.h"\n#include "ui.h"\n#include "ui_escape.h"\n#include "world.h"\n#include "config.h"\n#include "actor.h"\n#include "cursor_io.h"\n\n')
    f.write(on_tick_code)


# --- 2. ui_debug.cpp & h ---
# Extract the debugVisuals block from draw_overlay
debug_block_start = draw_overlay_code.find('        // Visual debug: highlight all geese when enabled\n        if (g_config.debugVisuals) {')
debug_block_end = draw_overlay_code.find('    cairo_restore(cr); \n\n    // Debug overlay')
debug_code = draw_overlay_code[debug_block_start:debug_block_end]

# It has a loop `for (auto* g : ActorManager::Instance().getGeese())` outside. We will make a function:
debug_func = """void draw_goose_debug_visuals(cairo_t* cr, Goose* g, const Vector2& origin) {
    cairo_save(cr);
    
    // 1. Highlight circle around goose
    cairo_set_source_rgba(cr, 1, 1, 0, 0.15);
    cairo_set_line_width(cr, 2.0);
    cairo_arc(cr, g->pos.x, g->pos.y, 40, 0, G_PI * 2);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 1, 1, 0, 0.4);
    cairo_stroke(cr);

    // 2. ID label near goose
    char idBuf[16];
    snprintf(idBuf, sizeof(idBuf), "ID %d", g->id);
    cairo_set_source_rgba(cr, 1, 1, 0, 0.9);
    cairo_move_to(cr, g->pos.x + 15, g->pos.y - 25);
    PangoLayout* idLayout = pango_cairo_create_layout(cr);
    pango_layout_set_text(idLayout, idBuf, -1);
    pango_cairo_show_layout(cr, idLayout);
    g_object_unref(idLayout);

    // 3. Target point and line to target
    cairo_set_source_rgba(cr, 0, 1, 0, 0.5);
    cairo_set_line_width(cr, 1.5);
    double dashes[] = {4.0, 2.0};
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_move_to(cr, origin.x, origin.y);
    cairo_line_to(cr, g->target.x, g->target.y);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0); 

    cairo_set_source_rgba(cr, 0, 1, 0, 0.8);
    if (g->state == GooseState::RETURNING) cairo_set_source_rgba(cr, 1, 0, 1, 0.8);
    if (g->state == GooseState::FETCHING) cairo_set_source_rgba(cr, 0, 1, 1, 0.8);
    
    cairo_arc(cr, g->target.x, g->target.y, 6, 0, G_PI * 2);
    cairo_fill(cr);

    // 4. Threshold circle
    float threshold = std::max(30.0f * g_config.general.globalScale, 25.0f);
    if (g->state == GooseState::RETURNING) threshold = std::max(50.0f * g_config.general.globalScale, 40.0f);
    if (g->state == GooseState::CHASE_CURSOR) threshold = std::max(22.0f * g_config.general.globalScale, 15.0f);

    cairo_set_source_rgba(cr, 1, 1, 1, 0.2);
    cairo_set_line_width(cr, 1.0);
    cairo_arc(cr, g->target.x, g->target.y, threshold, 0, G_PI * 2);
    cairo_stroke(cr);

    // 5. Text info under ID
    const char* stateName = "UNKNOWN";
    switch(g->state) {
        case GooseState::WANDER: stateName = "WANDER"; break;
        case GooseState::FETCHING: stateName = "FETCHING"; break;
        case GooseState::RETURNING: stateName = "RETURNING"; break;
        case GooseState::CHASE_CURSOR: stateName = "CHASE"; break;
        case GooseState::SNATCH_CURSOR: stateName = "SNATCH"; break;
    }
    float dist = Vector2::Distance(origin, g->target);
    char infoBuf[64];
    snprintf(infoBuf, sizeof(infoBuf), "%s (d:%.0f/%.0f)", stateName, dist, threshold);
    
    cairo_set_source_rgba(cr, 1, 1, 1, 0.7);
    cairo_move_to(cr, g->pos.x + 15, g->pos.y - 10);
    PangoLayout* infoLayout = pango_cairo_create_layout(cr);
    pango_layout_set_text(infoLayout, infoBuf, -1);
    pango_cairo_show_layout(cr, infoLayout);
    g_object_unref(infoLayout);

    // 6. Draw Beak Tip
    Vector2 bt = g->GetBeakTipDevice();
    cairo_set_source_rgba(cr, 1.0f, 0.5f, 0.0f, 0.8f);
    cairo_arc(cr, bt.x, bt.y, 4, 0, G_PI * 2);
    cairo_fill(cr);

    // Enhanced Path Visualization: Predictive Curve Simulation
    Vector2 simPos = origin;
    Vector2 simVel = (Vector2::Length(g->vel) < 1.0f) ? (Vector2::Normalize(g->target - origin) * g->currentSpeed) : g->vel;
    float simDt = 0.1f;
    
    cairo_set_source_rgba(cr, 1, 1, 1, 0.3);
    for (int i = 0; i < 15; i++) {
        Vector2 toTarget = g->target - simPos;
        float d = Vector2::Length(toTarget);
        if (d < 10.0f) break;

        Vector2 desired = Vector2::Normalize(toTarget) * g->currentSpeed;
        Vector2 seek = (desired - simVel) * 2.0f;
        
        Vector2 tangent{ -Vector2::Normalize(simVel).y, Vector2::Normalize(simVel).x };
        float curveFade = std::min(1.0f, d / 200.0f);
        Vector2 curve = tangent * (g->parabolicCurvature * g->currentSpeed * 0.8f * curveFade);
        
        Vector2 force = seek + curve;
        simVel = simVel + force * simDt;
        float s = Vector2::Length(simVel);
        if (s > g->currentSpeed) simVel = simVel * (g->currentSpeed / s);
        
        Vector2 nextSimPos = simPos + simVel * simDt;
        cairo_move_to(cr, simPos.x, simPos.y);
        cairo_line_to(cr, nextSimPos.x, nextSimPos.y);
        cairo_stroke(cr);
        
        simPos = nextSimPos;
        if (Vector2::Distance(simPos, origin) > 2000.0f) break;
    }

    cairo_restore(cr);
}
"""

with open('src/platform/linux/ui_debug.h', 'a') as f:
    f.write('struct Goose;\nstruct Vector2;\nvoid draw_goose_debug_visuals(cairo_t* cr, Goose* g, const Vector2& origin);\n')

with open('src/platform/linux/ui_debug.cpp', 'a') as f:
    f.write('\n#include "ui_debug.h"\n#include "goose.h"\n#include "config.h"\n#include <cmath>\n\n')
    f.write(debug_func)

# Replace the block in draw_overlay_code with a call to our new function
new_call = """        if (g_config.debugVisuals) {
            Vector2 origin = g->pos;
            if (g->state == GooseState::FETCHING || g->state == GooseState::RETURNING || g->state == GooseState::CHASE_CURSOR) {
                origin = g->GetBeakTipDevice();
            }
            draw_goose_debug_visuals(cr, g, origin);
        }
"""
draw_overlay_code = draw_overlay_code.replace(debug_code, new_call)
# Change "g." to "g->" in draw_overlay_code for Goose pointer
draw_overlay_code = draw_overlay_code.replace('g.', 'g->')

# Remove constants related to debug from ui.cpp, or just leave them.

# Replace draw_overlay in ui_drawing.cpp
with open('src/platform/linux/ui_drawing.cpp', 'r') as f:
    ui_drawing_cpp = f.read()

# Since we want to use the updated draw_overlay from ui.cpp, we just append it or replace the old one
old_do_start = ui_drawing_cpp.find('void draw_overlay(GtkDrawingArea*')
if old_do_start != -1:
    ui_drawing_cpp = ui_drawing_cpp[:old_do_start]

# Add our newly extracted functions
ui_drawing_cpp += '\n// --- Extracted from ui.cpp ---\n'
ui_drawing_cpp += draw_footprints_code
ui_drawing_cpp += draw_dropped_items_code
ui_drawing_cpp += draw_overlay_code

# To ensure it compiles, we need to make DrawFootprints and DrawDroppedItems not static, or just append them to the end of ui_drawing.cpp. We made them static in ui.cpp, but here they are just file-local to ui_drawing.cpp, so it's fine.
ui_drawing_cpp = ui_drawing_cpp.replace('static void DrawFootprints', 'void DrawFootprints')
ui_drawing_cpp = ui_drawing_cpp.replace('static void DrawDroppedItems', 'void DrawDroppedItems')

with open('src/platform/linux/ui_drawing.cpp', 'w') as f:
    f.write(ui_drawing_cpp)

# Clean up ui.cpp
ui_cpp = ui_cpp.replace(draw_footprints_code, '')
ui_cpp = ui_cpp.replace(draw_dropped_items_code, '')
ui_cpp = ui_cpp.replace(draw_overlay_code, '')
ui_cpp = ui_cpp.replace(on_tick_code, '')
ui_cpp = '#include "ui_drawing.h"\n#include "ui_tick.h"\n#include "ui_debug.h"\n' + ui_cpp

with open('src/platform/linux/ui.cpp', 'w') as f:
    f.write(ui_cpp)
