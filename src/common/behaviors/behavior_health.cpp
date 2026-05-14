// ===========================
// behavior_health.cpp
// Health System - Goose health bar
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(ctx.goose->id, "health");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.systems.health) return;

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
        state->currentHealth -= 5.0f;
        state->lastDamageTime = time;
        if (state->currentHealth <= 0) {
            state->isDead = true;
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.systems.health) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(goose->id, "health");

#ifdef __APPLE__
    CGContextRef ctx_ = (CGContextRef)renderCtx;
    if (!ctx_) return;

    float barWidth = 40.0f;
    float barHeight = 4.0f;
    float x = goose->pos.x - barWidth / 2;
    float y = goose->pos.y - 50.0f;

    CGContextSetRGBFillColor(ctx_, 0.2f, 0.2f, 0.2f, 0.8f);
    CGContextFillRect(ctx_, CGRectMake(x, y, barWidth, barHeight));

    float healthPct = state->currentHealth / state->maxHealth;
    float r = 1.0f - healthPct;
    float g = healthPct;
    CGContextSetRGBFillColor(ctx_, r, g, 0.0f, 1.0f);
    CGContextFillRect(ctx_, CGRectMake(x, y, barWidth * healthPct, barHeight));
#endif
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

static Behavior g_healthBehavior = {
    .id = "health",
    .name = "Health",
    .description = "Health system for geese",
    .enabledPtr = &s_enabled,
    .configPtr = &g_config.behaviors.systems.health,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_healthBehavior);