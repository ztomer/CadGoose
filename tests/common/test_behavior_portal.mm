#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "event_bus.h"
#include "hotkey.h"
#include "platform_input_mock.h"
#include "actor.h"
#include "behaviors/states/portal_state.h"

class BehaviorPortalTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("portal")) {
            BehaviorRegistry::Instance().Restore();
        }
        savedControlPortals = g_config.behaviors.control.portals;
        g_config.behaviors.control.portals = true;

        goose = new Goose(1, "goose", 1920, 1080);
        goose->behaviorsEnabled = true;
        goose->pos = {500, 500};
        goose->vel = {100, 100};
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
        resetPortalStatics();
        PlatformInputMock_Reset();
    }

    void TearDown() override {
        g_config.behaviors.control.portals = savedControlPortals;
        ActorManager::Instance().destroyAllOfType("portal");
        delete goose;
        PlatformInputMock_Reset();
    }

    int keyOne() const { return KeyNameToKeyCode(g_config.portal.hotkey1); }
    int keyTwo() const { return KeyNameToKeyCode(g_config.portal.hotkey2); }
    int keyZero() const { return KeyNameToKeyCode(g_config.portal.hotkey0); }

    Goose* goose;
    BehaviorContext ctx{};

    void resetPortalStatics() {
        extern void Portal_ResetForTest();
        Portal_ResetForTest();
    }

private:
    bool savedControlPortals;
};

TEST_F(BehaviorPortalTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PortalState>(1, "portal");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->portalA.active);
    EXPECT_FALSE(state->portalB.active);
}

TEST_F(BehaviorPortalTest, KeyOnePlacesPortalA) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(300, 400);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);

    auto* state = BehaviorStateManager::Instance().Get<PortalState>(1, "portal");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->portalA.active);
    EXPECT_FLOAT_EQ(state->portalA.x, 300);
    EXPECT_FLOAT_EQ(state->portalA.y, 400);

    bool actorFound = false;
    auto& mgr = ActorManager::Instance();
    for (int i = 0; i < mgr.totalCount(); i++) {
        Actor* a = mgr.getByIndex(i);
        if (a && strcmp(a->type(), "portal") == 0 && a->id() == 1) {
            actorFound = true;
            EXPECT_FLOAT_EQ(a->position().x, 300);
            EXPECT_FLOAT_EQ(a->position().y, 400);
        }
    }
    EXPECT_TRUE(actorFound);
}

TEST_F(BehaviorPortalTest, KeyTwoPlacesPortalB) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(700, 800);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);

    auto* state = BehaviorStateManager::Instance().Get<PortalState>(1, "portal");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->portalB.active);
    EXPECT_FLOAT_EQ(state->portalB.x, 700);
    EXPECT_FLOAT_EQ(state->portalB.y, 800);

    bool actorFound = false;
    auto& mgr = ActorManager::Instance();
    for (int i = 0; i < mgr.totalCount(); i++) {
        Actor* a = mgr.getByIndex(i);
        if (a && strcmp(a->type(), "portal") == 0 && a->id() == 2) {
            actorFound = true;
            EXPECT_FLOAT_EQ(a->position().x, 700);
            EXPECT_FLOAT_EQ(a->position().y, 800);
        }
    }
    EXPECT_TRUE(actorFound);
}

TEST_F(BehaviorPortalTest, GooseInPortalATeleportsToB) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);

    PlatformInputMock_SetMousePosition(300, 300);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);

    goose->pos = {100, 100};
    goose->vel = {50, 50};

    int teleportCount = 0;
    auto sub = EventBus::Instance().Subscribe<GooseTeleportedEvent>(
        [&](const GooseTeleportedEvent&) { ++teleportCount; });

    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    EXPECT_FLOAT_EQ(goose->pos.x, 300);
    EXPECT_FLOAT_EQ(goose->pos.y, 300);
    EXPECT_FLOAT_EQ(goose->vel.x, 0);
    EXPECT_FLOAT_EQ(goose->vel.y, 0);
    EXPECT_EQ(teleportCount, 1);

    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorPortalTest, GooseInPortalBTeleportsToA) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(150, 250);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);

    PlatformInputMock_SetMousePosition(400, 500);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);

    goose->pos = {400, 500};

    int teleportCount = 0;
    auto sub = EventBus::Instance().Subscribe<GooseTeleportedEvent>(
        [&](const GooseTeleportedEvent&) { ++teleportCount; });

    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    EXPECT_FLOAT_EQ(goose->pos.x, 150);
    EXPECT_FLOAT_EQ(goose->pos.y, 250);

    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorPortalTest, JustTeleportedGuard) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);

    PlatformInputMock_SetMousePosition(300, 300);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);

    goose->pos = {100, 100};

    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    EXPECT_FLOAT_EQ(goose->pos.x, 300);

    ctx.time = 3.016;
    b->tick(goose, ctx, 0.016, 3.016);

    EXPECT_FLOAT_EQ(goose->pos.x, 300);
}

