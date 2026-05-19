// actor_leafpile.mm
// Leaf pile actor implementation — autumn leaves that scatter when goose approaches.

#include "actor_leafpile.h"
#include "random_util.h"
#include "config.h"
#include "world.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#include "render_colors.h"
#include <cmath>

#ifdef __APPLE__
#include "behavior_element_window.h"
#include <CoreGraphics/CoreGraphics.h>
#include <Foundation/Foundation.h>
#endif

static constexpr int kLeafRandomSpeed = 200;
static constexpr float kLeafSpeedMultiplier = 200.0f;
static constexpr float kLeafSpeedFactor = 0.2f;
static constexpr float kLeafVelZMin = 10.0f;
static constexpr float kLeafVelZRange = 490;
static constexpr float kLeafGravity = -900.0f;

static RenderColor LeafColors[4] = {
    {0.8f, 0.4f, 0.1f, 1.0f}, // orange
    {0.9f, 0.6f, 0.2f, 1.0f}, // yellow-orange
    {0.7f, 0.3f, 0.1f, 1.0f}, // brown
    {0.6f, 0.5f, 0.2f, 1.0f}, // golden
};

LeafPileActor::LeafPileActor(const Vector2& pos, float radius, float height, double currentTime)
    : Actor(), m_radius(radius), m_height(height), m_timeCreated(currentTime), m_timeSinceKicked(-1.0f)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    position = {pos.x, pos.y};
    active = true;
    this->radius = radius;

    m_leaves.resize(LEAVES_PER_PILE);
    for (int i = 0; i < LEAVES_PER_PILE; i++) {
        float angle = (rng_util::RandRange(360)) * (float)M_PI / 180.0f;
        float r = ((rng_util::RandRange(1000)) / 1000.0f);
        Vector2 val = Vector2{r * cosf(angle), r * sinf(angle)};
        float num = (rng_util::RandRange(1000)) / 1000.0f;
        num *= num;
        m_leaves[i].curPosZ = num * height;
        m_leaves[i].curPosPlanar = val * radius * (1.0f - num);
        m_leaves[i].curPosPlanar.y *= 0.6f;
        m_leaves[i].velPlanar = Vector2{0.0f, 0.0f};
        m_leaves[i].velZ = 0.0f;
        m_leaves[i].colorIndex = rng_util::RandRange(4);
    }
}

LeafPileActor::~LeafPileActor() {
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

void LeafPileActor::kick(Vector2 kickVelocity, double currentTime, float gooseSpeedPercentage) {
    m_timeSinceKicked = currentTime;
    float num = Lerp(0.6f, 1.1f, gooseSpeedPercentage);
    for (size_t i = 0; i < m_leaves.size(); i++) {
        Vector2 val = Vector2::Normalize(m_leaves[i].curPosPlanar);
        float dot = Dot(val, Vector2::Normalize(kickVelocity));
        float num2 = 1.0f - std::abs(dot);
        float randSpeed = (rng_util::RandRange(kLeafRandomSpeed));
        Vector2 val2 = val * randSpeed;
        val2 = val2 + val * num2 * kLeafSpeedMultiplier * kLeafSpeedFactor;
        val2 = val2 + (kickVelocity - val2) * 0.3f;
        float num3 = kLeafVelZMin + (rng_util::RandRange((int)kLeafVelZRange));
        num3 *= Lerp(0.9f, 1.1f, num2);
        m_leaves[i].velPlanar = val2 * num;
        m_leaves[i].velZ = num3 * num;
    }
}

void LeafPileActor::tick(WorldContext& ctx, double dt, double time) {
    if (!active) return;

    if (m_timeSinceKicked > 0.0f) {
        for (size_t i = 0; i < m_leaves.size(); i++) {
            m_leaves[i].curPosPlanar = m_leaves[i].curPosPlanar + m_leaves[i].velPlanar * dt;
            m_leaves[i].curPosZ += m_leaves[i].velZ * dt;
            m_leaves[i].velZ += kLeafGravity * dt;
            if (m_leaves[i].curPosZ < 0.0f) {
                m_leaves[i].curPosZ = 0.0f;
                m_leaves[i].velZ *= -0.3f;
                m_leaves[i].velPlanar = m_leaves[i].velPlanar * 0.2f;
            }
        }
    }
}

void LeafPileActor::render(IRenderer* renderer) {
    if (!active) return;

#ifdef __APPLE__
    float winSize = m_radius * 2.0f + 20.0f;
    float winX = position.x - winSize / 2.0f;
    float winY = position.y - winSize / 2.0f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();
                r.Translate(winSize * 0.5f, winSize * 0.5f);

                for (size_t i = 0; i < m_leaves.size(); i++) {
                    const auto& leaf = m_leaves[i];
                    float x = leaf.curPosPlanar.x;
                    float y = -leaf.curPosPlanar.y; // flip Y for screen coords
                    float z = leaf.curPosZ;
                    float alpha = 1.0f - (z / m_height) * 0.5f;
                    if (alpha < 0.2f) alpha = 0.2f;

                    RenderColor color = LeafColors[leaf.colorIndex];
                    color.a = alpha;
                    r.DrawEllipse({x, y}, 2.0f, 1.5f, color);
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
