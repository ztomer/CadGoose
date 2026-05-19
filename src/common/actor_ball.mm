// actor_ball.mm
// Ball actor implementation — physics, window, animation.

#include "actor_ball.h"
#include "actor.h"
#include "config.h"
#include "world.h"
#include "event_bus.h"
#include "assets.h"
#include "goose_math.h"
#include "cursor_backend.h"
#include <cmath>

#ifdef __APPLE__
#include "behavior_element_window.h"
#include <CoreGraphics/CoreGraphics.h>
#endif

static constexpr float BALL_INIT_X = 300.0f;
static constexpr float BALL_INIT_Y = 300.0f;
static constexpr int kBallScreenMargin = 50;

static float GetBallSize() {
    return g_config.behaviors.ball.size;
}

BallActor::BallActor()
    : Actor(), velocity{0, 0}, speed(0), lastKickTime(0),
      lastAnimateTime(0), animationGap(0), currentFrame(0), m_wasKicked(false)
#ifdef __APPLE__
    , m_window(nil), m_windowKey(nil)
#endif
{
    m_position = {BALL_INIT_X, BALL_INIT_Y};
    m_radius = GetBallSize();
    m_active = true;

#ifdef __APPLE__
    m_images[0] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball.png");
    m_images[1] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball2.png");
    m_images[2] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball3.png");
#endif
}

BallActor::~BallActor() {
#ifdef __APPLE__
    if (m_window) {
        [m_window closeAndRemove];
        if (m_windowKey) {
            [[BehaviorElementWindowManager shared] unregisterWindow:m_windowKey];
        }
        m_window = nil;
        m_windowKey = nil;
    }
#endif
}

void BallActor::onGooseKick(const Vector2& goosePos, float gooseFootSize, double time) {
    float kickDist = gooseFootSize * 0.8f;
    float dist = Vector2::Distance(m_position.toVector2(), goosePos);
    if (dist < kickDist && time - lastKickTime > KICK_COOLDOWN) {
        Vector2 dir = Vector2::Normalize(Vector2{m_position.x - goosePos.x, m_position.y - goosePos.y});
        velocity = dir * KICK_SPEED;
        speed = KICK_SPEED;
        lastKickTime = time;
        lastAnimateTime = time;
        animationGap = ANIM_GAP_MIN;
        m_wasKicked = true;
        // gooseId is unknown here (the actor doesn't track who kicked it), so -1 means "goose"
        EventBus::Instance().Publish(BallKickedEvent{-1, m_position.x, m_position.y, velocity.x, velocity.y});
        g_assets.Honk();
    }
}

void BallActor::onCursorKick(const Vector2& cursorPos, double time) {
    float dist = std::hypot(cursorPos.x - m_position.x, cursorPos.y - m_position.y);
    if (dist < m_radius && time - lastKickTime > KICK_COOLDOWN) {
        Vector2 dir = Vector2::Normalize(Vector2{m_position.x - cursorPos.x, m_position.y - cursorPos.y});
        velocity = dir * KICK_SPEED;
        speed = KICK_SPEED;
        lastKickTime = time;
        lastAnimateTime = time;
        animationGap = ANIM_GAP_MIN;
        m_wasKicked = true;
        EventBus::Instance().Publish(BallKickedEvent{0, m_position.x, m_position.y, velocity.x, velocity.y});
        g_assets.Honk();
    }
}

void BallActor::tick(WorldContext& ctx, double dt, double time) {
    if (!m_active) return;

    Vector2 delta = velocity * (float)dt;
    m_position = {m_position.x + delta.x, m_position.y + delta.y};
    speed *= (1.0f - DECELERATION * (float)dt);
    velocity = Vector2::Normalize(velocity) * speed;

    if (speed < SPEED_THRESHOLD) {
        speed = 0.0f;
        velocity = {0, 0};
    }

    if (speed > 0.0f) {
        float elapsed = (float)(time - lastAnimateTime);
        if (elapsed > animationGap) {
            currentFrame = (currentFrame + 1) % ANIM_FRAME_COUNT;
            lastAnimateTime = time;
            animationGap = std::max(ANIM_GAP_MIN, ANIM_GAP_MAX - speed / ANIM_SPEED_DIVISOR);
        }
    }

    clampToBounds((float)kBallScreenMargin, (float)g_world.screenWidth, (float)g_world.screenHeight);
}

void BallActor::clampToBounds(float margin, float screenWidth, float screenHeight) {
    if (m_position.x < margin) { m_position.x = margin; velocity.x *= -0.5f; }
    if (m_position.x > screenWidth - margin) { m_position.x = screenWidth - margin; velocity.x *= -0.5f; }
    if (m_position.y < margin) { m_position.y = margin; velocity.y *= -0.5f; }
    if (m_position.y > screenHeight - margin) { m_position.y = screenHeight - margin; velocity.y *= -0.5f; }
}

void BallActor::render(IRenderer* renderer) {
    if (!m_active) return;

#ifdef __APPLE__
    float drawSize = m_radius;
    CGImageRef img = (CGImageRef)m_images[currentFrame % ANIM_FRAME_COUNT];
    float imgDrawW = drawSize;
    float imgDrawH = drawSize;

    if (img) {
        float imgW = (float)CGImageGetWidth(img);
        float imgH = (float)CGImageGetHeight(img);
        float imgScale = drawSize / std::max(imgW, imgH);
        imgDrawW = imgW * imgScale;
        imgDrawH = imgH * imgScale;
    }

    float winW = m_radius * 2.0f;
    float winH = m_radius * 2.0f;
    float winX = m_position.x - winW / 2.0f;
    float winY = m_position.y - winH / 2.0f;
    float imgOffsetX = (winW - imgDrawW) * 0.5f;
    float imgOffsetY = (winH - imgDrawH) * 0.5f;

    if (!m_window) {
        m_window = [[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGImageRef ballImg = (CGImageRef)m_images[currentFrame % ANIM_FRAME_COUNT];
                if (ballImg) {
                    float imgW = (float)CGImageGetWidth(ballImg);
                    float imgH = (float)CGImageGetHeight(ballImg);
                    float imgScale = drawSize / std::max(imgW, imgH);
                    float drawW = imgW * imgScale;
                    float drawH = imgH * imgScale;
                    CGContextDrawImage(cgCtx, CGRectMake(imgOffsetX, imgOffsetY, drawW, drawH), ballImg);
                } else {
                    CGContextSetRGBFillColor(cgCtx, 0.3f, 0.3f, 0.3f, 1.0f);
                    CGContextFillEllipseInRect(cgCtx, CGRectMake(imgOffsetX, imgOffsetY, drawSize, drawSize));
                }
            }
            deviceX:winX deviceY:winY width:winW height:winH];
        m_windowKey = [[BehaviorElementWindowManager shared] registerWindow:m_window];
    } else {
        [m_window updatePosition:winX y:winY width:winW height:winH];
        [(BehaviorElementContentView*)m_window.contentView setNeedsDisplay:YES];
    }
#endif
}
