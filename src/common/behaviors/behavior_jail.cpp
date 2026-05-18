// ===========================
// behavior_jail.cpp
// Jail Behavior - Trap the goose in a cage
// Based on GooseJail by WackyModer
// ===========================
#include "behavior.h"
#include "behaviors/states/jail_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "hotkey.h"
#include "ring_buffer.h"
#include "actor.h"
#include "actor_jail.h"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

static bool s_oWasKeyDown = false;
static bool s_pWasKeyDown = false;
static RingBuffer<Vector2, kMaxJails> s_jails;
static bool s_jailsActive = false;
static double s_lastInputTime = 0;

static bool IsKeyPressed(int keyCode) {
#ifdef __APPLE__
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return false;
    char keys[32];
    XQueryKeymap(dpy, keys);
    int keyIndex = keyCode / 8;
    int keyBit = keyCode % 8;
    bool pressed = (keys[keyIndex] & (1 << keyBit)) != 0;
    XCloseDisplay(dpy);
    return pressed;
#else
    (void)keyCode;
    return false;
#endif
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(ctx.goose->id, "jail");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(goose->id, "jail");

    if (!g_config.behaviors.control.jail) {
        state->isJailed = false;
        s_jailsActive = false;
        s_jails.clear();
        // Remove all jail actors
        auto& mgr = ActorManager::Instance();
        for (int i = mgr.totalCount() - 1; i >= 0; i--) {
            Actor* a = mgr.getByIndex(i);
            if (a && strcmp(a->type(), "jail") == 0) {
                mgr.remove(a);
                delete a;
            }
        }
        return;
    }

    if (time > s_lastInputTime) {
        s_lastInputTime = time;

        Vector2 cursorPos{-1, -1};
        if (g_cursorProvider) {
            CursorState cs = g_cursorProvider->Read();
            if (cs.hasPos()) {
                cursorPos = cs.position;
            }
        }

        bool oDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyO));
        if (oDown && !s_oWasKeyDown) {
            if (s_jailsActive) {
                s_jails.clear();
                s_jailsActive = false;
                // Remove all jail actors
                auto& mgr = ActorManager::Instance();
                for (int i = mgr.totalCount() - 1; i >= 0; i--) {
                    Actor* a = mgr.getByIndex(i);
                    if (a && strcmp(a->type(), "jail") == 0) {
                        mgr.remove(a);
                        delete a;
                    }
                }
            }
            s_jails.push(cursorPos);
            // Create jail actor
            JailActor* jail = new JailActor(cursorPos);
            ActorManager::Instance().add(jail);
        }
        s_oWasKeyDown = oDown;

        bool pDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyP));
        if (pDown && !s_pWasKeyDown && !s_jails.empty()) {
            s_jailsActive = !s_jailsActive;
            g_assets.Honk();
        }
        s_pWasKeyDown = pDown;
    }

    state->isJailed = s_jailsActive && !s_jails.empty();
    ctx.isJailed = state->isJailed;

    if (state->isJailed) {
        Vector2 nearest = s_jails[0];
        float minDist = Vector2::Distance(goose->pos, nearest);
        for (size_t i = 1; i < s_jails.size(); ++i) {
            float d = Vector2::Distance(goose->pos, s_jails[i]);
            if (d < minDist) {
                minDist = d;
                nearest = s_jails[i];
            }
        }
        
        goose->target = nearest;
        goose->pos = nearest;
        goose->vel = {0, 0};
        goose->state = GooseState::WANDER;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    (void)goose; (void)ctx; (void)renderCtx;
}

static Behavior g_jailBehavior = BEHAVIOR_DEF_CUSTOM(
    "jail", "Jail", "Press O to set jail position, P to trap goose. Based on GooseJail by WackyModer",
    g_config.behaviors.control.jail, init, tick, render,
    [](BehaviorContext&) {}, true, false
);

REGISTER_BEHAVIOR(g_jailBehavior);
