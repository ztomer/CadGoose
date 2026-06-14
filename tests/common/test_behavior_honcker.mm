#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "event_bus.h"
#include "hotkey.h"
#include "renderer_interface.h"
#include "behaviors/states/honcker_state.h"
#include "platform_input_mock.h"

void Honcker_Honk(Goose* goose, double time);

struct MockHonkRenderer : public IRenderer {
    int ellipseCount = 0;
    int imageCount = 0;

    void DrawEllipse(RenderPoint, float, float, RenderColor) override { ++ellipseCount; }
    void DrawImage(void*, RenderRect) override { ++imageCount; }
    bool GetImageSize(void* img, float* w, float* h) override { if (img && w && h) { *w = 32; *h = 32; return true; } return false; }
    void SaveState() override {}
    void RestoreState() override {}
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override {}
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    void DrawText(const char*, RenderPoint, RenderColor, float) override {}
    float MeasureText(const char*, float) override { return 0; }
    void SetAlpha(float) override {}
};

struct HonkSpyGoose : public Goose {
    int honkCount = 0;
    HonkSpyGoose(int id) : Goose(id, "honker", 1920, 1080) { behaviorsEnabled = true; m_canHonk = true; }
    void onHonk() override { ++honkCount; }
};

class BehaviorHonckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("honcker")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new HonkSpyGoose(1);
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
        PlatformInputMock_Reset();
    }

    void TearDown() override {
        delete goose;
        PlatformInputMock_Reset();
    }

    int honkerKeyCode() const {
        return KeyNameToKeyCode(g_config.behaviors.honcker.hotkey);
    }

    HonkSpyGoose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorHonckerTest, InitDoesNotCreateState) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<HonckerState>(1, "honcker");
    EXPECT_EQ(state, nullptr);
}

TEST_F(BehaviorHonckerTest, HonkMakesVisible) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Honcker_Honk(goose, 1.0);

    auto* state = BehaviorStateManager::Instance().Get<HonckerState>(1, "honcker");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->visible);
    EXPECT_EQ(state->lastShowTime, 1.0);
    EXPECT_EQ(goose->honkCount, 1);
}

TEST_F(BehaviorHonckerTest, HonkPublishesEvent) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    int eventCount = 0;
    auto sub = EventBus::Instance().Subscribe<GooseHonkedEvent>([&](const GooseHonkedEvent&) { ++eventCount; });

    Honcker_Honk(goose, 1.0);

    EXPECT_EQ(eventCount, 1);
    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorHonckerTest, HonkWithNullGoose) {
    Honcker_Honk(nullptr, 1.0);
}

TEST_F(BehaviorHonckerTest, RenderNull) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorHonckerTest, RenderNotVisible) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    MockHonkRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.ellipseCount, 0);
}

TEST_F(BehaviorHonckerTest, RenderHonk) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Honcker_Honk(goose, 1.0);

    goose->rig.neckHead = {100, 100};

    MockHonkRenderer renderer;
    b->render(goose, ctx, &renderer);
    // After honk.png exists and GetImageSize returns true for non-null images,
    // the image-draw path is taken (s_honkImage loaded by init)
    EXPECT_EQ(renderer.ellipseCount, 0);
    EXPECT_EQ(renderer.imageCount, 1);
}

TEST_F(BehaviorHonckerTest, TickNoKeyPress) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->tick(goose, ctx, 0.016, 0.0);

    EXPECT_EQ(goose->honkCount, 0);
}

TEST_F(BehaviorHonckerTest, TickKeyPress) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->tick(goose, ctx, 0.016, 0.0);

    PlatformInputMock_SetKeyState(honkerKeyCode(), true);

    int eventCount = 0;
    auto sub = EventBus::Instance().Subscribe<GooseHonkedEvent>([&](const GooseHonkedEvent&) { ++eventCount; });

    b->tick(goose, ctx, 0.016, 1.0);

    EXPECT_EQ(goose->honkCount, 1);
    EXPECT_EQ(eventCount, 1);

    auto* state = BehaviorStateManager::Instance().Get<HonckerState>(1, "honcker");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->visible);
    EXPECT_EQ(state->lastShowTime, 1.0);

    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorHonckerTest, TickKeyHold) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->tick(goose, ctx, 0.016, 0.0);

    PlatformInputMock_SetKeyState(honkerKeyCode(), true);

    b->tick(goose, ctx, 0.016, 1.0);
    EXPECT_EQ(goose->honkCount, 1);

    b->tick(goose, ctx, 0.016, 2.0);
    EXPECT_EQ(goose->honkCount, 1);
}

TEST_F(BehaviorHonckerTest, TickKeyRelease) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->tick(goose, ctx, 0.016, 0.0);

    PlatformInputMock_SetKeyState(honkerKeyCode(), true);
    b->tick(goose, ctx, 0.016, 1.0);
    EXPECT_EQ(goose->honkCount, 1);

    PlatformInputMock_SetKeyState(honkerKeyCode(), false);
    b->tick(goose, ctx, 0.016, 2.0);
    EXPECT_EQ(goose->honkCount, 1);

    PlatformInputMock_SetKeyState(honkerKeyCode(), true);
    b->tick(goose, ctx, 0.016, 3.0);
    EXPECT_EQ(goose->honkCount, 2);
}

TEST_F(BehaviorHonckerTest, TickAutoHide) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->tick(goose, ctx, 0.016, 0.0);

    PlatformInputMock_SetKeyState(honkerKeyCode(), true);
    b->tick(goose, ctx, 0.016, 1.0);

    auto* state = BehaviorStateManager::Instance().Get<HonckerState>(1, "honcker");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->visible);

    PlatformInputMock_SetKeyState(honkerKeyCode(), false);

    b->tick(goose, ctx, 0.016, 2.0);
    EXPECT_FALSE(state->visible);
}

TEST_F(BehaviorHonckerTest, HonkPublishesEventFromTick) {
    auto* b = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    b->tick(goose, ctx, 0.016, 0.0);

    PlatformInputMock_SetKeyState(honkerKeyCode(), true);

    int eventCount = 0;
    GooseHonkedEvent captured{};
    auto sub = EventBus::Instance().Subscribe<GooseHonkedEvent>([&](const GooseHonkedEvent& e) {
        ++eventCount;
        captured = e;
    });

    b->tick(goose, ctx, 0.016, 5.0);

    EXPECT_EQ(eventCount, 1);
    EXPECT_EQ(captured.gooseId, 1);

    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorHonckerTest, HonkerHonkWithNoCanHonk) {
    HonkSpyGoose silent(2);
    silent.m_canHonk = false;
    Honcker_Honk(&silent, 1.0);
    EXPECT_EQ(silent.honkCount, 0);
}
