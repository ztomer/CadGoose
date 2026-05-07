// ===========================
// behavior_portal.cpp
// Portal Behavior - User-controlled portal placement
// Based on PortalGoos by Moonaliss1
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>

static bool s_enabled = true;
static bool s_portalsOn = true;
static bool s_p0Pressed = false;

enum PortalKey { P_KEY = 0x50, D1_KEY = 0x31, D2_KEY = 0x32, D0_KEY = 0x30 };

static bool IsKeyPressed(int keyCode) {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return false;

    CGEventRef event = CGEventCreate(source);
    if (!event) {
        CFRelease(source);
        return false;
    }

    CGKeyCode key = CGKeyCode(keyCode);
    int64_t eventKey = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    bool pressed = (eventKey == key);

    CFRelease(event);
    CFRelease(source);
    return pressed;
}

static void init(BehaviorContext& ctx) {
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
            g_assets.Honk();
            state->justTeleported = true;
        } else if (inP2 && state->portalA.active) {
            goose->pos.x = state->portalA.x;
            goose->pos.y = state->portalA.y;
            g_assets.Honk();
            state->justTeleported = true;
        }
    }

    short pState = IsKeyPressed(P_KEY) ? -1 : 0;
    if (pState != 0) {
        CGEventRef event = CGEventCreate(nullptr);
        if (event) {
            CGPoint mousePos = CGEventGetLocation(event);
            CFRelease(event);

            bool d1Pressed = IsKeyPressed(D1_KEY);
            bool d2Pressed = IsKeyPressed(D2_KEY);
            bool d0Pressed = IsKeyPressed(D0_KEY);

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

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.control.portals || !s_portalsOn) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(goose->id, "portal");

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    if (state->portalA.active) {
        float p1w = g_config.portal.p1Width;
        float p1h = g_config.portal.p1Height;
        CGRect rect = CGRectMake(state->portalA.x - p1w/2, state->portalA.y - p1h/2, p1w, p1h);

        CGContextSetRGBFillColor(cg, 0.0f, 0.8f, 1.0f, 0.5f);
        CGContextFillEllipseInRect(cg, rect);
        CGContextSetRGBStrokeColor(cg, 0.0f, 0.5f, 1.0f, 1.0f);
        CGContextSetLineWidth(cg, 3.0f);
        CGContextStrokeEllipseInRect(cg, rect);
    }

    if (state->portalB.active) {
        float p2w = g_config.portal.p2Width;
        float p2h = g_config.portal.p2Height;
        CGRect rect = CGRectMake(state->portalB.x - p2w/2, state->portalB.y - p2h/2, p2w, p2h);

        CGContextSetRGBFillColor(cg, 1.0f, 0.5f, 0.0f, 0.5f);
        CGContextFillEllipseInRect(cg, rect);
        CGContextSetRGBStrokeColor(cg, 1.0f, 0.3f, 0.0f, 1.0f);
        CGContextSetLineWidth(cg, 3.0f);
        CGContextStrokeEllipseInRect(cg, rect);
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