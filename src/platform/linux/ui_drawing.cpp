#include "ui_drawing.h"
#include "ui.h"
#include "world.h"
#include "config.h"
#include "assets.h"
#include "goose.h"
#include "actor.h"
#include "actor_dropped_item.h"
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
    // PangoLayout is created on first draw when we have a valid cairo context
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
    for (const auto& fp : g_world.footprints) {
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

    const bool selected = (g.id == g_world.selectedGooseId);
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
    for (const auto* g : ActorManager::Instance().getGeese()) {
        if (g->state == GooseState::MUD && g->activeMud) {
            draw_mud(cr, *g->activeMud, screenH);
        }
    }
}

static void draw_cursor_grab_indicator(cairo_t* cr, int screenH) {
    if (g_world.cursorGrabberId == -1 || !g_cursorPosSet) return;

    const Goose* g = nullptr;
    for (const auto* gx : ActorManager::Instance().getGeese()) {
        if (gx->id == g_world.cursorGrabberId) { g = gx; break; }
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
    if (!g_config.debug.visuals) return;

    const bool selectedOnly = (g_world.selectedGooseId != 0);
    if (selectedOnly) {
        for (const auto* g : ActorManager::Instance().getGeese()) {
            if (g->id == g_world.selectedGooseId) {
                int y = screenH - 60;
                cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 14);
                cairo_set_source_rgb(cr, 1, 1, 1);
                char buf[256];
                snprintf(buf, sizeof(buf), "[#%d] Pos(%.0f,%.0f) Vel(%.1f,%.1f) State=%s Step=%.2f",
                    g->id, g->pos.x, g->pos.y, g->vel.x, g->vel.y, StateName(g->state), g->stepTime);
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
        ActorManager::Instance().getGeese().size(), g_mudPatches.size(), ActorManager::Instance().getDroppedItems().size(), 1.0 / g_frameDelta, g_ramMB);
    cairo_move_to(cr, 12, y += 18);
    cairo_show_text(cr, buf);

    snprintf(buf, sizeof(buf), "Cursor: (%.0f, %.0f)  Grabber: #%d",
        g_cursorPos.x, g_cursorPos.y, g_world.cursorGrabberId);
    cairo_move_to(cr, 12, y += 18);
    cairo_show_text(cr, buf);

    cairo_set_source_rgb(cr, 0.7, 1, 0.7);
    snprintf(buf, sizeof(buf), "Scale: %.2f  Mud: %d/%d  Cursor: %d/%d  Memes: %s",
        g_config.general.globalScale, (int)g_config.mud.enabled,
        g_config.mud.maxPatches, (int)g_config.cursor.chaseEnabled,
        g_config.cursor.maxGrabbers, g_config.general.memesEnabled ? "on" : "off");
    cairo_move_to(cr, 12, y += 18);
    cairo_show_text(cr, buf);

    if (!g_world.uiLog.empty()) {
        cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
        snprintf(buf, sizeof(buf), "LOG: %s", g_world.uiLog.back().c_str());
        cairo_move_to(cr, 12, y += 18);
        cairo_show_text(cr, buf);
    }
}

static void render_goose_cursor(cairo_t* cr, int screenH) {
    if (g_world.cursorGrabberId == -1 || !g_cursorPosSet) return;

    for (const auto* g : ActorManager::Instance().getGeese()) {
        if (g->id != g_world.cursorGrabberId) continue;

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
    auto items = ActorManager::Instance().getDroppedItems();
    for (auto* actor : items) {
        draw_dropped_item(cr, actor->item(), screenH);
    }
}


// --- Extracted from ui.cpp ---
void DrawFootprints(cairo_t* cr) {
    for (auto& fp : g_world.footprints) {
        double age = g_time - fp.timeSpawned;
        float alpha = kFootprintAlpha;
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        float fadeStart = life * 0.7f;
        float fadeDur = life - fadeStart;
        if (age > fadeStart) alpha = kFootprintAlpha * (1.0f - (float)(age - fadeStart) / fadeDur);
        if (alpha <= 0) continue;

        cairo_save(cr);
        cairo_translate(cr, fp.pos.x, fp.pos.y);
        cairo_scale(cr, g_config.general.globalScale, g_config.general.globalScale);
        cairo_rotate(cr, fp.dir * G_PI / 180.0);
        
        cairo_set_source_rgba(cr, kFootprintPadR, kFootprintPadG, kFootprintPadB, alpha);
        
        cairo_save(cr);
        cairo_scale(cr, kFootprintPadScaleX, kFootprintPadScaleY);
        cairo_arc(cr, 0, 0, 1.0, 0, 2*G_PI);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_translate(cr, -kFootprintToeOffsetX, -kFootprintToeOffsetY);
        cairo_scale(cr, kFootprintToeScaleX, kFootprintToeScaleY);
        cairo_arc(cr, 0, 0, 1.0, 0, 2*G_PI);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_translate(cr, kFootprintToeOffsetX, -kFootprintToeOffsetY);
        cairo_scale(cr, kFootprintToeScaleX, kFootprintToeScaleY);
        cairo_arc(cr, 0, 0, 1.0, 0, 2*G_PI);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_restore(cr);
    }
}

void DrawDroppedItems(cairo_t* cr) {
    auto items = ActorManager::Instance().getDroppedItems();
    for (auto* actor : items) {
        DroppedItem& item = actor->item();
        if (!std::isfinite(item.pos.x) || !std::isfinite(item.pos.y) || !std::isfinite(item.rotation)) {
            continue; // Skip corrupted items
        }

        cairo_save(cr);
        float s = g_config.general.globalScale;
        // Center the coordinate system on the item, applying global scale
        cairo_translate(cr, item.pos.x + item.data->w * 0.5f * s, item.pos.y + item.data->h * 0.5f * s);
        cairo_scale(cr, s, s);
        cairo_rotate(cr, item.rotation);
        
        if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
            std::cerr << "[UI] Cairo error after item transform: " << cairo_status_to_string(cairo_status(cr)) << std::endl;
            cairo_restore(cr);
            continue;
        }

        // Draw from top-left relative to scaled center
        cairo_translate(cr, -item.data->w * 0.5f, -item.data->h * 0.5f);

        if (item.data->type == ItemData::MEME && item.data->pixbuf) {
            set_source_pixbuf_manual(cr, item.data->pixbuf, 0, 0);
            cairo_paint(cr);
        } else if (item.data->type == ItemData::TEXT) {
            cairo_set_source_rgb(cr, 1, 1, 0.85);
            cairo_rectangle(cr, 0, 0, item.data->w, item.data->h);
            cairo_fill_preserve(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_set_line_width(cr, 2);
            cairo_stroke(cr);

            PangoLayout* layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(layout, item.data->Text().c_str(), -1);
            pango_layout_set_width(layout, (item.data->w - kLinuxTextPadding) * PANGO_SCALE);
            cairo_move_to(cr, kLinuxTextPadding / 2, kLinuxTextPadding / 2);
            pango_cairo_show_layout(cr, layout);
            g_object_unref(layout);
        }
        cairo_restore(cr);

        // Visual debug for items: draw a point at their center
        if (g_config.debug.visuals) {
            cairo_save(cr);
            Vector2 center = item.pos + Vector2{(float)item.data->w * 0.5f, (float)item.data->h * 0.5f} * s;
            cairo_set_source_rgba(cr, kDebugOrangeR, kDebugOrangeG, kDebugOrangeB, kDebugOrangeAlpha); // Orange point
            cairo_arc(cr, center.x, center.y, kDebugPointRadius, 0, G_PI * 2);
            cairo_fill(cr);
            
            // Label for time left
            char timeBuf[16];
            snprintf(timeBuf, sizeof(timeBuf), "%.1fs", g_config.item.itemLifetime - (g_time - item.timeDropped));
            cairo_move_to(cr, center.x + 8, center.y + 8);
            PangoLayout* tLayout = pango_cairo_create_layout(cr);
            pango_layout_set_text(tLayout, timeBuf, -1);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.6);
            pango_cairo_show_layout(cr, tLayout);
            g_object_unref(tLayout);
            cairo_restore(cr);
        }
    }
}

// --- Drawing (see ui_drawing.cpp) -----------------------------------------
void draw_overlay(GtkDrawingArea* area, cairo_t* cr, int width, int height, gpointer data) {
    MonitorInfo* m = (MonitorInfo*)data;
    if (!m || cairo_status(cr) != CAIRO_STATUS_SUCCESS) return;

    if (!g_config.cursor.multiMonitorEnabled && (m->x != 0 || m->y != 0)) {
        cairo_save(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr);
        cairo_restore(cr);
        return;
    }

    cairo_save(cr); 
    cairo_translate(cr, -m->x, -m->y);

    DrawFootprints(cr);
    DrawDroppedItems(cr);

    // Draw Geese
    for (auto* g : ActorManager::Instance().getGeese()) {
        if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) break;
        g->Draw(cr);

        // Minecraft-style name tag
        if (!g->name.empty()) {
            cairo_save(cr);
            PangoLayout* layout = pango_cairo_create_layout(cr);
            std::string tagText = "[#" + std::to_string(g->id) + "] " + g->name;
            pango_layout_set_text(layout, tagText.c_str(), -1);
            
            PangoFontDescription* desc = pango_font_description_from_string("Sans Bold 10");
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            int tw, th;
            pango_layout_get_pixel_size(layout, &tw, &th);

            float tagX = g->pos.x - tw / 2.0f;
            float tagY = g->pos.y - kLinuxNametagYOffset * g_config.general.globalScale; // Position above head

            // Background rectangle (translucent dark)
            cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
            cairo_rectangle(cr, tagX - kLinuxNametagPadX, tagY - kLinuxNametagPadY, tw + kLinuxNametagPadW, th + kLinuxNametagPadH);
            cairo_fill(cr);

            // White text
            cairo_set_source_rgb(cr, 1, 1, 1);
            cairo_move_to(cr, tagX, tagY);
            pango_cairo_show_layout(cr, layout);

            g_object_unref(layout);
            cairo_restore(cr);
        }

        if (g_config.debug.visuals) {
            Vector2 origin = g->pos;
            if (g->state == GooseState::FETCHING || g->state == GooseState::RETURNING || g->state == GooseState::CHASE_CURSOR) {
                origin = g->GetBeakTipDevice();
            }
            draw_goose_debug_visuals(cr, g, origin);
        }
    }
    cairo_restore(cr); 

    // Debug overlay (only on the primary monitor window)
    if (m->x == 0 && m->y == 0) {
        draw_debug_overlay(cr);
    }
}

