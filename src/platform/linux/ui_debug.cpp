#include "ui_debug.h"
#include "world.h"
#include "config.h"
#include "goose.h"
#include "actor.h"
#include "actor_dropped_item.h"
#include <cmath>
#include <vector>
#include <cstdio>

bool g_debugOverlayVerbose = false;
bool g_debugOverlaySelectedOnly = false;

void UiLogTrim();
const char* GetGooseStateStr(GooseState s);

void draw_debug_overlay(cairo_t* cr, int width, int height, bool verbose, bool selectedOnly) {
    if (!g_config.debug.toTerminal) return;

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

    snprintf(buf, sizeof(buf), "Goose Debug | geese:%zu  items:%zu  t:%.2f", ActorManager::Instance().getGeese().size(), ActorManager::Instance().getDroppedItems().size(), g_time);
    lines.emplace_back(buf);

    snprintf(buf, sizeof(buf), "Config | scale:%.2f  walk:%.0f  run:%.0f  memes:%s  cursorDefaults:%s (%d%%)  snatch:%.1fs",
             g_config.general.globalScale,
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
             g_world.cursorGrabberId,
             g_world.selectedGooseId);
    lines.emplace_back(buf);

    const bool filterSelected = selectedOnly && (GetGooseById(g_world.selectedGooseId) != nullptr);

    for (auto* goose : ActorManager::Instance().getGeese()) {
        if (filterSelected && goose->id != g_world.selectedGooseId) continue;

        const char* stateStr = GetGooseStateStr(goose->state);
        const float vmag = (float)std::sqrt(goose->vel.x * goose->vel.x + goose->vel.y * goose->vel.y);
        const char* heldStr = goose->heldItem ? (goose->heldItem->type == ItemData::MEME ? "MEME" : "NOTE") : "none";
        const bool isGrabber = (g_world.cursorGrabberId == goose->id);
        const float distToTarget = Vector2::Distance(goose->pos, goose->target);
        const bool posNaN = std::isnan(goose->pos.x) || std::isnan(goose->pos.y);
        const bool velNaN = std::isnan(goose->vel.x) || std::isnan(goose->vel.y);
        const char* warnStr = (posNaN || velNaN) ? " !!NaN!!" : "";

        snprintf(buf, sizeof(buf), "ID %d%s%s | %s  pos:(%.1f,%.1f)  vel:(%.1f,%.1f)=%.1f  dir:%.0f  spd:%.1f  held:%s",
                 goose->id, isGrabber ? "*" : "", warnStr,
                 stateStr,
                 goose->pos.x, goose->pos.y,
                 goose->vel.x, goose->vel.y, vmag,
                 goose->dir,
                 goose->currentSpeed,
                 heldStr);
        lines.emplace_back(buf);

        snprintf(buf, sizeof(buf), "    target:(%.1f,%.1f)  dist:%.1f  biases mouse:%d%% meme:%d%% note:%d%%",
                 goose->target.x, goose->target.y, distToTarget,
                 goose->attackMouseBias, goose->memeFetchBias, goose->noteFetchBias);
        lines.emplace_back(buf);

        if (verbose) {
            const float dragVmag = (float)std::sqrt(goose->dragVel.x * goose->dragVel.x + goose->dragVel.y * goose->dragVel.y);
            snprintf(buf, sizeof(buf), "    drag init:%s  pos:(%.1f,%.1f)  vel:(%.1f,%.1f)=%.1f  rot:%.2f  rotVel:%.2f",
                     goose->dragInit ? "Y" : "N",
                     goose->dragPos.x, goose->dragPos.y,
                     goose->dragVel.x, goose->dragVel.y, dragVmag,
                     goose->dragRot, goose->dragRotVel);
            lines.emplace_back(buf);

            snprintf(buf, sizeof(buf), "    rig body:(%.1f,%.1f)  neckBase:(%.1f,%.1f)  neckHead:(%.1f,%.1f)  neckLerp:%.2f",
                     goose->rig.body.x, goose->rig.body.y,
                     goose->rig.neckBase.x, goose->rig.neckBase.y,
                     goose->rig.neckHead.x, goose->rig.neckHead.y,
                     goose->rig.neckLerp);
            lines.emplace_back(buf);

            snprintf(buf, sizeof(buf), "    feet L:(%.1f,%.1f) move:%s  R:(%.1f,%.1f) move:%s  stepTime:%.2f",
                     goose->rig.lFoot.currentPos.x, goose->rig.lFoot.currentPos.y,
                     goose->rig.lFoot.moveStartTime >= 0 ? "Y" : "N",
                     goose->rig.rFoot.currentPos.x, goose->rig.rFoot.currentPos.y,
                     goose->rig.rFoot.moveStartTime >= 0 ? "Y" : "N",
                     goose->stepTime);
            lines.emplace_back(buf);

            if (goose->state == GooseState::SNATCH_CURSOR) {
                const double heldFor = g_time - goose->snatchStartTime;
                snprintf(buf, sizeof(buf), "    snatch t:%.2fs  pull:%.0f  off:(%.0f,%.0f)  circle r:%.0f  w:%.2f  a:%.2f",
                         heldFor,
                         goose->snatchPullDistance,
                         goose->snatchOffset.x, goose->snatchOffset.y,
                         goose->snatchRadius,
                         goose->snatchAngularSpeed,
                         goose->snatchAngle);
                lines.emplace_back(buf);
            }
        }
    }

    snprintf(buf, sizeof(buf), "Render | screen:%dx%d  droppedItems:%zu", g_world.screenWidth, g_world.screenHeight, ActorManager::Instance().getDroppedItems().size());
    lines.emplace_back(buf);

    for (const auto& l : g_world.uiLog) lines.emplace_back(l);

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
    int maxBoxW = g_world.screenWidth - 40;
    if (boxW > maxBoxW) boxW = maxBoxW;
    int maxBoxH = g_world.screenHeight - 40;
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
#include "ui_debug.h"
#include "goose.h"
#include "config.h"
#include <cmath>

void draw_goose_debug_visuals(cairo_t* cr, Goose* g, const Vector2& origin) {
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
