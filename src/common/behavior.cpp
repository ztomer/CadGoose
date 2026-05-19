// ===========================
// behavior.cpp
// Multi-Entity Behavior System Implementation
// ===========================

#include "behavior.h"
#include "behaviors/states/jail_state.h"
#include "behaviors/states/ball_state.h"
#include "behaviors/states/breadcrumb_state.h"
#include "behaviors/states/health_state.h"
#include "behaviors/states/anger_state.h"
#include "behaviors/states/portal_state.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "behaviors/states/jail_state.h"
#include "behaviors/states/ball_state.h"
#include "behaviors/states/breadcrumb_state.h"
#include "behaviors/states/health_state.h"
#include "behaviors/states/anger_state.h"
#include "behaviors/states/portal_state.h"
#include <cmath>
#include <algorithm>

// ===========================
// Physics Constants
// ===========================
constexpr float GRAVITY = 400.0f;
constexpr float BOUNCE_FACTOR = 0.7f;
constexpr float FRICTION = 0.98f;
constexpr float AIR_RESISTANCE = 0.995f;
static constexpr float kReferenceFrameRate = 60.0f;
static constexpr float kBallBounceThreshold = 20.0f;
static constexpr float kDragResistanceProbability = 0.05f;

// ===========================
// DT-Scaled Helper Macros
// ===========================
#define DT_SCALED_ROTATION(current, degPerSec, dt) (current + (degPerSec) * (dt))
#define DT_SCALED_VELOCITY(vel, friction, dt) (vel * std::pow(friction, dt * kReferenceFrameRate))
#define DT_SCALED_GRAVITY(gravity, dt) (gravity * dt)

// ===========================
// Behavior Registry Implementation
// ===========================

void EnsureBehaviorsRestored() {
}

void EnsureBehaviorsRestoredForce() {
}

void BehaviorRegistry::Register(Behavior& behavior) {
    behaviors.push_back(&behavior);
}

Behavior* BehaviorRegistry::Get(const char* id) {
    for (auto* behavior : behaviors) {
        if (std::strcmp(behavior->id, id) == 0) {
            return behavior;
        }
    }
    return nullptr;
}

void BehaviorRegistry::InitAll(Goose* goose) {
    if (!goose) return;

    BehaviorContext ctx{};
    ctx.goose = goose;
    ctx.time = g_time;
    ctx.globalScale = g_config.general.globalScale;

    for (auto* behavior : behaviors) {
        if (behavior->enabledPtr && *behavior->enabledPtr) {
            if (behavior->init) {
                try {
                    behavior->init(ctx);
                } catch (const std::exception& e) {
                    fprintf(stderr, "[BEHAVIOR] Init failed for %s: %s\n",
                            behavior->id, e.what());
                }
            }
        }
    }
}

void BehaviorRegistry::TickAll(Goose* goose, double dt, double time) {
    if (!goose) return;

    BehaviorContext ctx{};
    ctx.goose = goose;
    ctx.time = time;
    ctx.globalScale = g_config.general.globalScale;

    auto* jailState = BehaviorStateManager::Instance().Get<JailState>(goose->id, "jail");
    if (jailState && jailState->isJailed) {
        ctx.isJailed = true;
    }

    for (auto* behavior : behaviors) {
        bool isEnabled = behavior->enabledPtr && *behavior->enabledPtr;
        
        auto* state = BehaviorStateManager::Instance().Get<BehaviorState>(goose->id, behavior->id);
        bool wasEnabled = state ? state->wasEnabled : false;

        if (isEnabled || wasEnabled) {
            try {
                behavior->tick(goose, ctx, dt, time);
            } catch (const std::exception& e) {
                fprintf(stderr, "[BEHAVIOR] Tick failed for %s: %s\n",
                        behavior->id, e.what());
            }

            if (!state) {
                state = BehaviorStateManager::Instance().Get<BehaviorState>(goose->id, behavior->id);
            }
            if (state) {
                state->wasEnabled = isEnabled;
            }
        }
    }
}

void BehaviorRegistry::RenderAll(Goose* goose, IRenderer* renderer) {
    RenderPass(goose, renderer, false);
}

void BehaviorRegistry::RenderPass(Goose* goose, IRenderer* renderer, bool groundPass) {
    if (!goose) return;

    BehaviorContext behaviorCtx{};
    behaviorCtx.goose = goose;
    behaviorCtx.time = g_time;
    behaviorCtx.globalScale = g_config.general.globalScale;
    behaviorCtx.renderer = renderer;

    for (auto* behavior : behaviors) {
        if (behavior->enabledPtr && *behavior->enabledPtr && behavior->renderOnGround == groundPass) {
            try {
                behavior->render(goose, behaviorCtx, renderer);
            } catch (const std::exception& e) {
                fprintf(stderr, "[BEHAVIOR] Render failed for %s: %s\n",
                        behavior->id, e.what());
            }
        }
    }
}

void BehaviorRegistry::CleanupAll(Goose* goose) {
    if (!goose) return;

    BehaviorContext ctx{};
    ctx.goose = goose;
    ctx.time = g_time;

    for (auto* behavior : behaviors) {
        if (behavior->cleanup) {
            try {
                behavior->cleanup(ctx);
            } catch (...) {}
        }
    }

    BehaviorStateManager::Instance().RemoveForGoose(goose->id);
}

void BehaviorRegistry::Clear() {
    behaviors.clear();
}

