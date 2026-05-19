// actor_toy.mm
// Toy actor implementation — stick or ball on the ground with its own window.

#include "actor_toy.h"
#include "actor.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "renderer_interface.h"
#include "render_colors.h"
#include "cg_renderer.h"
#include <cmath>

#ifdef __APPLE__
#include "behavior_element_window.h"
#include <CoreGraphics/CoreGraphics.h>
#include <Foundation/Foundation.h>
#endif

ToyActor::ToyActor(Type type, const Vector2& pos, int instanceId)
    : Actor(), m_type(type), m_angle(0), m_spawnTime(0), m_instanceId(instanceId)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    position = {pos.x, pos.y};
    active = true;
    m_spawnTime = g_time;
    m_angle = (float)(rand() % 360);

    radius = (type == Stick) ? 12.0f : 15.0f;

#ifdef __APPLE__
    m_images[0] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/stick.png");
    m_images[1] = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/ball.png");
#endif
}

ToyActor::~ToyActor() {
#ifdef __APPLE__
    if (m_window) {
        BehaviorElementWindow* win = (__bridge BehaviorElementWindow*)m_window;
        [win closeAndRemove];
        if (m_windowKey) {
            [[BehaviorElementWindowManager shared] unregisterWindow:(__bridge NSNumber*)m_windowKey];
        }
        m_window = nullptr;
        m_windowKey = nullptr;
    }
#endif
}

void ToyActor::tick(double dt, double time) {
    if (!active) return;

    double age = time - m_spawnTime;
    if (age > TOY_LIFETIME) {
        active = false;
    }
}

void ToyActor::render(IRenderer* renderer) {
    if (!active) return;

#ifdef __APPLE__
    double age = g_time - m_spawnTime;
    float alpha = std::min(1.0f, (float)age / 0.5f);

    float winSize = (m_type == Stick) ? STICK_LENGTH : BALL_RADIUS * 2.0f;
    float winX = position.x - winSize / 2.0f;
    float winY = position.y - winSize / 2.0f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();
                r.Translate(winSize * 0.5f, winSize * 0.5f);

                if (m_type == Stick) {
                    float rad = m_angle * (float)M_PI / 180.0f;
                    float halfLen = STICK_LENGTH / 2.0f;
                    float halfWidth = STICK_WIDTH / 2.0f;
                    float cosA = std::cos(rad);
                    float sinA = std::sin(rad);

                    RenderPoint corners[4] = {
                        {-halfLen * cosA + halfWidth * sinA, -halfLen * sinA - halfWidth * cosA},
                        {halfLen * cosA + halfWidth * sinA, halfLen * sinA - halfWidth * cosA},
                        {halfLen * cosA - halfWidth * sinA, halfLen * sinA + halfWidth * cosA},
                        {-halfLen * cosA - halfWidth * sinA, -halfLen * sinA + halfWidth * cosA},
                    };
                    r.DrawPolygon(corners, 4, MakeStickBrown(alpha));
                } else {
                    r.DrawEllipse({0, 0}, BALL_RADIUS, BALL_RADIUS, MakeToyBallRed(alpha));
                }

                r.RestoreState();
            }
            deviceX:winX deviceY:winY width:winSize height:winSize]);
        BehaviorElementWindow* newWin = (__bridge BehaviorElementWindow*)m_window;
        m_windowKey = (void*)CFBridgingRetain([[BehaviorElementWindowManager shared] registerWindow:newWin]);
    } else {
        BehaviorElementWindow* win = (__bridge BehaviorElementWindow*)m_window;
        [win updatePosition:winX y:winY width:winSize height:winSize];
        [[win contentView] setNeedsDisplay:YES];
    }
#endif
}
