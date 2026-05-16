#include "ui_drawing.h"
#include "ui.h"
#include "world.h"
#include "config.h"
#include "assets.h"
#include "goose.h"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <pango/pangocairo.h>

void draw_debug_overlay(cairo_t* cr, int width, int height);
static void draw_dropped_item(cairo_t* cr, const DroppedItem& item, int screenH);
static void draw_goose(cairo_t* cr, const Goose& g, int screenH);
static void draw_footprints(cairo_t* cr, int screenH);
static void draw_mud(cairo_t* cr, const Mud& m, int screenH);
static void draw_debug(cairo_t* cr, int screenW, int screenH);

static PangoLayout* s_pangoLayout = nullptr;

void DrawingInit() {
    if (!s_pangoLayout) {
        s_pangoLayout = pango_cairo_create_layout(cr);
    }
}

void DrawingShutdown() {
    if (s_pangoLayout) {
        g_object_unref(s_pangoLayout);
        s_pangoLayout = nullptr;
    }
}

static const char* StateName(GooseState s) {
    switch (s) {
        case GooseState::WANDER: return "Wander";
        case GooseState::MUD: return "Mud";
        case GooseState::SNATCH_CURSOR: return "Snatch";
        case GooseState::CHASE: return "Chase";
        default: return "?";
    }
}

static void apply_goose_transform(cairo_t* cr, const Goose& g) {
    cairo_translate(cr, g.pos.x, g.pos.y);
    // facing might not be updated correctly when resting
    cairo_rotate(cr, g.dir * (M_PI / 180.0));
    if (g.isResting) {
        cairo_scale(cr, g_config.general.globalScale, g_config.general.globalScale * 0.5); // Flatten
    } else {
        cairo_scale(cr, g_config.general.globalScale, g_config.general.globalScale);
    }
}

static void draw_goose_sprite(cairo_t* cr, const Goose& g, const char* base_name, float alpha) {
    cairo_surface_t* base = g_assets.Get(base_name);
    if (!base) return;

    cairo_save(cr);
    apply_goose_transform(cr, g);

    cairo_set_source_surface(cr, base, -g_assets.GetWidth(base_name) / 2.0, -g_assets.GetHeight(base_name) / 2.0);
    cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
}

static void draw_wing_animation(cairo_t* cr, const Goose& g, float t) {
    cairo_surface_t* wing = g_assets.Get("wing.png");
    if (!wing) return;

    cairo_save(cr);
    apply_goose_transform(cr, g);

    double flap = std::sin(t * 12.0) * 0.25;
    cairo_translate(cr, -18, -5);
    cairo_rotate(cr, flap);

    cairo_set_source_surface(cr, wing, -g_assets.GetWidth("wing.png") / 2.0, -g_assets.GetHeight("wing.png") / 2.0);
    cairo_paint_with_alpha(cr, 0.9);
    cairo_restore(cr);
}

static void draw_beak(cairo_t* cr, const Goose& g) {
    cairo_surface_t* beak = g_assets.Get("beak.png");
    if (!beak) return;

    cairo_save(cr);
    apply_goose_transform(cr, g);
    cairo_translate(cr, 30, -2);
    cairo_set_source_surface(cr, beak, -g_assets.GetWidth("beak.png") / 2.0, -g_assets.GetHeight("beak.png") / 2.0);
    cairo_paint_with_alpha(cr, 0.95);
    cairo_restore(cr);
}

static void draw_footprints(cairo_t* cr, int screenH) {
    const auto now = std::chrono::steady_clock::now();
    for (const auto& fp : g_footprints) {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - fp.timestamp).count();
        float alpha = 1.0f - (float)age / fp.lifetime;
        if (alpha <= 0) continue;

        cairo_save(cr);
        cairo_set_source_rgba(cr, 0.3, 0.25, 0.2, alpha * 0.35);

        cairo_surface_t* ftsprite = g_assets.Get("footprint.png");
        if (ftsprite) {
            cairo_translate(cr, fp.pos.x, screenH - fp.pos.y);
            cairo_rotate(cr, fp.rot);
            cairo_set_source_surface(cr, ftsprite, -g_assets.GetWidth("footprint.png") / 2.0, -g_assets.GetHeight("footprint.png") / 2.0);
            cairo_paint(cr);
        } else {
            cairo_arc(cr, fp.pos.x, screenH - fp.pos.y, 6, 0, 2 * M_PI);
            cairo_fill(cr);
        }
        cairo_restore(cr);
    }
}

