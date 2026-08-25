#include <gtest/gtest.h>
#include <cmath>
#include <algorithm>
#include <limits>

#include "goose_math.h"
#include "goose.h"
#include "world.h"
#include "actor.h"
#include "config.h"
#include "ai_text_meme.h"
#include "actor.h"
#include "actor_dropped_item.h"

#include "assets.h"
#include "cursor_io.h"

// --- world.cpp coverage ---

TEST(WorldUtil, GetGooseByIdFound) {
    Goose* g = new Goose(401, "FindMe", 1920, 1080);
    ActorManager::Instance().add(g);

    Goose* found = GetGooseById(401);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, 401);

    Goose* notFound = GetGooseById(999);
    EXPECT_EQ(notFound, nullptr);

    ActorManager::Instance().destroyAllOfType(ActorType::Goose);
}

// --- goose_forces.cpp coverage (multi-monitor paths) ---

TEST(GooseForceDetail, EdgeAvoidanceWithMultiMonitor) {
    bool origMulti = g_config.cursor.multiMonitorEnabled;
    g_config.cursor.multiMonitorEnabled = false;
    MonitorInfo savedMon = {};
    if (!g_world.monitors.empty()) savedMon = g_world.monitors.front();
    g_world.monitors.clear();
    g_world.monitors.push_back({0, 0, 1920, 1080, nullptr});

    Goose g(410, "EdgeMon", 1920, 1080);
    g.pos = {10, 10};
    g.target = {500, 500};
    g.vel = {100, 100};
    g.state = GooseState::WANDER;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.016, 0.0, 1920, 1080, CursorState{});
    EXPECT_GT(g.pos.x, 0.0f);

    g_world.monitors.clear();
    if (!g_world.monitors.empty() || savedMon.width > 0) {
        g_world.monitors.push_back(savedMon);
    }
    g_config.cursor.multiMonitorEnabled = origMulti;
}

TEST(GooseForceDetail, ClampToScreenWithMultiMonitor) {
    bool origMulti = g_config.cursor.multiMonitorEnabled;
    g_config.cursor.multiMonitorEnabled = false;
    MonitorInfo savedMon = {};
    if (!g_world.monitors.empty()) savedMon = g_world.monitors.front();
    g_world.monitors.clear();
    g_world.monitors.push_back({0, 0, 1920, 1080, nullptr});

    Goose g(411, "ClampMon", 1920, 1080);
    g.pos = {500, 500};
    g.target = {500, 500};
    g.state = GooseState::FETCHING;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.016, 0.0, 1920, 1080, CursorState{});

    g_world.monitors.clear();
    if (!g_world.monitors.empty() || savedMon.width > 0) {
        g_world.monitors.push_back(savedMon);
    }
    g_config.cursor.multiMonitorEnabled = origMulti;
}

// --- goose_behaviors_wander.cpp coverage ---

TEST(GooseBehaviorDetail, ChaseCursor_AnotherGooseGrabbing) {
    int origGrabber = g_world.cursorGrabberId;
    g_world.cursorGrabberId = 999;

    Goose g(420, "ChaseOther", 1920, 1080);
    g.state = GooseState::CHASE_CURSOR;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.chaseStartTime = 0.0;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {600, 500};

    g.Update(0.1, 0.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);
    g_world.cursorGrabberId = origGrabber;
}

TEST(GooseBehaviorDetail, ChaseCursor_NoCursorPos) {
    int origGrabber = g_world.cursorGrabberId;
    g_world.cursorGrabberId = -1;

    Goose g(421, "ChaseNoPos", 1920, 1080);
    g.state = GooseState::CHASE_CURSOR;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.chaseStartTime = 0.0;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    CursorState c;
    c.caps = 0; // no position capability

    g.Update(0.1, 0.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);
    g_world.cursorGrabberId = origGrabber;
}

TEST(GooseBehaviorDetail, ChaseCursor_Timeout) {
    int origGrabber = g_world.cursorGrabberId;
    g_world.cursorGrabberId = -1;

    Goose g(422, "ChaseTO", 1920, 1080);
    g.state = GooseState::CHASE_CURSOR;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.chaseStartTime = 0.0;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {600, 500};

    g.Update(0.1, 100.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);
    g_world.cursorGrabberId = origGrabber;
}

