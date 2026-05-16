#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "goose_math.h"
#include "cursor_backend.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>

static CGImageRef s_ballImages[3] = {nullptr, nullptr, nullptr};

static constexpr float KICK_SPEED = 500.0f;
static constexpr float DECELERATION = 3.0f;
static constexpr float SPEED_THRESHOLD = 5.0f;
static constexpr float KICK_COOLDOWN = 0.5f;
static constexpr float BALL_INIT_X = 300.0f;
static constexpr float BALL_INIT_Y = 300.0f;
static constexpr float GOOSE_KICK_DIST_FACTOR = 0.8f;
static constexpr float CURSOR_KICK_DIST_FACTOR = 0.5f;
static constexpr float GOOSE_CHASE_SPEED_FACTOR = 0.7f;
static constexpr float ANIM_SPEED_DIVISOR = 5.0f;
static constexpr float ANIM_GAP_MIN = 0.05f;
static constexpr float ANIM_GAP_MAX = 0.2f;
static constexpr int ANIM_FRAME_COUNT = 3;

static float GetBallSize() {
    return g_config.behaviors.ball.size;
}

static BallState::Ball* GetOrCreateBall(int gooseId) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BallState>(gooseId, "ball");
    if (state->balls.empty()) {
        BallState::Ball ball;
        ball.pos = {BALL_INIT_X, BALL_INIT_Y};
        ball.vel = {0, 0};
        ball.radius = GetBallSize();
        ball.active = true;
        ball.speed = 0.0f;
        ball.lastKickTime = 0.0f;
        ball.lastAnimateTime = 0.0f;
        ball.animationGap = 0.0f;
        ball.currentFrame = 0;
        state->balls.push_back(ball);
    }
    return &state->balls.front();
}

