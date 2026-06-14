#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "renderer_interface.h"

struct MockNametagRenderer : public IRenderer {
    int saveCount = 0;
    int restoreCount = 0;
    int roundedRectCount = 0;
    int textCount = 0;
    int measureCount = 0;
    float measureReturn = 0;

    void SaveState() override { ++saveCount; }
    void RestoreState() override { ++restoreCount; }
    void DrawRoundedRect(RenderRect, float, RenderColor) override { ++roundedRectCount; }
    void DrawText(const char*, RenderPoint, RenderColor, float) override { ++textCount; }
    float MeasureText(const char*, float) override { ++measureCount; return measureReturn; }
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipse(RenderPoint, float, float, RenderColor) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    void DrawImage(void*, RenderRect) override {}
    bool GetImageSize(void*, float*, float*) override { return false; }
    void SetAlpha(float) override {}
};

class BehaviorNametagTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("nametag")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new Goose(1, "TestGoose", 1920, 1080);
        goose->behaviorsEnabled = true;
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        delete goose;
    }

    Goose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorNametagTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("nametag");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BehaviorState>(1, "nametag");
    ASSERT_NE(state, nullptr);
}

TEST_F(BehaviorNametagTest, TickIsNoOp) {
    auto* b = BehaviorRegistry::Instance().Get("nametag");
    ASSERT_NE(b, nullptr);
    b->tick(goose, ctx, 0.016, 0.0);
}

TEST_F(BehaviorNametagTest, RenderNull) {
    auto* b = BehaviorRegistry::Instance().Get("nametag");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorNametagTest, RenderEmptyName) {
    auto* b = BehaviorRegistry::Instance().Get("nametag");
    ASSERT_NE(b, nullptr);
    goose->name = "";
    MockNametagRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.saveCount, 0);
}

TEST_F(BehaviorNametagTest, RenderWithName) {
    auto* b = BehaviorRegistry::Instance().Get("nametag");
    ASSERT_NE(b, nullptr);
    goose->name = "Goose";
    goose->rig.neckHead = {100, 100};

    MockNametagRenderer renderer;
    renderer.measureReturn = 40.0f;

    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.saveCount, 1);
    EXPECT_EQ(renderer.restoreCount, 1);
    EXPECT_EQ(renderer.measureCount, 1);
    EXPECT_EQ(renderer.roundedRectCount, 2);
    EXPECT_EQ(renderer.textCount, 1);
}

TEST_F(BehaviorNametagTest, Cleanup) {
    auto* b = BehaviorRegistry::Instance().Get("nametag");
    ASSERT_NE(b, nullptr);
    ASSERT_NE(b->cleanup, nullptr);
    b->cleanup(ctx);
}

TEST_F(BehaviorNametagTest, RenderFallsBackToCharWidth) {
    auto* b = BehaviorRegistry::Instance().Get("nametag");
    ASSERT_NE(b, nullptr);
    goose->name = "X";
    goose->rig.neckHead = {100, 100};

    MockNametagRenderer renderer;
    renderer.measureReturn = 0.0f;

    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.saveCount, 1);
    EXPECT_EQ(renderer.restoreCount, 1);
    EXPECT_EQ(renderer.measureCount, 1);
    EXPECT_EQ(renderer.roundedRectCount, 2);
    EXPECT_EQ(renderer.textCount, 1);
}
