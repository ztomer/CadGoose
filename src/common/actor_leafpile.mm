// actor_leafpile.mm
// Leaf pile actor implementation — autumn leaves that scatter when goose approaches.

#include "actor_leafpile.h"
#include "random_util.h"
#include "config.h"
#include "world.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#include "render_colors.h"
#include <algorithm>
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
static constexpr float kLeafWidth = 7.5f;
static constexpr float kLeafHeight = 4.5f;
static constexpr float kLeafZCompression = 0.6f;
static constexpr float kLeafAlphaFadeRate = 0.5f;
static constexpr float kLeafAlphaMin = 0.2f;
static constexpr float kLeafVertPadRatio = 0.6f;
static constexpr float kLeafWindowPadding = 20.0f;

static RenderColor LeafColors[4] = {
    {0.8f, 0.4f, 0.1f, 1.0f}, // orange
    {0.9f, 0.6f, 0.2f, 1.0f}, // yellow-orange
    {0.7f, 0.3f, 0.1f, 1.0f}, // brown
    {0.6f, 0.5f, 0.2f, 1.0f}, // golden
};

LeafPileActor::LeafPileActor(const Vector2& pos, float radius, float height, double currentTime)
    : Actor(), m_height(height), m_timeCreated(currentTime), m_timeSinceKicked(-1.0f)
#ifdef __APPLE__
    , m_window(nullptr), m_windowKey(nullptr)
#endif
{
    m_position = {pos.x, pos.y};
    m_active = true;
    m_radius = radius;

    m_leaves.resize(LEAVES_PER_PILE);
    for (int i = 0; i < LEAVES_PER_PILE; i++) {
        float angle = (rng_util::RandRange(360)) * (float)M_PI / 180.0f;
        float r = ((rng_util::RandRange(1000)) / 1000.0f);
        Vector2 val = Vector2{r * cosf(angle), r * sinf(angle)};
        float num = (rng_util::RandRange(1000)) / 1000.0f;
        num *= num;
        m_leaves[i].curPosZ = num * height;
        m_leaves[i].curPosPlanar = val * m_radius * (1.0f - num);
        m_leaves[i].curPosPlanar.y *= 0.6f;
        m_leaves[i].velPlanar = Vector2{0.0f, 0.0f};
        m_leaves[i].velZ = 0.0f;
        m_leaves[i].colorIndex = rng_util::RandRange(4);
    }
}

LeafPileActor::~LeafPileActor() {
#ifdef __APPLE__
    if (m_window) {
        void* windowPtr = m_window;
        void* keyPtr = m_windowKey;
        m_window = nullptr;
        m_windowKey = nullptr;
        closeWindowOnMainThread(^{
            BehaviorElementWindow* win = (__bridge_transfer BehaviorElementWindow*)windowPtr;
            NSNumber* key = (__bridge_transfer NSNumber*)keyPtr;
            [[BehaviorElementWindowManager shared] unregisterWindow:key];
            [win closeAndRemove];
        });
    }
#endif
}

void LeafPileActor::kick(Vector2 kickVelocity, double currentTime, float gooseSpeedPercentage) {
    m_timeSinceKicked = currentTime;
    m_dirty = true; // leaves are about to move — resume per-frame redraws
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
    if (!m_active) return;

    float age = (float)(time - m_timeCreated);
    constexpr float kLifetime = 180.0f;
    constexpr float kFadeTime = 20.0f;

    if (age > kLifetime) {
        m_active = false;
        return;
    } else if (age > kLifetime - kFadeTime) {
        m_alphaMult = 1.0f - (age - (kLifetime - kFadeTime)) / kFadeTime;
    } else {
        m_alphaMult = 1.0f;
    }

    if (m_timeSinceKicked > 0.0f) {
        bool anyMoving = false;
        constexpr float kRestEps = 0.05f;
        for (size_t i = 0; i < m_leaves.size(); i++) {
            m_leaves[i].curPosPlanar = m_leaves[i].curPosPlanar + m_leaves[i].velPlanar * dt;
            m_leaves[i].curPosZ += m_leaves[i].velZ * dt;
            m_leaves[i].velZ += kLeafGravity * dt;
            if (m_leaves[i].curPosZ < 0.0f) {
                m_leaves[i].curPosZ = 0.0f;
                m_leaves[i].velZ *= -0.3f;
                m_leaves[i].velPlanar = m_leaves[i].velPlanar * 0.2f;
            }
            const auto& lf = m_leaves[i];
            if (lf.curPosZ > kRestEps || std::abs(lf.velZ) > kRestEps ||
                std::abs(lf.velPlanar.x) > kRestEps || std::abs(lf.velPlanar.y) > kRestEps) {
                anyMoving = true;
            }
        }
        // Something moved (or just came to rest) this frame → redraw once more.
        m_dirty = true;
        // Fully settled: stop integrating and let render() skip future frames.
        if (!anyMoving) m_timeSinceKicked = -1.0f;
    }
}