static void init(BehaviorContext& ctx) {
    if (!s_ballImages[0]) {
        s_ballImages[0] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball.png");
        s_ballImages[1] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball2.png");
        s_ballImages[2] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball3.png");
    }
    auto* ball = GetOrCreateBall(ctx.goose->id);
    ball->pos = {BALL_INIT_X, BALL_INIT_Y};
    ball->vel = {0, 0};
    ball->speed = 0.0f;
    ball->lastKickTime = ctx.time;
    ball->currentFrame = 0;
    ball->active = true;
    ball->radius = GetBallSize();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* ball = GetOrCreateBall(goose->id);
    if (!ball->active) return;

    float ballSize = GetBallSize();
    ball->radius = ballSize;

    CursorState cursor = {};
    if (g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }
    Vector2 cursorPos{cursor.position.x, cursor.position.y};

    if (ball->speed > SPEED_THRESHOLD) {
        ball->animationGap = std::max(ANIM_GAP_MIN, std::min(ANIM_GAP_MAX, ANIM_SPEED_DIVISOR / ball->speed));
        if (time - ball->lastAnimateTime > ball->animationGap) {
            if (ball->vel.x < 0) {
                ball->currentFrame++;
            } else {
                ball->currentFrame--;
            }
            if (ball->currentFrame >= ANIM_FRAME_COUNT) ball->currentFrame = 0;
            if (ball->currentFrame < 0) ball->currentFrame = ANIM_FRAME_COUNT - 1;
            ball->lastAnimateTime = time;
        }

        Vector2 norm = Vector2::Normalize(ball->vel);
        ball->vel = norm * ball->speed;
        ball->pos.x += ball->vel.x * (float)dt;
        ball->pos.y += ball->vel.y * (float)dt;

        float screenW = (float)g_screenWidth;
        float screenH = (float)g_screenHeight;

        if (ball->pos.x < 0) {
            ball->pos.x = 0;
            ball->vel.x *= -1.0f;
        }
        if (ball->pos.x + ballSize > screenW) {
            ball->pos.x = screenW - ballSize;
            ball->vel.x *= -1.0f;
        }
        if (ball->pos.y < 0) {
            ball->pos.y = 0;
            ball->vel.y *= -1.0f;
        }
        if (ball->pos.y + ballSize > screenH) {
            ball->pos.y = screenH - ballSize;
            ball->vel.y *= -1.0f;
        }

        ball->speed -= DECELERATION;
        if (ball->speed <= SPEED_THRESHOLD) {
            ball->currentFrame = 0;
            ball->speed = 0.0f;
            ball->vel = Vector2{0, 0};
        }
    }

    float ballCenterX = ball->pos.x + ballSize / 2.0f;
    float ballCenterY = ball->pos.y + ballSize / 2.0f;

    float cursorDist = std::hypot(cursorPos.x - ballCenterX, cursorPos.y - ballCenterY);

    if (ball->speed > SPEED_THRESHOLD && goose) {
        goose->target = Vector2{ballCenterX, ballCenterY};
    }

    // Cursor kicks the ball when close
    if (cursorDist < ballSize * CURSOR_KICK_DIST_FACTOR && ball->speed <= SPEED_THRESHOLD) {
        ball->speed = KICK_SPEED;
        Vector2 dir = Vector2{ballCenterX - cursorPos.x, ballCenterY - cursorPos.y};
        ball->vel = Vector2::Normalize(dir) * ball->speed;
        ball->lastKickTime = time;

        if (goose) {
            goose->target = Vector2{ballCenterX, ballCenterY};
            if (goose->state == GooseState::WANDER) {
                goose->state = GooseState::CHASE_CURSOR;
            }
        }
    }

    // Goose kicks the ball when walking near it (any state)
    if (goose && ball->speed <= SPEED_THRESHOLD && time - ball->lastKickTime > KICK_COOLDOWN) {
        float gooseDist = std::hypot(goose->pos.x - ballCenterX, goose->pos.y - ballCenterY);
        if (gooseDist < ballSize * GOOSE_KICK_DIST_FACTOR) {
            ball->speed = KICK_SPEED * GOOSE_CHASE_SPEED_FACTOR;
            Vector2 dir = Vector2{ballCenterX - goose->pos.x, ballCenterY - goose->pos.y};
            ball->vel = Vector2::Normalize(dir) * ball->speed;
            ball->lastKickTime = time;
            goose->target = Vector2{ballCenterX, ballCenterY};
            if (goose->state == GooseState::WANDER) {
                goose->state = GooseState::CHASE_CURSOR;
            }
            g_assets.Honk();
        }
    }

    // Goose keeps chasing moving ball
    if (goose && goose->state == GooseState::CHASE_CURSOR && ball->speed > SPEED_THRESHOLD) {
        goose->target = Vector2{ballCenterX, ballCenterY};
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* ball = GetOrCreateBall(goose->id);
    if (!ball->active) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    float ballSize = ball->radius;
    CGImageRef img = s_ballImages[ball->currentFrame % ANIM_FRAME_COUNT];
    float drawSize = ballSize;
    float scale = ctx.globalScale;
    Vector2 drawPos{goose->pos.x + (ball->pos.x - goose->pos.x) / scale,
                    goose->pos.y + (ball->pos.y - goose->pos.y) / scale};
    if (img) {
        float imgW = (float)CGImageGetWidth(img);
        float imgH = (float)CGImageGetHeight(img);
        float imgScale = drawSize / std::max(imgW, imgH);
        float drawW = imgW * imgScale;
        float drawH = imgH * imgScale;
        CGRect rect = CGRectMake(drawPos.x, drawPos.y, drawW, drawH);
        CGContextDrawImage(cg, rect, img);
    } else {
        CGContextSetRGBFillColor(cg, 0.3f, 0.3f, 0.3f, 1.0f);
        CGContextFillEllipseInRect(cg, CGRectMake(drawPos.x, drawPos.y, drawSize, drawSize));
    }
#endif
}

static Behavior g_ballBehavior = BEHAVIOR_DEF_STARTER(
    "ball", "Ball", "Ball that goose chases and kicks. Based on BallMod by TheOrlando",
    g_config.behaviors.fun.ball, init, tick, render
);

REGISTER_BEHAVIOR(g_ballBehavior);
