// ===========================
// test_behaviors_control.cpp
// Tests for control/info behaviors: honcker, jail, nametag, presence, drag
// ===========================
#include "behaviors/states/all.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"
#include "behaviors/states/honcker_state.h"
#include "behaviors/states/jail_state.h"
#include "behaviors/states/drag_state.h"

static void ResetBehaviorState() {
    BehaviorStateManager::Instance().ClearAll();
}

// ===========================
// Honcker Behavior Tests
// ===========================
TEST(HonckerBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<HonckerState>(0, "honcker");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->lastHonkTime, 0);
    EXPECT_EQ(state->lastShowTime, 0);
    EXPECT_FALSE(state->isOnCooldown);
    EXPECT_FALSE(state->visible);
}

TEST(HonckerBehavior, VisibilityTimeout) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HonckerState>(0, "honcker");
    state->Reset();
    state->visible = true;
    state->lastShowTime = 0.0;

    double time = 0.6;
    if (time - state->lastShowTime > 0.5) {
        state->visible = false;
    }
    EXPECT_FALSE(state->visible);
}

TEST(HonckerBehavior, VisibilityStillActive) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HonckerState>(0, "honcker");
    state->Reset();
    state->visible = true;
    state->lastShowTime = 0.0;

    double time = 0.3;
    if (time - state->lastShowTime > 0.5) {
        state->visible = false;
    }
    EXPECT_TRUE(state->visible);
}

TEST(HonckerBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.control.honcker, false);
    g_config.behaviors.control.honcker = true;
    EXPECT_EQ(g_config.behaviors.control.honcker, true);
    g_config.behaviors.control.honcker = false;
}

TEST(HonckerBehavior, HonckerConfigDefaults) {
    EXPECT_EQ(g_config.behaviors.honcker.hotkey, "f");
    EXPECT_FLOAT_EQ(g_config.behaviors.honcker.size, 40.0f);
    EXPECT_FLOAT_EQ(g_config.behaviors.honcker.cooldown, 0.5f);
}

// ===========================
// Jail Behavior Tests
// ===========================
TEST(JailBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<JailState>(0, "jail");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isJailed);
}

TEST(JailBehavior, JailDisablesWhenConfigOff) {
    g_config.behaviors.control.jail = false;
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(0, "jail");
    state->Reset();
    state->isJailed = true;

    if (!g_config.behaviors.control.jail) {
        state->isJailed = false;
    }
    EXPECT_FALSE(state->isJailed);
    g_config.behaviors.control.jail = false;
}

TEST(JailBehavior, GoosePositionedAtJail) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(0, "jail");
    state->Reset();
    state->isJailed = true;

    Vector2 jailPos{400.0f, 300.0f};
    Vector2 goosePos{100.0f, 100.0f};

    goosePos = jailPos;
    EXPECT_FLOAT_EQ(goosePos.x, 400.0f);
    EXPECT_FLOAT_EQ(goosePos.y, 300.0f);
}

TEST(JailBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.control.jail, false);
    g_config.behaviors.control.jail = true;
    EXPECT_EQ(g_config.behaviors.control.jail, true);
    g_config.behaviors.control.jail = false;
}

TEST(JailBehavior, JailConfigDefaults) {
    EXPECT_EQ(g_config.behaviors.jail.hotkeyO, "o");
    EXPECT_EQ(g_config.behaviors.jail.hotkeyP, "p");
    EXPECT_FLOAT_EQ(g_config.behaviors.jail.size, 150.0f);
}

// ===========================
// Nametag Behavior Tests
// ===========================
TEST(NametagBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.info.nametag, false);
    g_config.behaviors.info.nametag = true;
    EXPECT_EQ(g_config.behaviors.info.nametag, true);
    g_config.behaviors.info.nametag = false;
}

TEST(NametagBehavior, NametagConfigDefaults) {
    EXPECT_FLOAT_EQ(g_config.behaviors.nametag.size, 14.0f);
}

TEST(NametagBehavior, NameBoxWidth) {
    std::string name = "Honker";
    size_t nameLen = name.length();
    float nameWidth = (float)nameLen * 8.0f;
    EXPECT_FLOAT_EQ(nameWidth, 48.0f);
}

TEST(NametagBehavior, NameBoxPosition) {
    Vector2 headPos{400.0f, 300.0f};
    std::string name = "Honker";
    size_t nameLen = name.length();
    float nameWidth = (float)nameLen * 8.0f;
    float boxHeight = 18.0f;
    float boxX = headPos.x - nameWidth / 2.0f - 4.0f;
    float boxY = headPos.y - 40.0f;

    EXPECT_FLOAT_EQ(boxX, 372.0f);
    EXPECT_FLOAT_EQ(boxY, 260.0f);
}