// ===========================
// Ball Physics
// ===========================
void UpdateBallPhysics(BallState::Ball& ball, float screenWidth, float screenHeight,
                       float globalScale, double dt) {
    if (!ball.active) return;

    ball.vel.y += DT_SCALED_GRAVITY(GRAVITY, dt);

    ball.vel.x = DT_SCALED_VELOCITY(ball.vel.x, AIR_RESISTANCE, dt);
    ball.vel.y = DT_SCALED_VELOCITY(ball.vel.y, AIR_RESISTANCE, dt);

    ball.pos.x += ball.vel.x * dt;
    ball.pos.y += ball.vel.y * dt;

    float screenBottom = screenHeight / globalScale;
    float screenRight = screenWidth / globalScale;
    float radius = ball.radius;

    if (ball.pos.y > screenBottom - radius) {
        ball.pos.y = screenBottom - radius;
        ball.vel.y = -ball.vel.y * BOUNCE_FACTOR;
        ball.vel.x *= FRICTION;
        if (std::abs(ball.vel.y) < kBallBounceThreshold) {
            ball.vel.y = 0;
        }
    }

    if (ball.pos.x < radius) {
        ball.pos.x = radius;
        ball.vel.x = -ball.vel.x * BOUNCE_FACTOR;
    } else if (ball.pos.x > screenRight - radius) {
        ball.pos.x = screenRight - radius;
        ball.vel.x = -ball.vel.x * BOUNCE_FACTOR;
    }
}

void KickBallFromCursor(BallState::Ball& ball, float cursorX, float cursorY, float kickForce) {
    float dx = ball.pos.x - cursorX;
    float dy = ball.pos.y - cursorY;
    float dist = std::hypot(dx, dy);

    if (dist > 0.001f && dist < kickForce * 0.5f) {
        Vector2 dir = Vector2::Normalize({dx, dy});
        ball.vel.x = dir.x * kickForce;
        ball.vel.y = dir.y * kickForce;
    }
}

void KickBallFromGoose(BallState::Ball& ball, float gooseX, float gooseY, float kickForce) {
    float dx = ball.pos.x - gooseX;
    float dy = ball.pos.y - gooseY;
    float dist = std::hypot(dx, dy);

    if (dist > 0.001f) {
        Vector2 dir = Vector2::Normalize({dx, dy});
        ball.vel.x = dir.x * kickForce;
        ball.vel.y = dir.y * kickForce;
    }
}

// ===========================
// Breadcrumb Physics
// ===========================
void UpdateCrumbPhysics(BreadCrumbState::Crumb& crumb, float globalScale, double dt) {
    if (!crumb.active) return;

    crumb.vel.y += DT_SCALED_GRAVITY(GRAVITY * 0.5f, dt);

    crumb.pos.x += crumb.vel.x * dt;
    crumb.pos.y += crumb.vel.y * dt;

    crumb.lifetime -= dt;
    if (crumb.lifetime <= 0) {
        crumb.active = false;
    }
}

// ===========================
// Acid Rotation (DT-normalized)
// ===========================
float CalculateAcidRotation(float currentDirection, float degreesPerSecond, double dt) {
    return NormalizeAngle360(DT_SCALED_ROTATION(currentDirection, degreesPerSecond, dt));
}

// ===========================
// Rainbow Hue Cycling (DT-normalized)
// ===========================
float CalculateRainbowHue(float currentHue, float degreesPerSecond, double dt) {
    float newHue = currentHue + degreesPerSecond * dt;
    if (newHue >= 360.0f) {
        newHue -= 360.0f;
    }
    return newHue;
}

// ===========================
// Health System
// ===========================
void ApplyDamage(HealthState* state, float damage, double time) {
    if (!state) return;
    state->currentHealth = Clamp(state->currentHealth - damage, 0.0f, state->maxHealth);
    state->lastDamageTime = time;
}

void ApplyRegen(HealthState* state, float regenPerSecond, double dt) {
    if (!state) return;
    if (state->currentHealth < state->maxHealth) {
        state->regenAccumulator += regenPerSecond * dt;
        if (state->regenAccumulator >= 1.0f) {
            state->currentHealth = Clamp(state->currentHealth + 1.0f, 0.0f, state->maxHealth);
            state->regenAccumulator -= 1.0f;
        }
    }
}

// ===========================
// Anger System
// ===========================
void IncreaseAnger(AngerState* state, float amount, double time) {
    if (!state) return;
    state->angerLevel = Clamp(state->angerLevel + amount, 0.0f, 100.0f);
    state->lastAngerIncrease = time;
}

void DecreaseAnger(AngerState* state, float amount, double dt) {
    if (!state) return;
    state->angerLevel = Clamp(state->angerLevel - amount * dt, 0.0f, 100.0f);
}

void ResetPunchCooldown(AngerState* state, double time) {
    if (!state) return;
    state->isPunching = false;
    state->lastPunchTime = time;
}

// ===========================
// Portal Teleport Logic
// ===========================
bool CheckPortalCollision(float x, float y, const PortalState::Portal& portal, float radius) {
    if (!portal.active) return false;
    float dist = std::hypot(x - portal.x, y - portal.y);
    return dist < radius;
}

void TeleportThroughPortal(float& x, float& y, PortalState::Portal& fromPortal,
                           PortalState::Portal& toPortal, float radius) {
    if (!fromPortal.active || !toPortal.active) return;
    if (CheckPortalCollision(x, y, fromPortal, radius)) {
        x = toPortal.x + (x - fromPortal.x) * 0.1f;
        y = toPortal.y + (y - fromPortal.y) * 0.1f;
    }
}

// ===========================
// Drag Resistance
// ===========================
bool CheckDragResistance(float dragSpeed, float resistanceThreshold, float randomValue) {
    return dragSpeed > resistanceThreshold && randomValue < kDragResistanceProbability;
}

float CalculateDragResistance(float dragSpeed, float maxSpeed) {
    if (dragSpeed > maxSpeed) {
        return 1.0f - (maxSpeed / dragSpeed);
    }
    return 0.0f;
}