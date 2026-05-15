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

static constexpr float KICK_SPEED = 500.0f;
static constexpr float DECELERATION = 3.0f;
static constexpr float SPEED_THRESHOLD = 5.0f;

static Vector2 s_ballPos{300.0f, 300.0f};
static Vector2 s_ballVel{0.0f, 0.0f};
static float s_ballSpeed = 0.0f;
static float s_lastKickTime = 0.0f;
static float s_lastAnimateTime = 0.0f;
static float s_animationGap = 0.0f;
static int s_currentImage = 0;
static bool s_ballActive = true;

static float GetBallSize() {
    return g_config.behaviors.ball.size;
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

    float ballSize = GetBallSize();
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
        s_animationGap = std::max(0.05f, std::min(0.2f, 5.0f / s_ballSpeed));
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

        Vector2 norm = Vector2::Normalize(s_ballVel);
        s_ballVel = norm * s_ballSpeed;
        s_ballPos.x += s_ballVel.x * (float)dt;
        s_ballPos.y += s_ballVel.y * (float)dt;

        if (s_ballPos.x < 0) {
            s_ballPos.x = 0;
            s_ballVel.x *= -1.0f;
        }
        if (s_ballPos.x + ballSize > screenW) {
            s_ballPos.x = screenW - ballSize;
            s_ballVel.x *= -1.0f;
        }
        if (s_ballPos.y < 0) {
            s_ballPos.y = 0;
            s_ballVel.y *= -1.0f;
        }
        if (s_ballPos.y + ballSize > screenH) {
            s_ballPos.y = screenH - ballSize;
            s_ballVel.y *= -1.0f;
        }

        s_ballSpeed -= DECELERATION;
        if (s_ballSpeed <= SPEED_THRESHOLD) {
            s_currentImage = 0;
            s_ballSpeed = 0.0f;
            s_ballVel = Vector2{0, 0};
        }
    }

    float ballCenterX = s_ballPos.x + ballSize / 2.0f;
    float ballCenterY = s_ballPos.y + ballSize / 2.0f;
    float ballCenterDevX = ballCenterX * ctx.globalScale;
    float ballCenterDevY = ballCenterY * ctx.globalScale;
    float cursorX = cursorScreen.x / ctx.globalScale;
    float cursorY = cursorScreen.y / ctx.globalScale;

    float cursorDist = std::sqrt((cursorX - ballCenterX) * (cursorX - ballCenterX) +
                                 (cursorY - ballCenterY) * (cursorY - ballCenterY));

    if (s_ballSpeed > SPEED_THRESHOLD && goose) {
        goose->target = Vector2{ballCenterDevX, ballCenterDevY};
    }

    // Cursor kicks the ball when close
    if (cursorDist < ballSize / 2.0f && s_ballSpeed <= SPEED_THRESHOLD) {
        s_ballSpeed = KICK_SPEED;
        Vector2 dir = Vector2{ballCenterX - cursorX, ballCenterY - cursorY};
        s_ballVel = Vector2::Normalize(dir) * s_ballSpeed;
        s_lastKickTime = time;

        if (goose) {
            goose->target = Vector2{ballCenterDevX, ballCenterDevY};
            if (goose->state == GooseState::WANDER) {
                goose->state = GooseState::CHASE_CURSOR;
            }
        }
    }

    // Goose kicks the ball when walking near it (any state)
    if (goose && s_ballSpeed <= SPEED_THRESHOLD && time - s_lastKickTime > 0.5) {
        float gooseWorldX = goose->pos.x / ctx.globalScale;
        float gooseWorldY = goose->pos.y / ctx.globalScale;
        float gooseDist = std::sqrt((gooseWorldX - ballCenterX) * (gooseWorldX - ballCenterX) +
                                    (gooseWorldY - ballCenterY) * (gooseWorldY - ballCenterY));
        if (gooseDist < ballSize * 0.8f) {
            s_ballSpeed = KICK_SPEED * 0.7f;
            Vector2 dir = Vector2{ballCenterX - gooseWorldX, ballCenterY - gooseWorldY};
            s_ballVel = Vector2::Normalize(dir) * s_ballSpeed;
            s_lastKickTime = time;
            goose->target = Vector2{ballCenterDevX, ballCenterDevY};
            if (goose->state == GooseState::WANDER) {
                goose->state = GooseState::CHASE_CURSOR;
            }
            g_assets.Honk();
        }
    }

    // Goose keeps chasing moving ball
    if (goose && goose->state == GooseState::CHASE_CURSOR && s_ballSpeed > SPEED_THRESHOLD) {
        goose->target = Vector2{ballCenterDevX, ballCenterDevY};
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.ball) return;
    if (!s_ballActive) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    float ballSize = GetBallSize();

    CGImageRef img = s_ballImages[s_currentImage];
    float devSize = ballSize * ctx.globalScale;
    if (img) {
        float imgW = (float)CGImageGetWidth(img);
        float imgH = (float)CGImageGetHeight(img);
        float scale = devSize / std::max(imgW, imgH);
        float drawW = imgW * scale;
        float drawH = imgH * scale;
        CGRect rect = CGRectMake(s_ballPos.x * ctx.globalScale, s_ballPos.y * ctx.globalScale, drawW, drawH);
        CGContextDrawImage(cg, rect, img);
    } else {
        CGContextSetRGBFillColor(cg, 0.3f, 0.3f, 0.3f, 1.0f);
        CGContextFillEllipseInRect(cg, CGRectMake(s_ballPos.x * ctx.globalScale, s_ballPos.y * ctx.globalScale, devSize, devSize));
    }
}

static Behavior g_ballBehavior = {
    .id = "ball",
    .name = "Ball",
    .description = "Ball that goose chases and kicks. Based on BallMod by TheOrlando",
    .enabledPtr = &s_enabled,
    .configPtr = &g_config.behaviors.fun.ball,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_ballBehavior);