static void draw_mud(cairo_t* cr, const Mud& m, int screenH) {
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.25, 0.18, 0.08, m.alpha);
    cairo_translate(cr, m.x, screenH - m.y);
    cairo_ellipse(cr, 0, 0, m.w / 2.0, m.h / 2.0);
    cairo_fill(cr);
    cairo_restore(cr);
}

static void draw_dropped_item(cairo_t* cr, const DroppedItem& item, int screenH) {
    float scale = g_config.general.globalScale;
    float itemW = item.data->w * scale;
    float itemH = item.data->h * scale;
    cairo_save(cr);
    cairo_set_source_surface(cr, item.data->surface, item.pos.x - itemW / 2.0, screenH - item.pos.y - itemH / 2.0);
    cairo_paint_with_alpha(cr, item.alpha);
    cairo_restore(cr);
}

static void draw_goose(cairo_t* cr, const Goose& g, int screenH) {
    if (g.pos.x < -100 || g.pos.x > g_config.screen.width + 100 ||
        g.pos.y < -100 || g.pos.y > g_config.screen.height + 100)
        return;

    const bool selected = (g.id == g_selectedGooseId);
    const bool chasing = (g.state == GooseState::CHASE);
    const bool snatching = (g.state == GooseState::SNATCH_CURSOR);

    if (chasing || snatching) {
        draw_goose_sprite(cr, g, "goose_attack.png", 0.95f);
    } else if (selected) {
        draw_goose_sprite(cr, g, "goose_selected.png", 0.95f);
    } else {
        draw_goose_sprite(cr, g, "goose.png", 0.9f);
    }

    if (chasing || snatching || selected) {
        draw_wing_animation(cr, g, g.stepTime * 0.5f);
        draw_beak(cr, g);
    }

    if (selected && !chasing && !snatching) {
        cairo_save(cr);
        cairo_translate(cr, g.pos.x, screenH - g.pos.y);
        cairo_set_source_rgb(cr, 1, 1, 0);
        cairo_set_line_width(cr, 2);
        cairo_ellipse(cr, 0, 0, g_assets.GetWidth("goose.png") * 0.55 * g.scale, g_assets.GetHeight("goose.png") * 0.55 * g.scale);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

static void draw_mud_layer(cairo_t* cr, int screenH) {
    for (const auto& m : g_mudPatches) {
        draw_mud(cr, m, screenH);
    }
    for (const auto& g : g_geese) {
        if (g.state == GooseState::MUD && g.activeMud) {
            draw_mud(cr, *g.activeMud, screenH);
        }
    }
}

static void draw_cursor_grab_indicator(cairo_t* cr, int screenH) {
    if (g_cursorGrabberId == -1 || !g_cursorPosSet) return;

    const Goose* g = nullptr;
    for (const auto& gx : g_geese) {
        if (gx.id == g_cursorGrabberId) { g = &gx; break; }
    }
    if (!g) return;

    cairo_save(cr);
    cairo_set_source_rgb(cr, 1, 0.2, 0.2);
    cairo_set_line_width(cr, 2);

    cairo_translate(cr, g_cursorPos.x, screenH - g_cursorPos.y);
    cairo_rotate(g_cursorAngle);
    cairo_move_to(cr, -10, 0);
    cairo_line_to(cr, 10, 0);
    cairo_move_to(cr, 0, -10);
    cairo_line_to(cr, 0, 10);
    cairo_stroke(cr);

    cairo_restore(cr);
}

static void draw_debug(cairo_t* cr, int screenW, int screenH) {
    if (!g_config.debugOverlayEnabled) return;

    const char* filter = g_config.debugShowSelectedOnly ? "Selected" : nullptr;
    if (filter) {
        if (g_selectedGooseId == 0) return;
        for (const auto& g : g_geese) {
            if (g.id == g_selectedGooseId) {
                int y = screenH - 60;
                cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 14);
                cairo_set_source_rgb(cr, 1, 1, 1);
                char buf[256];
                snprintf(buf, sizeof(buf), "[#%d] Pos(%.0f,%.0f) Vel(%.1f,%.1f) State=%s Step=%.2f",
                    g.id, g.pos.x, g.pos.y, g.vel.x, g.vel.y, StateName(g.state), g.stepTime);
                cairo_move_to(cr, 12, y);
                cairo_show_text(cr, buf);
                return;
            }
           }
    }

    char buf[512];
    int y = 16;
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 13);
    cairo_set_source_rgb(cr, 1, 1, 0.8);

    snprintf(buf, sizeof(buf), "Geese: %zu  Mud: %zu  Items: %zu  FPS: %.0f  RAM: %.1f MB",
        g_geese.size(), g_mudPatches.size(), g_droppedItems.size(), 1.0 / g_frameDelta, g_ramMB);
    cairo_move_to(cr, 12, y += 18);
    cairo_show_text(cr, buf);

    snprintf(buf, sizeof(buf), "Cursor: (%.0f, %.0f)  Grabber: #%d",
        g_cursorPos.x, g_cursorPos.y, g_cursorGrabberId);
    cairo_move_to(cr, 12, y += 18);
    cairo_show_text(cr, buf);

    cairo_set_source_rgb(cr, 0.7, 1, 0.7);
    snprintf(buf, sizeof(buf), "Spawn: %d  Mud: %d/%d  Cursor: %d/%d  Meme: %d  Note: %d",
        g_config.spawnEnabled ? 1 : 0, g_config.mudEnabled ? 1 : 0,
        g_config.mud.maxPatches, g_config.cursorEnabled ? 1 : 0,
        g_config.cursor.maxGrabbers, g_config.memeEnabled ? 1 : 0,
        g_config.noteEnabled ? 1 : 0);
    cairo_move_to(cr, 12, y += 18);
    cairo_show_text(cr, buf);

    if (!g_uiLog.empty()) {
        cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
        snprintf(buf, sizeof(buf), "LOG: %s", g_uiLog.back().c_str());
        cairo_move_to(cr, 12, y += 18);
        cairo_show_text(cr, buf);
    }

    if (g_config.debugVerbose && !g_debugPath.empty()) {
        cairo_set_source_rgba(cr, 0, 1, 0, 0.5);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, g_debugPath[0].x, screenH - g_debugPath[0].y);
        for (size_t i = 1; i < g_debugPath.size(); ++i) {
            cairo_line_to(cr, g_debugPath[i].x, screenH - g_debugPath[i].y);
        }
        cairo_stroke(cr);
    }
}

