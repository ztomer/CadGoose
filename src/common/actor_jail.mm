// actor_jail.mm
// Jail actor implementation — cage that traps the goose.

#include "actor_jail.h"
#include "config.h"
#include "world.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#include <cmath>

#ifdef __APPLE__
#include "behavior_element_window.h"
#include <CoreGraphics/CoreGraphics.h>
#include <Foundation/Foundation.h>
#endif

JailActor::JailActor(const Vector2& pos)
    : Actor()
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    position = {pos.x, pos.y};
    active = true;
    radius = JAIL_SIZE * 0.5f;
}

JailActor::~JailActor() {
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

void JailActor::tick(WorldContext& ctx, double dt, double time) {
    (void)dt; (void)time;
    // Jail stays at its position until explicitly removed
}

void JailActor::render(IRenderer* renderer) {
    if (!active) return;

#ifdef __APPLE__
    float winSize = JAIL_SIZE;
    float winX = position.x - winSize / 2.0f;
    float winY = position.y - winSize / 2.0f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();

                // Jail cage - rectangle with bars
                float half = winSize * 0.5f;
                r.DrawRectOutline({-half, -half, winSize, winSize}, {0.3f, 0.3f, 0.3f, 0.9f}, 2.0f);

                // Vertical bars
                for (int i = 1; i < 4; i++) {
                    float x = -half + (winSize / 4.0f) * i;
                    r.DrawLine({x, -half}, {x, half}, {0.3f, 0.3f, 0.3f, 0.6f}, 1.5f);
                }

                // Horizontal bars
                for (int i = 1; i < 4; i++) {
                    float y = -half + (winSize / 4.0f) * i;
                    r.DrawLine({-half, y}, {half, y}, {0.3f, 0.3f, 0.3f, 0.6f}, 1.5f);
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
