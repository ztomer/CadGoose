// test_goose_render.mm — Goose::draw / render / drawBody / tick.
//
// These sat at 0% because nothing drove the goose through a real rendering
// context or a full tick. They need no window: a CGBitmapContext wrapped in the
// production CGRenderer exercises the same code path the app uses, and we can
// read the pixels back to prove the goose actually rendered.

#import <CoreGraphics/CoreGraphics.h>
#include <gtest/gtest.h>
#include <vector>

#include "goose.h"
#include "world.h"
#include "config.h"
#include "actor_manager.h"
#include "cg_renderer.h"
#include "behavior_registry.h"

namespace {

constexpr int kW = 256;
constexpr int kH = 256;

class GooseCanvas {
public:
    GooseCanvas() : m_pixels(kW * kH * 4, 0) {
        m_cs = CGColorSpaceCreateDeviceRGB();
        m_ctx = CGBitmapContextCreate(m_pixels.data(), kW, kH, 8, kW * 4, m_cs,
                                      kCGImageAlphaPremultipliedLast);
    }
    ~GooseCanvas() {
        if (m_ctx) CGContextRelease(m_ctx);
        if (m_cs) CGColorSpaceRelease(m_cs);
    }

    CGContextRef ctx() const { return m_ctx; }

    int nonZeroPixels() const {
        int n = 0;
        for (size_t i = 3; i < m_pixels.size(); i += 4) {
            if (m_pixels[i] != 0) ++n;
        }
        return n;
    }

private:
    std::vector<unsigned char> m_pixels;
    CGColorSpaceRef m_cs = nullptr;
    CGContextRef m_ctx = nullptr;
};

// Owns a goose registered with the ActorManager and removes it again, so the
// manager's geese cache does not leak across tests.
class GooseFixture : public ::testing::Test {
protected:
    void SetUp() override {
        m_goose = new Goose(1, "Gander", kW, kH);
        m_goose->pos = {128.0f, 128.0f};
        ActorManager::Instance().add(m_goose);
    }

    void TearDown() override {
        ActorManager::Instance().remove(m_goose);
        delete m_goose;
        m_goose = nullptr;
    }

    Goose* m_goose = nullptr;
};

}  // namespace

TEST_F(GooseFixture, DrawPutsPixelsOnTheCanvas) {
    GooseCanvas canvas;
    CGRenderer renderer(canvas.ctx());

    m_goose->draw(&renderer);

    EXPECT_GT(canvas.nonZeroPixels(), 0)
        << "Goose::draw produced an entirely empty canvas";
}

TEST_F(GooseFixture, RenderDelegatesToDraw) {
    GooseCanvas viaRender;
    GooseCanvas viaDraw;
    CGRenderer r1(viaRender.ctx());
    CGRenderer r2(viaDraw.ctx());

    m_goose->render(&r1);
    m_goose->draw(&r2);

    // render() is documented as a pass-through to draw(); same goose state in,
    // same amount of ink out.
    EXPECT_GT(viaRender.nonZeroPixels(), 0);
    EXPECT_EQ(viaRender.nonZeroPixels(), viaDraw.nonZeroPixels())
        << "render() should be equivalent to draw()";
}

TEST_F(GooseFixture, DrawToleratesNullRenderer) {
    m_goose->draw(nullptr);
    m_goose->render(nullptr);
    SUCCEED() << "a null renderer must be a no-op, not a crash";
}

