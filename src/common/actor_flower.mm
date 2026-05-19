// actor_flower.mm
// Flower actor implementation — grows from ground with stem and petals.

#include "actor_flower.h"
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

FlowerActor::FlowerActor(const Vector2& pos, float hue, double spawnTime)
    : Actor(), m_hue(hue), m_growth(0), m_stemHeight(0), m_petalSize(0), m_spawnTime(spawnTime)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    m_position = {pos.x, pos.y};
    m_active = true;
    m_radius = MAX_PETAL_SIZE + 5.0f;
}

FlowerActor::~FlowerActor() {
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

void FlowerActor::tick(WorldContext& ctx, double dt, double time) {
    if (!m_active) return;

    double age = time - m_spawnTime;
    float growTime = g_config.behaviors.interactiveDrops.flowerGrowTime;

    if (age > FLOWER_LIFETIME) {
        m_active = false;
        return;
    }

    if (age < growTime) {
        float p = (float)age / growTime;
        m_growth = p;
        m_stemHeight = MAX_STEM_HEIGHT * p;
        m_petalSize = MAX_PETAL_SIZE * p;
    } else {
        m_growth = 1.0f;
        m_stemHeight = MAX_STEM_HEIGHT;
        m_petalSize = MAX_PETAL_SIZE;
    }
}

void FlowerActor::render(IRenderer* renderer) {
    if (!m_active || m_growth < GROWTH_THRESHOLD) return;

#ifdef __APPLE__
    float winSize = m_stemHeight + m_petalSize * 2.0f + 10.0f;
    float winX = m_position.x - winSize / 2.0f;
    float winY = m_position.y - m_stemHeight - m_petalSize * 2.0f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();
                r.Translate(winSize * 0.5f, winSize * 0.5f);

                // Stem
                float stemX = 0;
                float stemTopY = -m_stemHeight;
                float stemBottomY = 0;
                r.DrawLine({stemX, stemTopY}, {stemX, stemBottomY}, {0.2f, 0.6f, 0.2f, 1.0f}, 2.0f);

                // Petals (5 petals)
                float centerX = 0;
                float centerY = stemTopY;
                float petalAngle = 72.0f;
                for (int i = 0; i < 5; i++) {
                    float angle = (i * petalAngle) * (float)M_PI / 180.0f;
                    float px = centerX + std::cos(angle) * m_petalSize;
                    float py = centerY + std::sin(angle) * m_petalSize;
                    r.DrawEllipse({px, py}, m_petalSize * 0.5f, m_petalSize * 0.5f, {m_hue / 360.0f, 0.7f, 0.8f, 1.0f});
                }

                // Center
                r.DrawEllipse({centerX, centerY}, m_petalSize * 0.3f, m_petalSize * 0.3f, {1.0f, 0.9f, 0.3f, 1.0f});

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
