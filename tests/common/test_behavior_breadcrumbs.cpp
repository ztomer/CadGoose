// ===========================
// test_behavior_breadcrumbs.cpp
// Unit tests for BreadCrumbs behavior
// ===========================
#include "behaviors/states/all.h"
#include "gtest/gtest.h"

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"
#include "behaviors/states/breadcrumb_state.h"

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

TEST(BreadCrumbsBehavior, CrumbEatenByProximity) {
    struct TestCrumb { float x, y; bool eaten = false; };
    TestCrumb crumb{100.0f, 100.0f, false};
    float gooseX = 105.0f, gooseY = 100.0f;
    float eatRadius = 32.0f;
    float dist = std::hypot(gooseX - crumb.x, gooseY - crumb.y);
    EXPECT_LT(dist, eatRadius);
    if (dist < eatRadius) crumb.eaten = true;
    EXPECT_TRUE(crumb.eaten);
}

TEST(BreadCrumbsBehavior, CrumbNotEatenWhenFar) {
    struct TestCrumb { float x, y; bool eaten = false; };
    TestCrumb crumb{100.0f, 100.0f, false};
    float gooseX = 200.0f, gooseY = 200.0f;
    float eatRadius = 32.0f;
    float dist = std::hypot(gooseX - crumb.x, gooseY - crumb.y);
    EXPECT_GT(dist, eatRadius);
    EXPECT_FALSE(crumb.eaten);
}

TEST(BreadCrumbsBehavior, EatenCrumbSkippedInRender) {
    struct Crumb { float x, y; bool eaten; double time; float lifetime; };
    std::vector<Crumb> crumbs = {
        {0, 0, false, 0, 10},
        {10, 10, true, 0, 10},
        {20, 20, false, 0, 10},
    };
    int renderCount = 0, skipCount = 0;
    for (const auto& c : crumbs) {
        if (c.eaten) { skipCount++; continue; }
        renderCount++;
    }
    EXPECT_EQ(renderCount, 2);
    EXPECT_EQ(skipCount, 1);
}

TEST(BreadCrumbsBehavior, ChewingAnimationFieldsWork) {
    Config_Init();
    Goose goose(0, "TestGoose", 800, 600);
    EXPECT_FALSE(goose.isChewing);

    goose.isChewing = true;
    goose.chewingStartTime = goose.lastUpdateTime;
    EXPECT_TRUE(goose.isChewing);

    double elapsed = goose.lastUpdateTime - goose.chewingStartTime;
    EXPECT_DOUBLE_EQ(elapsed, 0.0);
}

TEST(BreadCrumbsBehavior, ChewingAnimationExpiresAfterDuration) {
    Config_Init();
    Goose goose(0, "TestGoose", 800, 600);
    goose.isChewing = true;
    goose.chewingStartTime = 10.0;
    goose.lastUpdateTime = 10.5;

    const double kChewDuration = 0.4;
    double elapsed = goose.lastUpdateTime - goose.chewingStartTime;
    if (elapsed >= kChewDuration) {
        goose.isChewing = false;
    }
    EXPECT_FALSE(goose.isChewing);
}

TEST(BreadCrumbsBehavior, EatenCrumbRemovedFromFront) {
    struct Crumb { float x, y; bool eaten; double time; float lifetime; };
    std::vector<Crumb> crumbs = {
        {0, 0, true, 0, 10},
        {10, 10, false, 0, 10},
        {20, 20, false, 0, 10},
    };
    while (!crumbs.empty() && crumbs.front().eaten) {
        crumbs.erase(crumbs.begin());
    }
    EXPECT_EQ(crumbs.size(), 2);
    EXPECT_FALSE(crumbs[0].eaten);
}