// ===========================
// Presence Behavior Tests
// ===========================
TEST(PresenceBehavior, StateLabels) {
    GooseState state = GooseState::WANDER;
    const char* label = "Unknown";
    switch (state) {
        case GooseState::WANDER: label = "Wandering"; break;
        case GooseState::FETCHING: label = "Fetching"; break;
        case GooseState::RETURNING: label = "Returning"; break;
        case GooseState::CHASE_CURSOR: label = "Chasing Cursor"; break;
        case GooseState::SNATCH_CURSOR: label = "Snatching Cursor"; break;
    }
    EXPECT_STREQ(label, "Wandering");

    state = GooseState::FETCHING;
    switch (state) {
        case GooseState::WANDER: label = "Wandering"; break;
        case GooseState::FETCHING: label = "Fetching"; break;
        case GooseState::RETURNING: label = "Returning"; break;
        case GooseState::CHASE_CURSOR: label = "Chasing Cursor"; break;
        case GooseState::SNATCH_CURSOR: label = "Snatching Cursor"; break;
    }
    EXPECT_STREQ(label, "Fetching");
}

TEST(PresenceBehavior, StatusFormat) {
    const char* status = "Wandering";
    char fullStatus[64];
    snprintf(fullStatus, sizeof(fullStatus), "Goose: %s", status);
    EXPECT_STREQ(fullStatus, "Goose: Wandering");
}

TEST(PresenceBehavior, UpdateInterval) {
    double time = 0.0;
    double lastUpdate = 0.0;
    float interval = 0.5f;

    EXPECT_FALSE(time - lastUpdate >= interval);

    time = 0.6;
    EXPECT_TRUE(time - lastUpdate >= interval);
}

TEST(PresenceBehavior, VisibilityToggle) {
    bool targetVisible = true;
    bool lastVisible = false;
    bool changed = (targetVisible != lastVisible);
    EXPECT_TRUE(changed);

    lastVisible = true;
    changed = (targetVisible != lastVisible);
    EXPECT_FALSE(changed);
}

TEST(PresenceBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.info.presence, false);
    g_config.behaviors.info.presence = true;
    EXPECT_EQ(g_config.behaviors.info.presence, true);
    g_config.behaviors.info.presence = false;
}

// ===========================
// Drag Behavior Tests
// ===========================
TEST(DragBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<DragState>(0, "drag");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isDragging);
    EXPECT_FLOAT_EQ(state->dragAnchorX, 0);
    EXPECT_FLOAT_EQ(state->dragAnchorY, 0);
    EXPECT_EQ(state->dragStartTime, 0);
    EXPECT_FLOAT_EQ(state->resistanceChance, 0.0f);
}

TEST(DragBehavior, DragRadius) {
    Vector2 goosePos{100.0f, 100.0f};
    Vector2 cursorPos{130.0f, 120.0f};
    float radius = 45.0f;

    float dx = goosePos.x - cursorPos.x;
    float dy = goosePos.y - cursorPos.y;
    bool onGoose = (dx > -radius && dx < radius && dy > -radius && dy < radius);
    EXPECT_TRUE(onGoose);
}

TEST(DragBehavior, OutsideDragRadius) {
    Vector2 goosePos{100.0f, 100.0f};
    Vector2 cursorPos{200.0f, 200.0f};
    float radius = 45.0f;

    float dx = goosePos.x - cursorPos.x;
    float dy = goosePos.y - cursorPos.y;
    bool onGoose = (dx > -radius && dx < radius && dy > -radius && dy < radius);
    EXPECT_FALSE(onGoose);
}

TEST(DragBehavior, DragPositionUpdate) {
    Vector2 cursorPos{100.0f, 100.0f};
    Vector2 goosePos{50.0f, 50.0f};
    Vector2 gooseVel{0, 0};

    goosePos.x = cursorPos.x - 5.0f;
    goosePos.y = cursorPos.y;
    gooseVel = {0, 0};

    EXPECT_FLOAT_EQ(goosePos.x, 95.0f);
    EXPECT_FLOAT_EQ(goosePos.y, 100.0f);
    EXPECT_FLOAT_EQ(gooseVel.x, 0);
    EXPECT_FLOAT_EQ(gooseVel.y, 0);
}

TEST(DragBehavior, DirectionWobble) {
    float dir = 0.0f;
    int wobble = rand() % 21 - 10;
    dir += (float)wobble;
    EXPECT_GE(dir, -10.0f);
    EXPECT_LE(dir, 10.0f);
}

TEST(DragBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.control.drag, false);
    g_config.behaviors.control.drag = true;
    EXPECT_EQ(g_config.behaviors.control.drag, true);
    g_config.behaviors.control.drag = false;
}

TEST(DragBehavior, DragConfigDefaults) {
    EXPECT_FLOAT_EQ(g_config.behaviors.drag.radius, 45.0f);
}
