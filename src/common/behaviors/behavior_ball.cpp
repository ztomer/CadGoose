// ===========================
// behavior_ball.cpp
// Ball Behavior - Push balls around (soccer, beach, generic)
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>

static bool s_enabled = true;

static void drawSoccerPattern(CGContextRef ctx, float x, float y, float radius) {
    CGContextSetRGBFillColor(ctx, 0.15f, 0.15f, 0.15f, 1.0f);

    float hexRadius = radius * 0.3f;
    float hexes[5][2] = {
        {0, 0},
        {0, -radius * 0.4f},
        {radius * 0.35f, -radius * 0.2f},
        {radius * 0.35f, radius * 0.2f},
        {0, radius * 0.4f}
    };

    for (int i = 0; i < 5; i++) {
        CGContextBeginPath(ctx);
        int sides = 6;
        for (int j = 0; j <= sides; j++) {
            float angle = (float)j * 2.0f * M_PI / (float)sides - M_PI_2;
            float px = x + hexes[i][0] + hexRadius * cosf(angle);
            float py = y + hexes[i][1] + hexRadius * sinf(angle);
            if (j == 0) CGContextMoveToPoint(ctx, px, py);
            else CGContextAddLineToPoint(ctx, px, py);
        }
        CGContextClosePath(ctx);
        CGContextFillPath(ctx);
    }
}

static void drawBeachStripes(CGContextRef ctx, float x, float y, float radius) {
    int stripeCount = 6;
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, x, y);

    for (int i = 0; i < stripeCount; i++) {
        float angle = (float)i * 2.0f * M_PI / (float)stripeCount;
        float stripeAngle = M_PI / (float)stripeCount;

        CGContextSaveGState(ctx);
        CGContextRotateCTM(ctx, angle);

        CGContextBeginPath(ctx);
        CGContextMoveToPoint(ctx, radius * 0.3f, 0);
        CGContextAddLineToPoint(ctx, radius, 0);
        CGContextRotateCTM(ctx, stripeAngle);
        CGContextAddLineToPoint(ctx, radius * 0.3f, 0);
        CGContextClosePath(ctx);

        if (i % 2 == 0) {
            CGContextSetRGBFillColor(ctx, 0.9f, 0.3f, 0.2f, 1.0f);
        } else {
            CGContextSetRGBFillColor(ctx, 1.0f, 1.0f, 0.2f, 1.0f);
        }
        CGContextFillPath(ctx);

        CGContextRestoreGState(ctx);
    }

    CGContextRestoreGState(ctx);
}

static void drawBall(CGContextRef ctx, const BallState::Ball& ball, float globalScale) {
    float radius = ball.radius * globalScale;
    float x = ball.pos.x * globalScale;
    float y = ball.pos.y * globalScale;

    CGContextSetRGBFillColor(ctx, ball.r, ball.g, ball.b, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(x - radius, y - radius, radius * 2, radius * 2));

    if (ball.type == BallState::BallType::Soccer) {
        drawSoccerPattern(ctx, x, y, radius);
    } else if (ball.type == BallState::BallType::Beach) {
        drawBeachStripes(ctx, x, y, radius);
    }

    CGContextSetRGBStrokeColor(ctx, ball.strokeR, ball.strokeG, ball.strokeB, 1.0f);
    CGContextSetLineWidth(ctx, ball.strokeWidth * globalScale);
    CGContextStrokeEllipseInRect(ctx, CGRectMake(x - radius, y - radius, radius * 2, radius * 2));
}

