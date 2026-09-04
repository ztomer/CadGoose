#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "cursor_io.h"
#include "event_bus.h"
#include "ring_buffer.h"
#include "hotkey.h"
#include "platform_input_mock.h"

struct MockCrumbCursorProvider : public ICursorProvider {
    CursorState cursor;
    CursorState Read() override { return cursor; }
    void Execute(const CursorAction&) override {}
    void set(float x, float y, bool hasPos = true) {
        cursor = {.position = {x, y}, .caps = hasPos ? CAP_GET_POS : CAP_NONE};
    }
};

class BehaviorBreadcrumbsTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("breadcrumbs")) {
            BehaviorRegistry::Instance().Restore();
        }
        PlatformInputMock_Reset();

        savedProvider = g_cursorProvider;
        g_cursorProvider = &mockCursor;
        mockCursor.set(0, 0); // default valid position so hasPos() returns true

        goose = new Goose(1, "goose", 1920, 1080);
        goose->behaviorsEnabled = true;
        goose->pos = {500, 500};
        ctx.goose = goose;
        ctx.time = 100;
        ctx.world = &g_world;
    }

    void TearDown() override {
        g_cursorProvider = savedProvider;
        ActorManager::Instance().destroyAllOfType(ActorType::Breadcrumb);
        delete goose;
        PlatformInputMock_Reset();
    }

    MockCrumbCursorProvider mockCursor;
    ICursorProvider* savedProvider = nullptr;

    void populateCrumbs(int count, double time, float lifetime = 10.0f) {
        for (int i = 0; i < count; i++) {
            Crumbs crumb;
            crumb.pos = {100.0f + i * 10.0f, 200.0f};
            crumb.time = time;
            crumb.lifetime = lifetime;
            crumb.eaten = false;
            g_world.crumbs.push(crumb);
        }
    }

    Goose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorBreadcrumbsTest, Init) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);

    Crumbs testCrumb;
    testCrumb.pos = {10, 20};
    testCrumb.time = 50;
    testCrumb.lifetime = 5;
    g_world.crumbs.push(testCrumb);

    b->init(ctx);

    EXPECT_TRUE(g_world.crumbs.empty());
}

TEST_F(BehaviorBreadcrumbsTest, TickThrottle) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {500, 500};

    b->tick(goose, ctx, 0.016, 100.0);

    goose->pos = {600, 600};

    b->tick(goose, ctx, 0.016, 100.001);

    EXPECT_TRUE(g_world.crumbs.empty());
}

TEST_F(BehaviorBreadcrumbsTest, RenderNoop) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorBreadcrumbsTest, GooseEatsNearbyCrumb) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Crumbs crumb;
    crumb.pos = {500, 500};
    crumb.time = 95;
    crumb.lifetime = 20.0f;
    crumb.eaten = false;
    g_world.crumbs.push(crumb);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_TRUE(g_world.crumbs.empty());
}

TEST_F(BehaviorBreadcrumbsTest, GooseDoesNotEatFarCrumb) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    Crumbs crumb;
    crumb.pos = {1000, 1000};
    crumb.time = 95;
    crumb.lifetime = 20.0f;
    crumb.eaten = false;
    g_world.crumbs.push(crumb);

    b->tick(goose, ctx, 0.016, 100.0);

    ASSERT_FALSE(g_world.crumbs.empty());
    EXPECT_FALSE(g_world.crumbs.front().eaten);
}

TEST_F(BehaviorBreadcrumbsTest, MaxCrumbsEnforced) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    size_t maxCrumbs = (size_t)g_config.behaviors.breadCrumbs.maxCrumbs;
    populateCrumbs((int)(maxCrumbs * 2), 95, 20.0f);

    b->tick(goose, ctx, 0.016, 200.0);

    EXPECT_LE(g_world.crumbs.size(), maxCrumbs);
}

TEST_F(BehaviorBreadcrumbsTest, ExpiredCrumbsCleaned) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    populateCrumbs(3, 50, 10.0f);

    b->tick(goose, ctx, 0.016, 200.0);

    EXPECT_TRUE(g_world.crumbs.empty());
}

TEST_F(BehaviorBreadcrumbsTest, EatenCrumbsPopped) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {500, 500};

    Crumbs crumb1;
    crumb1.pos = {500, 500};
    crumb1.time = 95;
    crumb1.lifetime = 20.0f;
    crumb1.eaten = false;
    g_world.crumbs.push(crumb1);

    Crumbs crumb2;
    crumb2.pos = {600, 600};
    crumb2.time = 95;
    crumb2.lifetime = 20.0f;
    crumb2.eaten = false;
    g_world.crumbs.push(crumb2);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(g_world.crumbs.size(), (size_t)1);
}

TEST_F(BehaviorBreadcrumbsTest, NoCursorProvider) {
    g_cursorProvider = nullptr;
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    int keyCode = KeyNameToKeyCode(g_config.behaviors.breadCrumbs.hotkey);
    PlatformInputMock_SetKeyState(keyCode, true);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_TRUE(g_world.crumbs.empty());
}

TEST_F(BehaviorBreadcrumbsTest, CursorNoPosition) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(0, 0, false);

    int keyCode = KeyNameToKeyCode(g_config.behaviors.breadCrumbs.hotkey);
    PlatformInputMock_SetKeyState(keyCode, true);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_TRUE(g_world.crumbs.empty());
}

TEST_F(BehaviorBreadcrumbsTest, KeyPressDropsFirstCrumb) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    int keyCode = KeyNameToKeyCode(g_config.behaviors.breadCrumbs.hotkey);
    PlatformInputMock_SetKeyState(keyCode, true);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_FALSE(g_world.crumbs.empty());
    EXPECT_FLOAT_EQ(g_world.crumbs.front().pos.x, 300);
    EXPECT_FLOAT_EQ(g_world.crumbs.front().pos.y, 400);
    EXPECT_FALSE(g_world.crumbs.front().eaten);
}

TEST_F(BehaviorBreadcrumbsTest, KeyHoldDragDropsAdditionalCrumbs) {
    auto* b = BehaviorRegistry::Instance().Get("breadcrumbs");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(100, 100);
    int keyCode = KeyNameToKeyCode(g_config.behaviors.breadCrumbs.hotkey);
    PlatformInputMock_SetKeyState(keyCode, true);

    b->tick(goose, ctx, 0.016, 100.0);
    ASSERT_EQ(g_world.crumbs.size(), (size_t)1);

    mockCursor.set(200, 100);
    b->tick(goose, ctx, 0.016, 101.0);

    EXPECT_EQ(g_world.crumbs.size(), (size_t)2);
    EXPECT_FLOAT_EQ(g_world.crumbs.back().pos.x, 200);
    EXPECT_FLOAT_EQ(g_world.crumbs.back().pos.y, 100);
}
