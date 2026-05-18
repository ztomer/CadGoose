// actor_breadcrumb.mm
// Breadcrumb actor implementation — dropped at cursor, expires after lifetime.

#include "actor_breadcrumb.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#include <cmath>

#ifdef __APPLE__
#include "behavior_element_window.h"
#include <CoreGraphics/CoreGraphics.h>
#include <Foundation/Foundation.h>
#endif

BreadcrumbActor::BreadcrumbActor(const Vector2& pos, double spawnTime, float lifetime)
    : Actor(), m_spawnTime(spawnTime), m_lifetime(lifetime), m_image(nullptr)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    position = {pos.x, pos.y};
    active = true;
    radius = CRUMB_SIZE * 0.5f;

#ifdef __APPLE__
    m_image = (void*)g_assets.GetBehaviorImage("Assets/Images/OtherGfx/crumbs.png");
#endif
}

BreadcrumbActor::~BreadcrumbActor() {
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

void BreadcrumbActor::tick(double dt, double time) {
    (void)dt;
    if (!active) return;

    if (time - m_spawnTime > m_lifetime) {
        active = false;
    }
}

void BreadcrumbActor::render(IRenderer* renderer) {
    if (!active) return;

#ifdef __APPLE__
    double age = g_time - m_spawnTime;
    float alpha = std::min(1.0f, (float)(m_lifetime - age) / 2.0f);
    if (alpha < 0) alpha = 0;

    float winSize = CRUMB_SIZE;
    float winX = position.x - winSize / 2.0f;
    float winY = position.y - winSize / 2.0f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();

                if (m_image) {
                    CGImageRef img = (CGImageRef)m_image;
                    float w = (float)CGImageGetWidth(img);
                    float h = (float)CGImageGetHeight(img);
                    float scale = winSize / w;
                    r.Translate(-w * 0.5f * scale, -h * 0.5f * scale);
                    r.Scale(scale, scale);
                    r.SetAlpha(alpha);
                    r.DrawImage(img, {0, 0, w, h});
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
