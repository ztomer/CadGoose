// ===========================
// behavior_health.cpp
// Health System - Goose health bar
// ===========================
#include "behavior.h"
#include "behaviors/states/health_state.h"
#include "event_bus.h"
#include "goose.h"
#include "config.h"
#include "renderer_interface.h"
#include "render_colors.h"
#include "cg_renderer.h"
#include "behaviors/states/health_state.h"

static constexpr float kHealthBarWidth = 40.0f;
static constexpr float kHealthBarHeight = 4.0f;
static constexpr float kHealthBarYOffset = 50.0f;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(ctx.goose->id, "health");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(goose->id, "health");

    if (state->isDead) return;

    if (state->currentHealth < state->maxHealth) {
        state->regenAccumulator += g_config.behaviors.health.regenRate * dt;
        if (state->regenAccumulator >= 1.0f) {
            state->currentHealth = std::min(state->maxHealth, state->currentHealth + 1.0f);
            state->regenAccumulator = 0.0f;
        }
    }

    if (time - state->lastDamageTime > g_config.behaviors.health.damageCooldown && goose->currentSpeed > 200.0f) {
        constexpr float kDamagePerHit = 5.0f;
        state->currentHealth -= kDamagePerHit;
        state->lastDamageTime = time;
        EventBus::Instance().Publish(GooseDamagedEvent{goose->id, kDamagePerHit, time});
        if (state->currentHealth <= 0) {
            state->isDead = true;
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(goose->id, "health");

    if (!irenderer) return;
    IRenderer& renderer = *irenderer;

    float barWidth = kHealthBarWidth;
    float barHeight = kHealthBarHeight;
    float x = goose->pos.x - barWidth / 2;
    float y = goose->pos.y - kHealthBarYOffset;

    renderer.DrawRect({x, y, barWidth, barHeight}, MakeHealthBarBg(0.8f));

    float healthPct = state->currentHealth / state->maxHealth;
    renderer.DrawRect({x, y, barWidth * healthPct, barHeight},
                     RenderColor{1.0f - healthPct, healthPct, 0.0f, 1.0f});
}

void Health_Damage(Goose* goose, float amount, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(goose->id, "health");
    state->currentHealth = std::max(0.0f, state->currentHealth - amount);
    state->lastDamageTime = time;
}

void Health_Heal(Goose* goose, float amount) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(goose->id, "health");
    state->currentHealth = std::min(state->maxHealth, state->currentHealth + amount);
}

static Behavior g_healthBehavior = BEHAVIOR_DEF(
    "health", "Health", "Health system for geese",
    g_config.behaviors.systems.health, init, tick, render
);

REGISTER_BEHAVIOR(g_healthBehavior);