TEST_F(BehaviorPortalTest, KeyZeroTogglesPortals) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);

    PlatformInputMock_SetMousePosition(300, 300);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);

    PlatformInputMock_SetKeyState(keyZero(), true);
    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);
    PlatformInputMock_SetKeyState(keyZero(), false);

    goose->pos = {100, 100};

    ctx.time = 4.0;
    b->tick(goose, ctx, 0.016, 4.0);

    EXPECT_NE(goose->pos.x, 300);
}

TEST_F(BehaviorPortalTest, GooseNotInPortalNoTeleport) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);

    PlatformInputMock_SetMousePosition(300, 300);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);

    goose->pos = {500, 500};

    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);

    EXPECT_FLOAT_EQ(goose->pos.x, 500);
}

TEST_F(BehaviorPortalTest, KeyOneWithoutMouseFallsBackToGoosePos) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(0, 0, false);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);

    auto* state = BehaviorStateManager::Instance().Get<PortalState>(1, "portal");
    ASSERT_NE(state, nullptr);
    EXPECT_FLOAT_EQ(state->portalA.x, 500);
    EXPECT_FLOAT_EQ(state->portalA.y, 500);
}

TEST_F(BehaviorPortalTest, KeyOneEdgeDetected) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);

    int actorCount = ActorManager::Instance().totalCount();

    PlatformInputMock_SetMousePosition(200, 200);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    EXPECT_EQ(ActorManager::Instance().totalCount(), actorCount);
}

TEST_F(BehaviorPortalTest, CleanupRemovesPortalActors) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);

    PlatformInputMock_SetMousePosition(300, 300);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);

    int portalCount = 0;
    for (int i = 0; i < ActorManager::Instance().totalCount(); i++) {
        Actor* a = ActorManager::Instance().getByIndex(i);
        if (a && strcmp(a->type(), "portal") == 0) portalCount++;
    }
    EXPECT_GT(portalCount, 0);

    auto& cleanup = b->cleanup;
    if (cleanup) cleanup(ctx);

    portalCount = 0;
    for (int i = 0; i < ActorManager::Instance().totalCount(); i++) {
        Actor* a = ActorManager::Instance().getByIndex(i);
        if (a && strcmp(a->type(), "portal") == 0) portalCount++;
    }
    EXPECT_EQ(portalCount, 0);
}

TEST_F(BehaviorPortalTest, ReplacesPortalAOnSecondKeyPress) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);
    ctx.time = 1.5;
    b->tick(goose, ctx, 0.016, 1.5);

    int actorCountAfterFirst = ActorManager::Instance().totalCount();

    PlatformInputMock_SetMousePosition(200, 200);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    EXPECT_EQ(ActorManager::Instance().totalCount(), actorCountAfterFirst);

    auto* state = BehaviorStateManager::Instance().Get<PortalState>(1, "portal");
    ASSERT_NE(state, nullptr);
    EXPECT_FLOAT_EQ(state->portalA.x, 200);
    EXPECT_FLOAT_EQ(state->portalA.y, 200);
}

TEST_F(BehaviorPortalTest, ReplacesPortalBOnSecondKeyPress) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(300, 400);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);
    ctx.time = 1.5;
    b->tick(goose, ctx, 0.016, 1.5);

    int actorCountAfterFirst = ActorManager::Instance().totalCount();

    PlatformInputMock_SetMousePosition(500, 600);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);

    EXPECT_EQ(ActorManager::Instance().totalCount(), actorCountAfterFirst);

    auto* state = BehaviorStateManager::Instance().Get<PortalState>(1, "portal");
    ASSERT_NE(state, nullptr);
    EXPECT_FLOAT_EQ(state->portalB.x, 500);
    EXPECT_FLOAT_EQ(state->portalB.y, 600);
}

TEST_F(BehaviorPortalTest, JustTeleportedResetsWhenOutsidePortal) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    PlatformInputMock_SetMousePosition(100, 100);
    PlatformInputMock_SetKeyState(keyOne(), true);
    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, 1.0);
    PlatformInputMock_SetKeyState(keyOne(), false);

    PlatformInputMock_SetMousePosition(300, 300);
    PlatformInputMock_SetKeyState(keyTwo(), true);
    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, 2.0);
    PlatformInputMock_SetKeyState(keyTwo(), false);

    goose->pos = {100, 100};
    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, 3.0);
    EXPECT_FLOAT_EQ(goose->pos.x, 300);

    goose->pos = {900, 900};
    ctx.time = 4.0;
    b->tick(goose, ctx, 0.016, 4.0);
    EXPECT_FLOAT_EQ(goose->pos.x, 900);

    auto* state = BehaviorStateManager::Instance().Get<PortalState>(1, "portal");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->justTeleported);
}

TEST_F(BehaviorPortalTest, RenderNoOp) {
    auto* b = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}
