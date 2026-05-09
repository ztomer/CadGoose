#include "ui_debug.h"
#include "world.h"
#include "config.h"
#include "goose.h"
#include <cmath>
#include <vector>
#include <cstdio>

bool g_debugOverlayVerbose = false;
bool g_debugOverlaySelectedOnly = false;

void UiLogTrim();
const char* GetGooseStateStr(GooseState s);

void draw_debug_overlay(cairo_t* cr, int width, int height, bool verbose, bool selectedOnly) {
    if (!g_config.debugToTerminal) return;

    UiLogTrim();

    cairo_save(cr);

    const int pad = 8;
    const int x = 20;
    const int y = 20;

    PangoLayout* layout = pango_cairo_create_layout(cr);
    PangoFontDescription* desc = pango_font_description_from_string("Monospace 11");
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    std::vector<std::string> lines;
    char buf[256];
    CursorState curState = g_cursorProvider ? g_cursorProvider->Read() : CursorState{};
    Vector2 cursorPos = (curState.caps & CAP_GET_POS) ? curState.position : Vector2{-1, -1};

    snprintf(buf, sizeof(buf), "Goose Debug | geese:%zu  items:%zu  t:%.2f", g_geese.size(), g_droppedItems.size(), g_time);
    lines.emplace_back(buf);

    snprintf(buf, sizeof(buf), "Config | scale:%.2f  walk:%.0f  run:%.0f  memes:%s  cursorDefaults:%s (%d%%)  snatch:%.1fs",
             g_config.globalScale,
             g_config.baseWalkSpeed,
             g_config.baseRunSpeed,
             g_config.memesEnabled ? "on" : "off",
             g_config.cursorChaseEnabled ? "on" : "off",
             g_config.cursorChaseChance,
             g_config.snatchDuration);
    lines.emplace_back(buf);

    snprintf(buf, sizeof(buf), "Cursor | %s  pos:(%.0f,%.0f)  grabber:%d  selected:%d",
             g_cursorProvider ? "Active" : "None",
             cursorPos.x, cursorPos.y,
             g_cursorGrabberId,
             g_selectedGooseId);
    lines.emplace_back(buf);

    const bool filterSelected = selectedOnly && (GetGooseById(g_selectedGooseId) != nullptr);

    for (auto& goose : g_geese) {
        if (filterSelected && goose.id != g_selectedGooseId) continue;

        const char* stateStr = GetGooseStateStr(goose.state);
        const float vmag = (float)std::sqrt(goose.vel.x * goose.vel.x + goose.vel.y * goose.vel.y);
        const char* heldStr = goose.heldItem ? (goose.heldItem->type == ItemData::MEME ? "MEME" : "NOTE") : "none";
        const bool isGrabber = (g_cursorGrabberId == goose.id);
        const float distToTarget = Vector2::Distance(goose.pos, goose.target);
        const bool posNaN = std::isnan(goose.pos.x) || std::isnan(goose.pos.y);
        const bool velNaN = std::isnan(goose.vel.x) || std::isnan(goose.vel.y);
        const char* warnStr = (posNaN || velNaN) ? " !!NaN!!" : "";

        snprintf(buf, sizeof(buf), "ID %d%s%s | %s  pos:(%.1f,%.1f)  vel:(%.1f,%.1f)=%.1f  dir:%.0f  spd:%.1f  held:%s",
                 goose.id, isGrabber ? "*" : "", warnStr,
                 stateStr,
                 goose.pos.x, goose.pos.y,
                 goose.vel.x, goose.vel.y, vmag,
                 goose.dir,
                 goose.currentSpeed,
                 heldStr);
        lines.emplace_back(buf);

        snprintf(buf, sizeof(buf), "    target:(%.1f,%.1f)  dist:%.1f  biases mouse:%d%% meme:%d%% note:%d%%",
                 goose.target.x, goose.target.y, distToTarget,
                 goose.attackMouseBias, goose.memeFetchBias, goose.noteFetchBias);
        lines.emplace_back(buf);

        if (verbose) {
            const float dragVmag = (float)std::sqrt(goose.dragVel.x * goose.dragVel.x + goose.dragVel.y * goose.dragVel.y);
            snprintf(buf, sizeof(buf), "    drag init:%s  pos:(%.1f,%.1f)  vel:(%.1f,%.1f)=%.1f  rot:%.2f  rotVel:%.2f",
                     goose.dragInit ? "Y" : "N",
                     goose.dragPos.x, goose.dragPos.y,
                     goose.dragVel.x, goose.dragVel.y, dragVmag,
                     goose.dragRot, goose.dragRotVel);
            lines.emplace_back(buf);

            snprintf(buf, sizeof(buf), "    rig body:(%.1f,%.1f)  neckBase:(%.1f,%.1f)  neckHead:(%.1f,%.1f)  neckLerp:%.2f",
                     goose.rig.body.x, goose.rig.body.y,
                     goose.rig.neckBase.x, goose.rig.neckBase.y,
                     goose.rig.neckHead.x, goose.rig.neckHead.y,
                     goose.rig.neckLerp);
            lines.emplace_back(buf);

            snprintf(buf, sizeof(buf), "    feet L:(%.1f,%.1f) move:%s  R:(%.1f,%.1f) move:%s  stepTime:%.2f",
                     goose.rig.lFoot.currentPos.x, goose.rig.lFoot.currentPos.y,
                     goose.rig.lFoot.moveStartTime >= 0 ? "Y" : "N",
                     goose.rig.rFoot.currentPos.x, goose.rig.rFoot.currentPos.y,
                     goose.rig.rFoot.moveStartTime >= 0 ? "Y" : "N",
                     goose.stepTime);
            lines.emplace_back(buf);

            if (goose.state == GooseState::SNATCH_CURSOR) {
                const double heldFor = g_time - goose.snatchStartTime;
                snprintf(buf, sizeof(buf), "    snatch t:%.2fs  pull:%.0f  off:(%.0f,%.0f)  circle r:%.0f  w:%.2f  a:%.2f",
                         heldFor,
                         goose.snatchPullDistance,
                         goose.snatchOffset.x, goose.snatchOffset.y,
                         goose.snatchRadius,
                         goose.snatchAngularSpeed,
                         goose.snatchAngle);
                lines.emplace_back(buf);
            }
        }
    }

    snprintf(buf, sizeof(buf), "Render | screen:%dx%d  droppedItems:%zu", g_screenWidth, g_screenHeight, g_droppedItems.size());
    lines.emplace_back(buf);

    for (const auto& l : g_uiLog) lines.emplace_back(l);

    int maxw = 0;
    int totalh = 0;
    for (const auto& l : lines) {
        pango_layout_set_text(layout, l.c_str(), -1);
        int pw = 0, ph = 0;
        pango_layout_get_pixel_size(layout, &pw, &ph);
        if (pw > maxw) maxw = pw;
        totalh += ph;
    }

    int boxW = maxw + pad * 2;
    int boxH = totalh + pad * 2;
    int maxBoxW = g_screenWidth - 40;
    if (boxW > maxBoxW) boxW = maxBoxW;
    int maxBoxH = g_screenHeight - 40;
    if (boxH > maxBoxH) boxH = maxBoxH;

    cairo_set_source_rgba(cr, 0.05, 0.05, 0.05, 0.75);
    cairo_rectangle(cr, x, y, boxW, boxH);
    cairo_fill(cr);

    int cy = y + pad;
    for (const auto& l : lines) {
        pango_layout_set_text(layout, l.c_str(), -1);
        cairo_move_to(cr, x + pad, cy);
        cairo_set_source_rgb(cr, 0.92, 0.92, 0.92);
        pango_cairo_show_layout(cr, layout);
        int pw = 0, ph = 0;
        pango_layout_get_pixel_size(layout, &pw, &ph);
        cy += ph;
        if (cy > y + boxH - pad) break;
    }

    g_object_unref(layout);
    cairo_restore(cr);
}