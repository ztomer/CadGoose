#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "cursor_io.h"
#include "event_bus.h"
#include "hotkey.h"
#include "platform_input_mock.h"
#include "actor.h"
#include "behaviors/states/jail_state.h"

struct JailMockCursorProvider : public ICursorProvider {
    CursorState cursor;
    CursorState Read() override { return cursor; }
    void Execute(const CursorAction&) override {}
    void set(float x, float y, bool hasPos = true) {
        cursor = {.position = {x, y}, .caps = hasPos ? CAP_GET_POS : CAP_NONE};
    }
};

class BehaviorJailTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("jail")) {
            BehaviorRegistry::Instance().Restore();
        }
        savedControlJail = g_config.behaviors.control.jail;
        g_config.behaviors.control.jail = true;

        savedProvider = g_cursorProvider;
        g_cursorProvider = &mockCursor;

        goose = new Goose(1, "goose", 1920, 1080);
        goose->behaviorsEnabled = true;
        goose->pos = {500, 500};
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
        resetJailStatics();
        PlatformInputMock_Reset();
        PlatformInputMock_SetMousePosition(300, 400);
    }

    void TearDown() override {
        g_config.behaviors.control.jail = savedControlJail;
        g_cursorProvider = savedProvider;
        ActorManager::Instance().destroyAllOfType("jail");
        delete goose;
        PlatformInputMock_Reset();
    }

    int oKeyCode() const { return KeyNameToKeyCode(g_config.behaviors.jail.hotkeyO); }
    int pKeyCode() const { return KeyNameToKeyCode(g_config.behaviors.jail.hotkeyP); }

    Goose* goose;
    BehaviorContext ctx{};
    JailMockCursorProvider mockCursor;
    ICursorProvider* savedProvider = nullptr;

    void resetJailStatics() {
        // Declared in behavior_jail.cpp
        extern void Jail_ResetForTest();
        Jail_ResetForTest();
    }

private:
    bool savedControlJail;
};

TEST_F(BehaviorJailTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<JailState>(1, "jail");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isJailed);
}

TEST_F(BehaviorJailTest, DisabledClearsState) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    g_config.behaviors.control.jail = false;

    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);

    auto* state = BehaviorStateManager::Instance().Get<JailState>(1, "jail");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isJailed);
}

TEST_F(BehaviorJailTest, OKeyPlacesJailAtCursor) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);

    bool jailFound = false;
    auto& mgr = ActorManager::Instance();
    for (int i = 0; i < mgr.totalCount(); i++) {
        Actor* a = mgr.getByIndex(i);
        if (a && a->actorType() == ActorType::Jail) {
            jailFound = true;
            EXPECT_FLOAT_EQ(a->position().x, 300);
            EXPECT_FLOAT_EQ(a->position().y, 400);
        }
    }
    EXPECT_TRUE(jailFound);
}

TEST_F(BehaviorJailTest, OKeyPressEdgeDetected) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);

    int countAfterFirst = ActorManager::Instance().totalCount();

    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    EXPECT_EQ(ActorManager::Instance().totalCount(), countAfterFirst);
}

TEST_F(BehaviorJailTest, PKeyTogglesJailActive) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    auto* state = BehaviorStateManager::Instance().Get<JailState>(1, "jail");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isJailed);
}

TEST_F(BehaviorJailTest, PKeyToggleOff) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(pKeyCode(), false);

    ctx.time = 2.5;
    b->tick(goose, ctx, 0.016, 2.5);

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    auto* state = BehaviorStateManager::Instance().Get<JailState>(1, "jail");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isJailed);
}

TEST_F(BehaviorJailTest, GooseJailedPosSnappedToNearestJail) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);

    goose->pos = {1000, 1000};

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    EXPECT_FLOAT_EQ(goose->pos.x, 300);
    EXPECT_FLOAT_EQ(goose->pos.y, 400);
    EXPECT_FLOAT_EQ(goose->target.x, 300);
    EXPECT_FLOAT_EQ(goose->target.y, 400);
}

TEST_F(BehaviorJailTest, GooseJailedEventPublished) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);

    int jailedCount = 0;
    auto sub = EventBus::Instance().Subscribe<GooseJailedEvent>(
        [&](const GooseJailedEvent&) { ++jailedCount; });

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    EXPECT_EQ(jailedCount, 1);

    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorJailTest, GooseFreedEventPublished) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(pKeyCode(), false);

    ctx.time = 2.5;
    b->tick(goose, ctx, 0.016, 2.5);

    int freedCount = 0;
    auto sub = EventBus::Instance().Subscribe<GooseFreedEvent>(
        [&](const GooseFreedEvent&) { ++freedCount; });

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    EXPECT_EQ(freedCount, 1);

    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorJailTest, RenderNoOp) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorJailTest, DisabledClearsJailActors) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    EXPECT_GT(ActorManager::Instance().countByType("jail"), 0);

    g_config.behaviors.control.jail = false;
    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    EXPECT_EQ(ActorManager::Instance().countByType("jail"), 0);
}

TEST_F(BehaviorJailTest, OKeyWhileActiveClearsJails) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(300, 400);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);
    PlatformInputMock_SetKeyState(pKeyCode(), false);

    auto* state = BehaviorStateManager::Instance().Get<JailState>(1, "jail");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isJailed);

    mockCursor.set(600, 700);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 4.0;
    b->tick(goose, ctx, 0.016, 4.0);

    EXPECT_FALSE(state->isJailed);
    EXPECT_GT(ActorManager::Instance().countByType("jail"), 0);
}

TEST_F(BehaviorJailTest, MultipleJailsNearestUsed) {
    auto* b = BehaviorRegistry::Instance().Get("jail");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(100, 100);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);

    ctx.time = 1.5;
    b->tick(goose, ctx, 0.016, 1.5);

    mockCursor.set(900, 900);
    PlatformInputMock_SetKeyState(oKeyCode(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(oKeyCode(), false);

    goose->pos = {800, 800};

    PlatformInputMock_SetKeyState(pKeyCode(), true);
    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    EXPECT_FLOAT_EQ(goose->pos.x, 900);
    EXPECT_FLOAT_EQ(goose->pos.y, 900);
}
