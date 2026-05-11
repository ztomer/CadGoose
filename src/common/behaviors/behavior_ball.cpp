// ===========================
// behavior_ball.cpp
// Ball Behavior - Single ball that goose chases and kicks
// Based on BallMod by TheOrlando
// Reference: BallModv1.0.dll decompiled
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "goose_math.h"
#include "cursor_backend.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>

static bool s_enabled = true;
static CGImageRef s_ballImages[3] = {nullptr, nullptr, nullptr};

static constexpr float BALL_SIZE = 40.0f;
static constexpr float KICK_SPEED = 20.0f;
static constexpr float DECELERATION = 0.25f;
static constexpr float SPEED_THRESHOLD = 1.0f;

static Vector2 s_ballPos{300.0f, 300.0f};
static Vector2 s_ballVel{0.0f, 0.0f};
static float s_ballSpeed = 0.0f;
static float s_lastKickTime = 0.0f;
static float s_lastAnimateTime = 0.0f;
static float s_animationGap = 0.0f;
static int s_currentImage = 0;
static bool s_ballActive = true;

static Vector2 NormalizeVec(Vector2 v) {
    float len = Vector2::Length(v);
    if (len < 0.001f) return Vector2{0, 0};
    return v / len;
}

static void init(BehaviorContext& ctx) {
    if (!s_ballImages[0]) {
        s_ballImages[0] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball.png");
        s_ballImages[1] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball2.png");
        s_ballImages[2] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball3.png");
    }
    s_ballPos = Vector2{300.0f, 300.0f};
    s_ballVel = Vector2{0, 0};
    s_ballSpeed = 0.0f;
    s_lastKickTime = g_time;
    s_currentImage = 0;
    s_ballActive = true;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.ball) return;
    if (!s_ballActive) return;

    float screenW = (float)g_screenWidth / ctx.globalScale;
    float screenH = (float)g_screenHeight / ctx.globalScale;

    auto* backend = g_backendManager.GetActiveBackend();
    Vector2 cursorScreen;
    if (backend) {
        Vector2 rawPos = backend->GetCursorPos();
        cursorScreen = Vector2{rawPos.x, rawPos.y};
    } else {
        cursorScreen = Vector2{0, 0};
    }

    if (s_ballSpeed > SPEED_THRESHOLD) {
        s_animationGap = 0.25f / s_ballSpeed;
        if (time - s_lastAnimateTime > s_animationGap) {
            if (s_ballVel.x < 0) {
                s_currentImage++;
            } else {
                s_currentImage--;
            }
            if (s_currentImage > 2) s_currentImage = 0;
            if (s_currentImage < 0) s_currentImage = 2;
            s_lastAnimateTime = time;
        }

        Vector2 norm = NormalizeVec(s_ballVel);
        s_ballVel = norm * s_ballSpeed;
        s_ballPos.x += s_ballVel.x * (float)dt;
        s_ballPos.y += s_ballVel.y * (float)dt;

        if (s_ballPos.x < 0) {
            s_ballPos.x = 0;
            s_ballVel.x *= -1.0f;
        }
        if (s_ballPos.x + BALL_SIZE > screenW) {
            s_ballPos.x = screenW - BALL_SIZE;
            s_ballVel.x *= -1.0f;
        }
        if (s_ballPos.y < 0) {
            s_ballPos.y = 0;
            s_ballVel.y *= -1.0f;
        }
        if (s_ballPos.y + BALL_SIZE > screenH) {
            s_ballPos.y = screenH - BALL_SIZE;
            s_ballVel.y *= -1.0f;
        }

        s_ballSpeed -= DECELERATION;
        if (s_ballSpeed <= SPEED_THRESHOLD) {
            s_currentImage = 0;
            s_ballSpeed = 0.0f;
            s_ballVel = Vector2{0, 0};
        }
    }

    float ballCenterX = s_ballPos.x + BALL_SIZE / 2.0f;
    float ballCenterY = s_ballPos.y + BALL_SIZE / 2.0f;
    float cursorX = cursorScreen.x / ctx.globalScale;
    float cursorY = cursorScreen.y / ctx.globalScale;

    float dist = std::sqrt((cursorX - ballCenterX) * (cursorX - ballCenterX) +
                          (cursorY - ballCenterY) * (cursorY - ballCenterY));

    if (dist < BALL_SIZE / 2.0f && s_ballSpeed <= SPEED_THRESHOLD) {
        s_ballSpeed = KICK_SPEED;
        Vector2 dir = Vector2{ballCenterX - cursorX, ballCenterY - cursorY};
        s_ballVel = NormalizeVec(dir) * s_ballSpeed;
        s_lastKickTime = time;

        if (goose) {
            goose->target = Vector2{ballCenterX, ballCenterY};
        }
    }

    if (goose && goose->state == GooseState::CHASE_CURSOR) {
        float distToTarget = Vector2::Distance(goose->pos, goose->target);
        if (distToTarget < 40.0f && time - s_lastKickTime > 1.0) {
            s_ballSpeed = KICK_SPEED;
            Vector2 dir = Vector2{ballCenterX - goose->pos.x, ballCenterY - goose->pos.y};
            s_ballVel = NormalizeVec(dir) * s_ballSpeed;
            s_lastKickTime = time;
            g_assets.Honk();
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.ball) return;
    if (!s_ballActive) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGImageRef img = s_ballImages[s_currentImage];
    if (img) {
        float w = (float)CGImageGetWidth(img) * ctx.globalScale;
        float h = (float)CGImageGetHeight(img) * ctx.globalScale;
        CGRect rect = CGRectMake(s_ballPos.x * ctx.globalScale, s_ballPos.y * ctx.globalScale, w, h);
        CGContextDrawImage(cg, rect, img);
    } else {
        CGContextSetRGBFillColor(cg, 0.3f, 0.3f, 0.3f, 1.0f);
        float size = BALL_SIZE * ctx.globalScale;
        CGContextFillEllipseInRect(cg, CGRectMake(s_ballPos.x * ctx.globalScale, s_ballPos.y * ctx.globalScale, size, size));
    }
}

static Behavior g_ballBehavior = {
    .id = "ball",
    .name = "Ball",
    .description = "Ball that goose chases and kicks. Based on BallMod by TheOrlando",
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