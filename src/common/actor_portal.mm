// actor_portal.mm
// Portal actor implementation — teleportation point (A or B).

#include "actor_portal.h"
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

PortalActor::PortalActor(Type type, const Vector2& pos)
    : Actor(), m_type(type), m_portalId(type == PortalA ? 1 : 2), m_image(nullptr)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    m_position = {pos.x, pos.y};
    m_active = true;
    m_radius = PORTAL_SIZE * 0.5f;

#ifdef __APPLE__
    const char* imageName = (type == PortalA) ? "Assets/Images/OtherGfx/p1.png" : "Assets/Images/OtherGfx/p2.png";
    m_image = (void*)g_assets.GetBehaviorImage(imageName);
#endif
}

PortalActor::~PortalActor() {
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

void PortalActor::tick(WorldContext& ctx, double dt, double time) {
    (void)dt; (void)time;
    // Portal stays at its m_position until explicitly removed
}

void PortalActor::render(IRenderer* renderer) {
    if (!m_active) return;

#ifdef __APPLE__
    float winSize = PORTAL_SIZE;
    float winX = m_position.x - winSize / 2.0f;
    float winY = m_position.y - winSize / 2.0f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();

                if (m_image) {
                    CGImageRef img = (CGImageRef)m_image;
                    float w = (float)CGImageGetWidth(img);
                    float h = (float)CGImageGetHeight(img);
                    // Window already enclosing the portal; fill it with the image.
                    // Aspect-preserve so non-square portal art stays centered.
                    float scale = winSize / std::max(w, h);
                    float drawW = w * scale;
                    float drawH = h * scale;
                    float offX = (winSize - drawW) * 0.5f;
                    float offY = (winSize - drawH) * 0.5f;
                    r.DrawImage(img, {offX, offY, drawW, drawH});
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