void LeafPileActor::render(IRenderer* renderer) {
    if (!m_active) return;

#ifdef __APPLE__
    // A resting pile draws nothing new: leaves only move while kicked, and the
    // only other visible change is the end-of-life alpha fade. Skip the window
    // reposition + 128-leaf redraw when neither changed. The window retains its
    // last-drawn content, so the leaves stay on screen. (Was ~9.7% main-thread.)
    bool fadeChanged = (m_alphaMult != m_lastRenderedAlpha);
    if (m_window && !m_dirty && !fadeChanged) return;

    float scale = g_config.general.globalScale;
    float scaledRadius = m_radius * scale;
    float scaledHeight = m_height * scale;

    // Compute bounding box of all leaves (in scaled coords) to size the window
    // dynamically. Kicked leaves can fly far beyond the initial pile radius.
    // Window stays centered on pile center, so size must cover the farthest leaf
    // edge (position + half-size) in any direction.
    float leafHalfW = kLeafWidth * 0.5f;
    float leafHalfH = kLeafHeight * 0.5f;
    float minX = -scaledRadius - leafHalfW, maxX = scaledRadius + leafHalfW;
    float minY = -scaledRadius - leafHalfH, maxY = scaledRadius + leafHalfH;
    for (size_t i = 0; i < m_leaves.size(); i++) {
        const auto& leaf = m_leaves[i];
        float x = leaf.curPosPlanar.x * scale;
        float y = leaf.curPosPlanar.y * scale - leaf.curPosZ * scale * kLeafZCompression;
        minX = std::min(minX, x - leafHalfW);
        maxX = std::max(maxX, x + leafHalfW);
        minY = std::min(minY, y - leafHalfH);
        maxY = std::max(maxY, y + leafHalfH);
    }

    // Window size covers the full bounding box of all leaves plus padding.
    // Window is centered on the pile, so we need enough room for leaves in all directions.
    float maxDim = std::max(maxX - minX, maxY - minY);
    float winSize = maxDim + kLeafWindowPadding * 2.0f;
    // Center the window on the bounding box center (which may shift when leaves scatter)
    float bboxCenterX = (minX + maxX) * 0.5f;
    float bboxCenterY = (minY + maxY) * 0.5f;
    float winX = m_position.x + bboxCenterX - winSize * 0.5f;
    float winY = m_position.y + bboxCenterY - winSize * 0.5f;

    if (!m_window) {
        m_window = (void*)CFBridgingRetain([[BehaviorElementWindow alloc]
            initWithDrawBlock:^(CGContextRef cgCtx) {
                CGRenderer r(cgCtx);
                r.SaveState();

                // Recompute everything inside the block — leaf positions change every frame
                float bMinX = -scaledRadius - leafHalfW, bMaxX = scaledRadius + leafHalfW;
                float bMinY = -scaledRadius - leafHalfH, bMaxY = scaledRadius + leafHalfH;
                for (size_t i = 0; i < m_leaves.size(); i++) {
                    const auto& leaf = m_leaves[i];
                    float lx = leaf.curPosPlanar.x * scale;
                    float ly = leaf.curPosPlanar.y * scale - leaf.curPosZ * scale * kLeafZCompression;
                    bMinX = std::min(bMinX, lx - leafHalfW);
                    bMaxX = std::max(bMaxX, lx + leafHalfW);
                    bMinY = std::min(bMinY, ly - leafHalfH);
                    bMaxY = std::max(bMaxY, ly + leafHalfH);
                }
                float bCenterX = (bMinX + bMaxX) * 0.5f;
                float bCenterY = (bMinY + bMaxY) * 0.5f;
                float bMaxDim = std::max(bMaxX - bMinX, bMaxY - bMinY);
                float bWinSize = bMaxDim + kLeafWindowPadding * 2.0f;

                r.Translate(bWinSize * 0.5f, bWinSize * 0.5f);

                for (size_t i = 0; i < m_leaves.size(); i++) {
                    const auto& leaf = m_leaves[i];
                    float x = leaf.curPosPlanar.x * scale - bCenterX;
                    float y = leaf.curPosPlanar.y * scale - leaf.curPosZ * scale * kLeafZCompression - bCenterY;
                    float z = leaf.curPosZ * scale;
                    float alpha = 1.0f - (z / scaledHeight) * kLeafAlphaFadeRate;
                    if (alpha < kLeafAlphaMin) alpha = kLeafAlphaMin;
                    alpha *= m_alphaMult;

                    RenderColor color = LeafColors[leaf.colorIndex];
                    color.a = alpha;
                    r.DrawEllipse({x, y}, kLeafWidth, kLeafHeight, color);
                }

                r.RestoreState();
            }
            deviceX:winX deviceY:winY width:winSize height:winSize]);
        BehaviorElementWindow* newWin = (__bridge BehaviorElementWindow*)m_window;
        newWin.level = NSStatusWindowLevel - 12; // Ground layer — above footprints, below flowers
        m_windowKey = (void*)CFBridgingRetain([[BehaviorElementWindowManager shared] registerWindow:newWin]);
    } else {
        BehaviorElementWindow* win = (__bridge BehaviorElementWindow*)m_window;
        [win updatePosition:winX y:winY width:winSize height:winSize];
        [[win contentView] setNeedsDisplay:YES];
    }

    m_dirty = false;
    m_lastRenderedAlpha = m_alphaMult;
#endif
}
