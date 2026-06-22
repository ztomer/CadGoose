// ===========================
// behavior_jail.cpp
// Jail Behavior - Trap the goose in a cage
// Based on GooseJail by WackyModer
// ===========================
#include "behavior.h"
#include "behaviors/states/jail_state.h"
#include "event_bus.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "hotkey.h"
#include "ring_buffer.h"
#include "actor.h"
#include "actor_jail.h"
#include "platform_input.h"

static bool s_oWasKeyDown = false;
static bool s_pWasKeyDown = false;
static RingBuffer<Vector2, kMaxJails> s_jails;
static bool s_jailsActive = false;
static double s_lastInputTime = 0;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(ctx.goose->id, "jail");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    (void)dt;
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(goose->id, "jail");

    if (!g_config.behaviors.control.jail) {
        state->isJailed = false;
        s_jailsActive = false;
        s_jails.clear();
        // Remove all jail actors
        auto& mgr = ActorManager::Instance();
        for (int i = mgr.totalCount() - 1; i >= 0; i--) {
            Actor* a = mgr.getByIndex(i);
            if (a && a->actorType() == ActorType::Jail) {
                a->setActive(false);
            }
        }
        return;
    }

    s_lastInputTime = time;

    Vector2 cursorPos{-1, -1};
    if (g_cursorProvider) {
        CursorState cs = g_cursorProvider->Read();
        if (cs.hasPos()) {
            cursorPos = cs.position;
        }
    }

    bool oDown = Platform_IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyO));
    if (oDown && !s_oWasKeyDown) {
        if (s_jailsActive) {
            s_jails.clear();
            s_jailsActive = false;
            // Remove all jail actors
            auto& mgr = ActorManager::Instance();
            for (int i = mgr.totalCount() - 1; i >= 0; i--) {
                Actor* a = mgr.getByIndex(i);
                if (a && a->actorType() == ActorType::Jail) {
                    a->setActive(false);
                }
            }
        }
        s_jails.push(cursorPos);
        // Create jail actor
        JailActor* jail = new JailActor(cursorPos);
        ActorManager::Instance().add(jail);
    }
    s_oWasKeyDown = oDown;

    bool pDown = Platform_IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyP));
    if (pDown && !s_pWasKeyDown && !s_jails.empty()) {
        s_jailsActive = !s_jailsActive;
        goose->onHonk();
    }
    s_pWasKeyDown = pDown;

    bool wasJailed = state->isJailed;
    state->isJailed = s_jailsActive && !s_jails.empty();
    ctx.isJailed = state->isJailed;

    if (!wasJailed && state->isJailed) {
        EventBus::Instance().Publish(GooseJailedEvent{goose->id, goose->pos.x, goose->pos.y});
    } else if (wasJailed && !state->isJailed) {
        EventBus::Instance().Publish(GooseFreedEvent{goose->id});
    }

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

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    (void)goose; (void)ctx; (void)irenderer;
}

// Test helper: reset all static state between test cases
void Jail_ResetForTest() {
    s_oWasKeyDown = false;
    s_pWasKeyDown = false;
    s_jails.clear();
    s_jailsActive = false;
    s_lastInputTime = 0;
}

static Behavior g_jailBehavior = BEHAVIOR_DEF_CUSTOM(
    "jail", "Jail", "Press O to set jail position, P to trap goose. Based on GooseJail by WackyModer",
    g_config.behaviors.control.jail, init, tick, render,
    [](BehaviorContext&) {}, true, false
);

REGISTER_BEHAVIOR(g_jailBehavior);