TEST_F(GooseFixture, DrawToleratesRendererWithoutNativeContext) {
    // A renderer whose nativeContext() is null (the IRenderer default) must be
    // rejected before any CoreGraphics call is attempted.
    struct NullContextRenderer : IRenderer {
        void SaveState() override {}
        void RestoreState() override {}
        void Translate(float, float) override {}
        void Scale(float, float) override {}
        void Rotate(float) override {}
        void DrawEllipse(RenderPoint, float, float, RenderColor) override {}
        void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
        void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
        void DrawRect(RenderRect, RenderColor) override {}
        void DrawRectOutline(RenderRect, RenderColor, float) override {}
        void DrawRoundedRect(RenderRect, float, RenderColor) override {}
        void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
        void DrawImage(void*, RenderRect) override {}
        bool GetImageSize(void*, float*, float*) override { return false; }
        void DrawText(const char*, RenderPoint, RenderColor, float) override {}
        float MeasureText(const char*, float) override { return 0.0f; }
        void SetAlpha(float) override {}
        // nativeContext() intentionally left as the base implementation (nullptr).
    };

    NullContextRenderer r;
    m_goose->draw(&r);
    SUCCEED() << "a renderer without a native context must be a no-op";
}

TEST_F(GooseFixture, DrawLeavesTheContextTransformBalanced) {
    GooseCanvas canvas;
    CGRenderer renderer(canvas.ctx());

    const CGAffineTransform before = CGContextGetCTM(canvas.ctx());
    m_goose->draw(&renderer);
    const CGAffineTransform after = CGContextGetCTM(canvas.ctx());

    // draw() wraps each behavior render pass in save/scale/restore. If any of
    // those were unbalanced the CTM would leak out and silently corrupt
    // everything drawn after the goose in the same frame.
    EXPECT_TRUE(CGAffineTransformEqualToTransform(before, after))
        << "draw() leaked a transform: a save/restore pair is unbalanced";
}

TEST_F(GooseFixture, DrawIsStableAcrossGlobalScaleSettings) {
    const float saved = g_config.general.globalScale;

    // globalScale is applied around the behavior render passes in Goose::draw
    // and again inside the body drawing code. This test does not pin down the
    // exact resulting size — it pins down that both settings render something
    // and neither corrupts the context.
    for (float scale : {0.5f, 1.0f, 2.0f}) {
        g_config.general.globalScale = scale;
        GooseCanvas canvas;
        CGRenderer renderer(canvas.ctx());
        const CGAffineTransform before = CGContextGetCTM(canvas.ctx());
        m_goose->draw(&renderer);

        EXPECT_GT(canvas.nonZeroPixels(), 0) << "no ink at globalScale=" << scale;
        EXPECT_TRUE(CGAffineTransformEqualToTransform(before, CGContextGetCTM(canvas.ctx())))
            << "transform leaked at globalScale=" << scale;
    }

    g_config.general.globalScale = saved;
}

TEST_F(GooseFixture, DrawBodyRendersDirectlyIntoAContext) {
    GooseCanvas canvas;
    m_goose->drawBody(canvas.ctx());
    EXPECT_GT(canvas.nonZeroPixels(), 0) << "drawBody drew nothing";
}

TEST_F(GooseFixture, TickAdvancesWithoutACursorProvider) {
    WorldContext world{};
    world.screenWidth = kW;
    world.screenHeight = kH;

    // Drives Update() + the behavior registry through the public tick entry
    // point. No cursor provider is installed, so the cursor-read and
    // cursor-execute branches both take their null path.
    for (int i = 0; i < 5; ++i) {
        m_goose->tick(world, 1.0 / 60.0, 0.1 * i);
    }

    // The goose must remain inside the world it was ticked in.
    EXPECT_GE(m_goose->pos.x, -100.0f);
    EXPECT_LE(m_goose->pos.x, static_cast<float>(kW) + 100.0f);
    EXPECT_GE(m_goose->pos.y, -100.0f);
    EXPECT_LE(m_goose->pos.y, static_cast<float>(kH) + 100.0f);
}

TEST_F(GooseFixture, TickThenDrawStillRenders) {
    WorldContext world{};
    world.screenWidth = kW;
    world.screenHeight = kH;
    for (int i = 0; i < 10; ++i) m_goose->tick(world, 1.0 / 60.0, 0.05 * i);

    GooseCanvas canvas;
    CGRenderer renderer(canvas.ctx());
    m_goose->draw(&renderer);

    EXPECT_GT(canvas.nonZeroPixels(), 0)
        << "a ticked goose must still render";
}
