#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "renderer_interface.h"
#include "platform_input_mock.h"

struct MockHatsRenderer : public IRenderer {
    int saveCount = 0;
    int restoreCount = 0;
    int drawImageCount = 0;
    float translateX = 0, translateY = 0;
    float scaleX = 0, scaleY = 0;

    void SaveState() override { ++saveCount; }
    void RestoreState() override { ++restoreCount; }
    void Translate(float x, float y) override { translateX = x; translateY = y; }
    void Scale(float sx, float sy) override { scaleX = sx; scaleY = sy; }
    void DrawImage(void*, RenderRect) override { ++drawImageCount; }
    bool GetImageSize(void* image, float* w, float* h) override {
        if (image) { *w = 32; *h = 32; return true; }
        *w = 0; *h = 0; return false;
    }

    void DrawEllipse(RenderPoint, float, float, RenderColor) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override {}
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    void DrawText(const char*, RenderPoint, RenderColor, float) override {}
    float MeasureText(const char*, float) override { return 0; }
    void SetAlpha(float) override {}
    void Rotate(float) override {}
};

class BehaviorHatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("hats")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new Goose(1, "goose", 1920, 1080);
        goose->behaviorsEnabled = true;
        goose->pos = {500, 500};
        goose->dir = 0;
        goose->rig.neckHead = {500, 460};
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
        PlatformInputMock_Reset();
    }

    void TearDown() override {
        delete goose;
        PlatformInputMock_Reset();
    }

    Goose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorHatsTest, Init) {
    auto* b = BehaviorRegistry::Instance().Get("hats");
    ASSERT_NE(b, nullptr);
    b->init(ctx);
}

TEST_F(BehaviorHatsTest, RenderWithNullRenderer) {
    auto* b = BehaviorRegistry::Instance().Get("hats");
    ASSERT_NE(b, nullptr);
    b->init(ctx);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorHatsTest, RenderWithImage) {
    auto* b = BehaviorRegistry::Instance().Get("hats");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {500, 500};
    goose->dir = 0;
    goose->rig.neckHead = {500, 460};

    MockHatsRenderer renderer;
    b->render(goose, ctx, &renderer);

    EXPECT_GE(renderer.saveCount, 1);
    EXPECT_GE(renderer.restoreCount, 1);
    EXPECT_GE(renderer.drawImageCount, 1);
}

TEST_F(BehaviorHatsTest, TickDoesNothing) {
    auto* b = BehaviorRegistry::Instance().Get("hats");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->tick(goose, ctx, 0.016, 0.0);
}

TEST_F(BehaviorHatsTest, CleanupFunction) {
    auto* b = BehaviorRegistry::Instance().Get("hats");
    ASSERT_NE(b, nullptr);
    b->cleanup(ctx);
}

TEST_F(BehaviorHatsTest, RenderFacingLeft) {
    auto* b = BehaviorRegistry::Instance().Get("hats");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {500, 500};
    goose->dir = 180;
    goose->rig.neckHead = {500, 460};

    MockHatsRenderer renderer;
    b->render(goose, ctx, &renderer);

    EXPECT_GE(renderer.saveCount, 1);
    EXPECT_GE(renderer.restoreCount, 1);
    EXPECT_GE(renderer.drawImageCount, 1);
    EXPECT_LT(renderer.scaleX, 0);
}

TEST_F(BehaviorHatsTest, RenderBabyStalinHeadScale) {
    auto* b = BehaviorRegistry::Instance().Get("hats");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Goose stalin(2, "baby_stalin", 1920, 1080);
    stalin.behaviorsEnabled = true;
    stalin.pos = {500, 500};
    stalin.dir = 0;
    stalin.rig.neckHead = {500, 460};

    MockHatsRenderer renderer;
    b->render(&stalin, ctx, &renderer);

    EXPECT_GE(renderer.saveCount, 1);
    EXPECT_GE(renderer.restoreCount, 1);
    EXPECT_GE(renderer.drawImageCount, 1);
}