TEST(GooseBehaviorDetail, Wander_TextOnlyFetch) {
    bool origMemes = g_config.general.memesEnabled;
    g_config.general.memesEnabled = false;

    Goose g(423, "WanderText", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;
    g.memeFetchBias = 100;
    g.noteFetchBias = 100;
    g.lastDropTime = -1000.0;

    CursorState c;
    g.Update(0.016, 0.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::FETCHING);
    EXPECT_EQ(g.forceItemFetch, FetchType::Text); // text-only

    g_config.general.memesEnabled = origMemes;
}

// --- goose_behaviors_fetch.cpp coverage ---

TEST(GooseBehaviorDetail, HandleFetchingClearsExistingHeldItem) {
    Goose g(430, "FetchClean", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.forceItemFetch = FetchType::TestImage;
    g.heldItem = g_assets.GetRandomMeme(1920, 1080, 0.1f);
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.1, 0.0, 1920, 1080, CursorState{});
    EXPECT_EQ(g.state, GooseState::RETURNING);
    EXPECT_NE(g.heldItem, nullptr);
}

TEST(GooseBehaviorDetail, HandleFetchingTestImage) {
    Goose g(431, "TestImg", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.forceItemFetch = FetchType::TestImage;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.1, 0.0, 1920, 1080, CursorState{});
    EXPECT_EQ(g.state, GooseState::RETURNING);
    EXPECT_NE(g.heldItem, nullptr);
}

TEST(GooseBehaviorDetail, HandleReturningDiscardsNonFiniteItem) {
    Goose g(432, "Discard", 1920, 1080);
    g.state = GooseState::RETURNING;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.heldItem = g_assets.GetRandomMeme(1920, 1080, 0.1f);
    g.dragRot = std::numeric_limits<float>::quiet_NaN();
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.1, 0.0, 1920, 1080, CursorState{});
    EXPECT_EQ(g.heldItem, nullptr);
    EXPECT_EQ(g.state, GooseState::WANDER);
}

TEST(GooseBehaviorDetail, HandleFetchingForcedText) {
    Goose g(440, "FetchForced", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.forcedText = "forced text item";
    g.forceItemFetch = FetchType::Meme;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.1, 0.0, 1920, 1080, CursorState{});
    EXPECT_EQ(g.state, GooseState::RETURNING);
    ASSERT_NE(g.heldItem, nullptr);
    EXPECT_TRUE(g.forcedText.empty());
    EXPECT_EQ(g.forceItemFetch, FetchType::Random);
}

TEST(GooseBehaviorDetail, HandleFetchingAiTextFromQueue) {
    AI_TextMeme_Reset();
    AI_TextMeme_Inject("AI generated text");

    Goose g(441, "FetchAI", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.forceItemFetch = FetchType::Text;
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.1, 0.0, 1920, 1080, CursorState{});
    EXPECT_EQ(g.state, GooseState::RETURNING);
    ASSERT_NE(g.heldItem, nullptr);
    EXPECT_EQ(g.forceItemFetch, FetchType::Random);

    AI_TextMeme_Reset();
}

TEST(GooseBehaviorDetail, HandleFetchingFallbackFetch) {
    Goose g(442, "FetchFallback", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.forceItemFetch = static_cast<FetchType>(99);
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.1, 0.0, 1920, 1080, CursorState{});
    EXPECT_EQ(g.state, GooseState::RETURNING);
    ASSERT_NE(g.heldItem, nullptr);
    EXPECT_EQ(g.forceItemFetch, FetchType::Random);
}

TEST(GooseBehaviorDetail, HandleReturningDropsToyItem) {
    ActorManager::Instance().destroyAllOfType(ActorType::Goose);

    Goose g(443, "DropToy", 1920, 1080);
    g.state = GooseState::RETURNING;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.heldItem = g_assets.CreateToyItem(false);
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.Update(0.1, 0.0, 1920, 1080, CursorState{});
    EXPECT_EQ(g.state, GooseState::WANDER);
    EXPECT_EQ(g.heldItem, nullptr);
}

TEST(GooseBehaviorDetail, WanderHonkTrigger) {
    Goose g(450, "WanderHonk", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    float origDivisor = g_config.honk.wanderHonkDivisor;
    g_config.honk.wanderHonkDivisor = 1; // RandRange(1) === 0, guaranteed honk

    g.lastDropTime = 0.0;
    g.randomOffset = 1000.0; // prevent fetch (cooldown won't have elapsed)

    CursorState c;
    g.Update(0.016, 0.1, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);

    g_config.honk.wanderHonkDivisor = origDivisor;
}

TEST(GooseBehaviorDetail, WanderHeistPath) {
    bool origMemes = g_config.general.memesEnabled;
    g_config.general.memesEnabled = true;

    ItemData* data = g_assets.GetRandomMeme(1920, 1080, 0.1f);
    ASSERT_NE(data, nullptr);

    DroppedItem drop;
    drop.data = data;
    drop.pos = {100, 100};
    drop.rotation = 0.0f;
    drop.timeDropped = 0;
    drop.pinned = false;
    new DroppedItemActor(drop);

    float origHeistPct = g_config.item.heistChancePercent;
    g_config.item.heistChancePercent = 100;

    Goose g(451, "WanderHeist", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;
    g.memeFetchBias = 0;
    g.noteFetchBias = 0;
    g.lastDropTime = 0.0;
    g.randomOffset = 1000.0;

    CursorState c;
    g.Update(0.016, 0.1, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);

    ActorManager::Instance().removeAllDroppedItems();
    g_config.item.heistChancePercent = origHeistPct;
    g_config.general.memesEnabled = origMemes;
}
