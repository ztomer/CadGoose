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
    m_position = {pos.x, pos.y};
    m_active = true;
    m_radius = JAIL_SIZE * 0.5f;
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
    // Jail stays at its m_position until explicitly removed
}

void JailActor::render(IRenderer* renderer) {
    if (!m_active) return;

#ifdef __APPLE__
    float winSize = JAIL_SIZE;
    float winX = m_position.x - winSize / 2.0f;
    float winY = m_position.y - winSize / 2.0f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();

                // Jail cage drawn in window-local coords (0..winSize), so it stays
                // inside the BehaviorElementWindow's content view instead of clipping
                // against negative-coord top/left edges.
                r.DrawRectOutline({0, 0, winSize, winSize}, {0.3f, 0.3f, 0.3f, 0.9f}, 2.0f);

                // Vertical bars
                for (int i = 1; i < 4; i++) {
                    float x = (winSize / 4.0f) * i;
                    r.DrawLine({x, 0}, {x, winSize}, {0.3f, 0.3f, 0.3f, 0.6f}, 1.5f);
                }

                // Horizontal bars
                for (int i = 1; i < 4; i++) {
                    float y = (winSize / 4.0f) * i;
                    r.DrawLine({0, y}, {winSize, y}, {0.3f, 0.3f, 0.3f, 0.6f}, 1.5f);
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
