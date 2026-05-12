// ===========================
// behavior_portal.cpp
// Portal Behavior - User-controlled portal placement
// Based on PortalGoos by Moonaliss1
// Reference: P+1 places portal 1, P+2 places portal 2, P+0 toggles portals on/off
// Uses p1.png and p2.png images
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>

static bool s_enabled = true;
static bool s_portalsOn = true;
static bool s_p0Pressed = false;
static CGImageRef s_portalImages[2] = {nullptr, nullptr};

enum PortalKey { P_KEY = 0x50, D1_KEY = 0x31, D2_KEY = 0x32, D0_KEY = 0x30 };

static bool IsKeyHeld(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

static void init(BehaviorContext& ctx) {
    if (!s_portalImages[0]) {
        s_portalImages[0] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/p1.png");
        s_portalImages[1] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/p2.png");
    }
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(ctx.goose->id, "portal");
    state->Reset();
    s_portalsOn = true;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.control.portals) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(goose->id, "portal");

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
        } else if (inP2 && state->portalA.active) {
            goose->pos.x = state->portalA.x;
            goose->pos.y = state->portalA.y;
            goose->vel = {0, 0};
            g_assets.Honk();
            state->justTeleported = true;
        }
    }

    bool pHeld = IsKeyHeld(0x50);

    if (pHeld) {
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
        if (source) {
            CGEventRef event = CGEventCreate(source);
            CFRelease(source);
            if (event) {
                CGPoint mousePos = CGEventGetLocation(event);
                CFRelease(event);

                bool d1Pressed = IsKeyHeld(0x31);
                bool d2Pressed = IsKeyHeld(0x32);
                bool d0Pressed = IsKeyHeld(0x30);

                if (d1Pressed) {
                    state->portalA.x = (float)mousePos.x;
                    state->portalA.y = (float)mousePos.y;
                    state->portalA.active = true;
                    state->portalA.portalId = 1;
                }
                if (d2Pressed) {
                    state->portalB.x = (float)mousePos.x;
                    state->portalB.y = (float)mousePos.y;
                    state->portalB.active = true;
                    state->portalB.portalId = 2;
                }
                if (d0Pressed && !s_p0Pressed) {
                    s_portalsOn = !s_portalsOn;
                    s_p0Pressed = true;
                } else if (!d0Pressed) {
                    s_p0Pressed = false;
                }
            }
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.control.portals || !s_portalsOn) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(goose->id, "portal");

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    if (state->portalA.active && s_portalImages[0]) {
        float w = (float)CGImageGetWidth(s_portalImages[0]);
        float h = (float)CGImageGetHeight(s_portalImages[0]);
        CGRect rect = CGRectMake(state->portalA.x - w/2, state->portalA.y - h/2, w, h);
        CGContextDrawImage(cg, rect, s_portalImages[0]);
    }

    if (state->portalB.active && s_portalImages[1]) {
        float w = (float)CGImageGetWidth(s_portalImages[1]);
        float h = (float)CGImageGetHeight(s_portalImages[1]);
        CGRect rect = CGRectMake(state->portalB.x - w/2, state->portalB.y - h/2, w, h);
        CGContextDrawImage(cg, rect, s_portalImages[1]);
    }
}

static Behavior g_portalBehavior = {
    .id = "portal",
    .name = "Portal",
    .description = "Hold P + 1/2 to place portals, P+0 to toggle. Based on PortalGoos by Moonaliss1",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_portalBehavior);