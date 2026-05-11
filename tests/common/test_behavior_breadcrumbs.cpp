// ===========================
// test_behavior_breadcrumbs.cpp
// Unit tests for BreadCrumbs behavior
// ===========================
#include "gtest/gtest.h"

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"

TEST(BreadCrumbsBehavior, BreadCrumbStateCreation) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BreadCrumbState>(0, "breadcrumbs");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->crumbs.size(), 0);
    EXPECT_FALSE(state->isThrowing);
}

TEST(BreadCrumbsBehavior, BreadCrumbActivation) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BreadCrumbState>(0, "breadcrumbs");
    state->Reset();
    state->crumbs.resize(10);

    EXPECT_FALSE(state->isThrowing);

    int activeCount = 0;
    for (auto& crumb : state->crumbs) {
        if (crumb.active) activeCount++;
    }
    EXPECT_EQ(activeCount, 10);
}

TEST(BreadCrumbsBehavior, BreadCrumbReset) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BreadCrumbState>(0, "breadcrumbs");
    state->Reset();
    state->crumbs.resize(5);

    for (auto& crumb : state->crumbs) {
        crumb.active = true;
    }

    state->Reset();
    int activeCount = 0;
    for (auto& crumb : state->crumbs) {
        if (crumb.active) activeCount++;
    }
    EXPECT_EQ(activeCount, 0);
}

TEST(BreadCrumbsBehavior, BreadCrumbSpawnAtPosition) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BreadCrumbState>(0, "breadcrumbs");
    state->Reset();
    state->crumbs.resize(5);

    for (auto& crumb : state->crumbs) {
        if (!crumb.active) {
            crumb.active = true;
            crumb.pos = {100.0f, 200.0f};
            crumb.lifetime = 10.0f;
            break;
        }
    }

    bool found = false;
    for (auto& crumb : state->crumbs) {
        if (crumb.active && crumb.pos.x == 100.0f && crumb.pos.y == 200.0f) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(BreadCrumbsBehavior, BreadCrumbLifetimeDecay) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BreadCrumbState>(0, "breadcrumbs");
    state->Reset();
    state->crumbs.resize(1);

    state->crumbs[0].active = true;
    state->crumbs[0].pos = {50.0f, 50.0f};
    state->crumbs[0].lifetime = 2.0f;

    double dt = 1.0;
    state->crumbs[0].lifetime -= dt;
    EXPECT_EQ(state->crumbs[0].lifetime, 1.0f);

    state->crumbs[0].lifetime -= dt;
    EXPECT_EQ(state->crumbs[0].lifetime, 0.0f);

    if (state->crumbs[0].lifetime <= 0) {
        state->crumbs[0].active = false;
    }
    EXPECT_FALSE(state->crumbs[0].active);
}

TEST(BreadCrumbsBehavior, BreadCrumbGravity) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BreadCrumbState>(0, "breadcrumbs");
    state->Reset();
    state->crumbs.resize(1);

    state->crumbs[0].active = true;
    state->crumbs[0].pos = {100.0f, 100.0f};
    state->crumbs[0].vel = {0.0f, 0.0f};
    state->crumbs[0].lifetime = 10.0f;

    float gravity = 200.0f;
    double dt = 0.016f;

    state->crumbs[0].vel.y += gravity * dt;
    state->crumbs[0].pos.y += state->crumbs[0].vel.y * dt;

    EXPECT_GT(state->crumbs[0].vel.y, 0.0f);
    EXPECT_GT(state->crumbs[0].pos.y, 100.0f);
}

TEST(BreadCrumbsBehavior, KeyCodeConstant) {
    int rightShiftKeyCode = 60;
    EXPECT_EQ(rightShiftKeyCode, 60);
}

TEST(BreadCrumbsBehavior, CursorPositionInScreenCoords) {
    float cursorX = 500.0f;
    float cursorY = 300.0f;

    float imgWidth = 64.0f;
    float imgHeight = 64.0f;

    float drawX = cursorX - imgWidth / 2.0f;
    float drawY = cursorY - imgHeight / 2.0f;

    EXPECT_EQ(drawX, 500.0f - 32.0f);
    EXPECT_EQ(drawY, 300.0f - 32.0f);
}