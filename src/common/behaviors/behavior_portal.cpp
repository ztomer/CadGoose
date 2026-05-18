// ===========================
// behavior_portal.cpp
// Portal Behavior - User-controlled portal placement
// Based on PortalGoos by Moonaliss1
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "hotkey.h"
#include "actor.h"
#include "actor_portal.h"

#ifdef __APPLE__
#include "renderer_interface.h"
#include "cg_renderer.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>

static bool s_portalsOn = true;
static bool s_p0Pressed = false;
static bool s_p1Pressed = false;
static bool s_p2Pressed = false;

static bool IsKeyHeld(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(ctx.goose->id, "portal");
    s_portalsOn = true;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(goose->id, "portal");
    auto& mgr = ActorManager::Instance();

    float p1w = g_config.portal.p1Width;
    float p1h = g_config.portal.p1Height;
    float p2w = g_config.portal.p2Width;
    float p2h = g_config.portal.p2Height;

    float distToP1 = std::hypot(goose->pos.x - state->portalA.x, goose->pos.y - state->portalA.y);
    float distToP2 = std::hypot(goose->pos.x - state->portalB.x, goose->pos.y - state->portalB.y);

    bool inP1 = goose->pos.x > state->portalA.x - p1w/2 && goose->pos.x < state->portalA.x + p1w/2 &&
                goose->pos.y > state->portalA.y - p1h/2 && goose->pos.y < state->portalA.y + p1h/2;
    bool inP2 = goose->pos.x > state->portalB.x - p2w/2 && goose->pos.x < state->portalB.x + p2w/2 &&
                goose->pos.y > state->portalB.y - p2h/2 && goose->pos.y < state->portalB.y + p2h/2;

    bool anyInPortal = inP1 || inP2;

    if (state->justTeleported) {
        if (!anyInPortal) {
            state->justTeleported = false;
        }
    } else if (s_portalsOn) {
        if (inP1 && state->portalB.active) {
            goose->pos.x = state->portalB.x;
            goose->pos.y = state->portalB.y;
            goose->vel = {0, 0};
            g_assets.Honk();
            state->justTeleported = true;
            fprintf(stderr, "[Portal] g%d teleported A->B at (%.0f,%.0f)\n", goose->id, state->portalB.x, state->portalB.y);
        } else if (inP2 && state->portalA.active) {
            goose->pos.x = state->portalA.x;
            goose->pos.y = state->portalA.y;
            goose->vel = {0, 0};
            g_assets.Honk();
            state->justTeleported = true;
            fprintf(stderr, "[Portal] g%d teleported B->A at (%.0f,%.0f)\n", goose->id, state->portalA.x, state->portalA.y);
        }
    }

    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
    CGPoint mousePos = {0, 0};
    bool haveMouse = false;
    if (source) {
        CGEventRef event = CGEventCreate(source);
        if (event) {
            mousePos = CGEventGetLocation(event);
            haveMouse = true;
            CFRelease(event);
        }
        CFRelease(source);
    }

    bool d1Pressed = IsKeyHeld(KeyNameToKeyCode(g_config.portal.hotkey1));
    bool d2Pressed = IsKeyHeld(KeyNameToKeyCode(g_config.portal.hotkey2));
    bool d0Pressed = IsKeyHeld(KeyNameToKeyCode(g_config.portal.hotkey0));

    if (d1Pressed && !s_p1Pressed) {
        s_p1Pressed = true;
        state->portalA.x = haveMouse ? (float)mousePos.x : goose->pos.x;
        state->portalA.y = haveMouse ? (float)mousePos.y : goose->pos.y;
        state->portalA.active = true;
        state->portalA.portalId = 1;
        fprintf(stderr, "[Portal] Portal 1 placed at (%.0f, %.0f)\n", state->portalA.x, state->portalA.y);

        // Update or create PortalActor A
        PortalActor* portalA = nullptr;
        for (int i = 0; i < mgr.totalCount(); i++) {
            Actor* a = mgr.getByIndex(i);
            if (a && strcmp(a->type(), "portal") == 0 && a->id() == 1) {
                portalA = static_cast<PortalActor*>(a);
                break;
            }
        }
        if (portalA) {
            portalA->position = {state->portalA.x, state->portalA.y};
            portalA->active = true;
        } else {
            portalA = new PortalActor(PortalActor::PortalA, {state->portalA.x, state->portalA.y});
            mgr.add(portalA);
        }
    } else if (!d1Pressed) {
        s_p1Pressed = false;
    }
    if (d2Pressed && !s_p2Pressed) {
        s_p2Pressed = true;
        state->portalB.x = haveMouse ? (float)mousePos.x : goose->pos.x;
        state->portalB.y = haveMouse ? (float)mousePos.y : goose->pos.y;
        state->portalB.active = true;
        state->portalB.portalId = 2;
        fprintf(stderr, "[Portal] Portal 2 placed at (%.0f, %.0f)\n", state->portalB.x, state->portalB.y);

        // Update or create PortalActor B
        PortalActor* portalB = nullptr;
        for (int i = 0; i < mgr.totalCount(); i++) {
            Actor* a = mgr.getByIndex(i);
            if (a && strcmp(a->type(), "portal") == 0 && a->id() == 2) {
                portalB = static_cast<PortalActor*>(a);
                break;
            }
        }
        if (portalB) {
            portalB->position = {state->portalB.x, state->portalB.y};
            portalB->active = true;
        } else {
            portalB = new PortalActor(PortalActor::PortalB, {state->portalB.x, state->portalB.y});
            mgr.add(portalB);
        }
    } else if (!d2Pressed) {
        s_p2Pressed = false;
    }
    if (d0Pressed && !s_p0Pressed) {
        s_portalsOn = !s_portalsOn;
        s_p0Pressed = true;
        fprintf(stderr, "[Portal] Portals %s\n", s_portalsOn ? "ON" : "OFF");
    } else if (!d0Pressed) {
        s_p0Pressed = false;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    (void)goose; (void)ctx; (void)renderCtx;
}

static Behavior g_portalBehavior = BEHAVIOR_DEF_STARTER(
    "portal", "Portal", "Press 1/2 to place portals at cursor, 0 to toggle. Based on PortalGoos by Moonaliss1",
    g_config.behaviors.control.portals, init, tick, render
);

REGISTER_BEHAVIOR(g_portalBehavior);
#else
// Linux stub
#endif