static void render_goose_cursor(cairo_t* cr, int screenH) {
    if (g_cursorGrabberId == -1 || !g_cursorPosSet) return;

    for (const auto& g : g_geese) {
        if (g.id != g_cursorGrabberId) continue;

        const char* name = "cursor_goose.png";
        cairo_surface_t* surface = g_assets.Get(name);
        if (!surface) return;

        cairo_save(cr);
        cairo_translate(cr, g_cursorPos.x, screenH - g_cursorPos.y);
        cairo_rotate(g_cursorAngle + M_PI);
        cairo_set_source_surface(cr, surface, -g_assets.GetWidth(name) / 2.0, -g_assets.GetHeight(name) / 2.0);
        cairo_paint_with_alpha(cr, 0.9);
        cairo_restore(cr);
    }
}

static void draw_dropped_items(cairo_t* cr, int screenH) {
    for (const auto& item : g_droppedItems) {
        draw_dropped_item(cr, item, screenH);
    }
}

void draw_overlay(GtkDrawingArea*, cairo_t* cr, int width, int height, gpointer data) {
    MonitorInfo* m = (MonitorInfo*)data;
    int screenH = m ? m->height : height;

    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_paint(cr);

    draw_mud_layer(cr, screenH);
    draw_footprints(cr, screenH);
    draw_dropped_items(cr, screenH);

    for (const auto& g : g_geese) {
        draw_goose(cr, g, screenH);
    }

    render_goose_cursor(cr, screenH);
    draw_cursor_grab_indicator(cr, screenH);

    draw_debug(cr, width, screenH);
}

void draw_debug_overlay(cairo_t* cr, int width, int height) {
    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    cairo_paint(cr);
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_move_to(cr, width / 2.0 - 100, height / 2.0);
    cairo_show_text(cr, "DEBUG OVERLAY");
}