static void spawnBall(BallState::Ball& ball, Goose* goose, const BehaviorConfig::BallConfig& cfg) {
    int typeRoll = rand() % 100;
    if (cfg.soccerEnabled && cfg.beachEnabled) {
        if (typeRoll < 33) {
            ball = BallState::CreateSoccerBall(cfg.soccerSize);
        } else if (typeRoll < 66) {
            ball = BallState::CreateBeachBall(cfg.beachSize);
        } else {
            ball = BallState::CreateGenericBall(cfg.size);
        }
    } else if (cfg.soccerEnabled) {
        ball = (typeRoll < 50) ? BallState::CreateSoccerBall(cfg.soccerSize) : BallState::CreateGenericBall(cfg.size);
    } else if (cfg.beachEnabled) {
        ball = (typeRoll < 50) ? BallState::CreateBeachBall(cfg.beachSize) : BallState::CreateGenericBall(cfg.size);
    } else {
        ball = BallState::CreateGenericBall(cfg.size);
    }

    ball.pos.x = goose->pos.x + (rand() % (int)(cfg.spawnRange * 2) - (int)cfg.spawnRange);
    ball.pos.y = goose->pos.y + (rand() % (int)(cfg.spawnRange * 2) - (int)cfg.spawnRange);
    ball.vel.x = (rand() % 100 - 50) * 0.1f;
    ball.vel.y = (rand() % 100 - 50) * 0.1f;
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BallState>(ctx.goose->id, "ball");
    state->Reset();
    state->balls.resize(g_config.behaviors.ball.count);
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.ball) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<BallState>(goose->id, "ball");
    if (state->balls.empty()) return;

    float screenW = (float)g_screenWidth / ctx.globalScale;
    float screenH = (float)g_screenHeight / ctx.globalScale;
    const auto& cfg = g_config.behaviors.ball;

    for (auto& ball : state->balls) {
        if (!ball.active) {
            if (rand() % (int)cfg.spawnChance == 0) {
                ball.active = true;
                spawnBall(ball, goose, cfg);
            }
            continue;
        }

        float dist = Vector2::Distance(ball.pos, goose->pos);
        float interactionDist = ball.radius + cfg.interactionRadius;

        if (dist < interactionDist && dist > 0.1f) {
            Vector2 dir = Vector2::Normalize(ball.pos - goose->pos);

            float speed = Vector2::Length(ball.vel);
            float ballSpeed = (ball.type == BallState::BallType::Soccer) ? cfg.soccerSpeed
                            : (ball.type == BallState::BallType::Beach) ? cfg.beachSpeed
                            : cfg.speed;

            if (speed < ballSpeed * 0.5f) {
                ball.vel.x += dir.x * ballSpeed * dt * 0.5f;
                ball.vel.y += dir.y * ballSpeed * dt * 0.5f;
            }
        }

        float friction = Vector2::Length(ball.vel) > 1.0f ? cfg.friction : cfg.friction * 0.95f;
        ball.vel.x *= std::pow(friction, dt * 60.0f);
        ball.vel.y *= std::pow(friction, dt * 60.0f);
        ball.pos.x += ball.vel.x * dt;
        ball.pos.y += ball.vel.y * dt;

        float bounce = (ball.type == BallState::BallType::Soccer) ? cfg.soccerBounce
                     : (ball.type == BallState::BallType::Beach) ? cfg.beachBounce
                     : cfg.bounceFactor;

        if (ball.pos.x < ball.radius) { ball.pos.x = ball.radius; ball.vel.x *= -bounce; }
        if (ball.pos.x > screenW - ball.radius) { ball.pos.x = screenW - ball.radius; ball.vel.x *= -bounce; }
        if (ball.pos.y < ball.radius) { ball.pos.y = ball.radius; ball.vel.y *= -bounce; }
        if (ball.pos.y > screenH - ball.radius) { ball.pos.y = screenH - ball.radius; ball.vel.y *= -bounce; }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.ball) return;

    auto* state = BehaviorStateManager::Instance().Get<BallState>(goose->id, "ball");
    if (!state) return;

    CGContextRef cgc = (CGContextRef)renderCtx;
    if (!cgc) return;

    for (auto& ball : state->balls) {
        if (!ball.active) continue;
        drawBall(cgc, ball, ctx.globalScale);
    }
}

static Behavior g_ballBehavior = {
    .id = "ball",
    .name = "Ball",
    .description = "Balls (soccer, beach, generic) that the goose can push around",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_ballBehavior);