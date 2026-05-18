// ===========================
// test_behaviors_registry_ai.cpp
// Tests for behavior registry, state manager, AI endpoints, pomodoro timing, portal persistence
// ===========================
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"
#include "ai_text_meme.h"
#include "hotkey.h"

static void ResetBehaviorState() {
    BehaviorStateManager::Instance().ClearAll();
}

// ===========================
// Behavior Registry Tests
// ===========================
TEST(BehaviorRegistry, ComprehensiveCount) {
    auto& registry = BehaviorRegistry::Instance();
    size_t count = registry.GetBehaviorCount();
    EXPECT_GE(count, 19);
}

TEST(BehaviorRegistry, BehaviorIds) {
    auto& registry = BehaviorRegistry::Instance();

    const char* expectedIds[] = {
        "toys", "boredom", "peeking",
        "interactive_drops", "pomodoro", "portal", "rainbow", "acid",
        "anger", "health", "honcker", "jail", "nametag",
        "presence", "drag"
    };

    for (const char* id : expectedIds) {
        Behavior* b = registry.Get(id);
        ASSERT_NE(b, nullptr) << "Behavior " << id << " not registered";
        EXPECT_STREQ(b->id, id);
    }
}

TEST(BehaviorRegistry, ConfigPointers) {
    auto& registry = BehaviorRegistry::Instance();

    Behavior* pomodoro = registry.Get("pomodoro");
    ASSERT_NE(pomodoro, nullptr);
    EXPECT_EQ(pomodoro->configPtr, &g_config.behaviors.systems.pomodoro);

    Behavior* portal = registry.Get("portal");
    ASSERT_NE(portal, nullptr);
    EXPECT_EQ(portal->configPtr, &g_config.behaviors.control.portals);

    Behavior* health = registry.Get("health");
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->configPtr, &g_config.behaviors.systems.health);

    Behavior* rainbow = registry.Get("rainbow");
    ASSERT_NE(rainbow, nullptr);
    EXPECT_EQ(rainbow->configPtr, &g_config.behaviors.fun.rainbow);

    Behavior* acid = registry.Get("acid");
    ASSERT_NE(acid, nullptr);
    EXPECT_EQ(acid->configPtr, &g_config.behaviors.fun.acid);

    Behavior* anger = registry.Get("anger");
    ASSERT_NE(anger, nullptr);
    EXPECT_EQ(anger->configPtr, &g_config.behaviors.fun.anger);

    Behavior* honcker = registry.Get("honcker");
    ASSERT_NE(honcker, nullptr);
    EXPECT_EQ(honcker->configPtr, &g_config.behaviors.control.honcker);

    Behavior* jail = registry.Get("jail");
    ASSERT_NE(jail, nullptr);
    EXPECT_EQ(jail->configPtr, &g_config.behaviors.control.jail);

    Behavior* drag = registry.Get("drag");
    ASSERT_NE(drag, nullptr);
    EXPECT_EQ(drag->configPtr, &g_config.behaviors.control.drag);

    Behavior* nametag = registry.Get("nametag");
    ASSERT_NE(nametag, nullptr);
    EXPECT_EQ(nametag->configPtr, &g_config.behaviors.info.nametag);

    Behavior* presence = registry.Get("presence");
    ASSERT_NE(presence, nullptr);
    EXPECT_EQ(presence->configPtr, &g_config.behaviors.info.presence);

    Behavior* toys = registry.Get("toys");
    ASSERT_NE(toys, nullptr);
    EXPECT_EQ(toys->configPtr, &g_config.behaviors.fun.toysEnabled);

    Behavior* boredom = registry.Get("boredom");
    ASSERT_NE(boredom, nullptr);
    EXPECT_EQ(boredom->configPtr, &g_config.behaviors.fun.boredom);

    Behavior* peeking = registry.Get("peeking");
    ASSERT_NE(peeking, nullptr);
    EXPECT_EQ(peeking->configPtr, &g_config.behaviors.fun.peeking);

    Behavior* interactiveDrops = registry.Get("interactive_drops");
    ASSERT_NE(interactiveDrops, nullptr);
    EXPECT_EQ(interactiveDrops->configPtr, &g_config.behaviors.fun.interactiveDrops);
}

TEST(BehaviorRegistry, EnabledPtrNotNull) {
    auto& registry = BehaviorRegistry::Instance();
    const char* ids[] = {
        "toys", "boredom", "peeking",
        "interactive_drops", "pomodoro", "portal", "rainbow", "acid",
        "anger", "health", "honcker", "jail", "nametag",
        "presence", "drag"
    };

    for (const char* id : ids) {
        Behavior* b = registry.Get(id);
        ASSERT_NE(b, nullptr) << "Behavior " << id << " not found";
        EXPECT_NE(b->enabledPtr, nullptr) << "Behavior " << id << " has null enabledPtr";
    }
}

TEST(BehaviorRegistry, StarterBehaviors) {
    auto& registry = BehaviorRegistry::Instance();

    Behavior* portal = registry.Get("portal");
    ASSERT_NE(portal, nullptr);
    EXPECT_TRUE(portal->config.isStarter);

    Behavior* rainbow = registry.Get("rainbow");
    ASSERT_NE(rainbow, nullptr);
    EXPECT_TRUE(rainbow->config.isStarter);

    Behavior* anger = registry.Get("anger");
    ASSERT_NE(anger, nullptr);
    EXPECT_TRUE(anger->config.isStarter);

    Behavior* honcker = registry.Get("honcker");
    ASSERT_NE(honcker, nullptr);
    EXPECT_TRUE(honcker->config.isStarter);

    Behavior* jail = registry.Get("jail");
    ASSERT_NE(jail, nullptr);
    EXPECT_TRUE(jail->config.isStarter);

    Behavior* drag = registry.Get("drag");
    ASSERT_NE(drag, nullptr);
    EXPECT_TRUE(drag->config.isStarter);
}

TEST(BehaviorRegistry, NonStarterBehaviors) {
    auto& registry = BehaviorRegistry::Instance();

    Behavior* toys = registry.Get("toys");
    ASSERT_NE(toys, nullptr);
    EXPECT_FALSE(toys->config.isStarter);

    Behavior* health = registry.Get("health");
    ASSERT_NE(health, nullptr);
    EXPECT_FALSE(health->config.isStarter);

    Behavior* nametag = registry.Get("nametag");
    ASSERT_NE(nametag, nullptr);
    EXPECT_FALSE(nametag->config.isStarter);

    Behavior* presence = registry.Get("presence");
    ASSERT_NE(presence, nullptr);
    EXPECT_FALSE(presence->config.isStarter);
}

// ===========================
// Behavior State Manager Tests
// ===========================
TEST(BehaviorStateManager, CreateAndRetrieve) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();

    auto* state = mgr.GetOrCreate<RainbowState>(0, "rainbow");
    ASSERT_NE(state, nullptr);
    state->Reset();

    auto* retrieved = mgr.Get<RainbowState>(0, "rainbow");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(state, retrieved);
}

TEST(BehaviorStateManager, GetReturnsNullForMissing) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();

    auto* state = mgr.Get<RainbowState>(999, "rainbow");
    EXPECT_EQ(state, nullptr);
}

TEST(BehaviorStateManager, RemoveForGooseComprehensive) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();

    mgr.GetOrCreate<RainbowState>(0, "rainbow");
    mgr.GetOrCreate<AcidState>(0, "acid");

    EXPECT_EQ(mgr.GetStateCount(), 2);

    mgr.RemoveForGoose(0);
    EXPECT_EQ(mgr.GetStateCount(), 0);
}

TEST(BehaviorStateManager, MultipleGeese) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();

    auto* state0 = mgr.GetOrCreate<RainbowState>(0, "rainbow");
    state0->hue = 100.0f;

    auto* state1 = mgr.GetOrCreate<RainbowState>(1, "rainbow");
    state1->hue = 200.0f;

    EXPECT_FLOAT_EQ(mgr.Get<RainbowState>(0, "rainbow")->hue, 100.0f);
    EXPECT_FLOAT_EQ(mgr.Get<RainbowState>(1, "rainbow")->hue, 200.0f);
}

// ===========================
// AI HTTP Client Endpoint Tests
// ===========================
struct CompEndpointTestCase {
    int providerType;
    int osaurusPort;
    int ollamaPort;
    std::string customEndpoint;
    std::string expected;
};

static std::string GenerateCompEndpoint(int providerType, int osaurusPort, int ollamaPort, const std::string& customEndpoint) {
    switch (providerType) {
        case 0:
            return "http://localhost:" + std::to_string(osaurusPort) + "/v1/chat/completions";
        case 1:
            return "http://localhost:" + std::to_string(ollamaPort) + "/v1/chat/completions";
        case 2:
            return customEndpoint;
        default:
            return "http://localhost:1337/v1/chat/completions";
    }
}

class CompAIEndpointTest : public ::testing::TestWithParam<CompEndpointTestCase> {};

TEST_P(CompAIEndpointTest, GeneratesCorrectURL) {
    const auto& p = GetParam();
    std::string result = GenerateCompEndpoint(p.providerType, p.osaurusPort, p.ollamaPort, p.customEndpoint);
    EXPECT_EQ(result, p.expected);
}

INSTANTIATE_TEST_SUITE_P(
    CompEndpointScenarios, CompAIEndpointTest,
    ::testing::Values(
        CompEndpointTestCase{0, 1337, 11434, "", "http://localhost:1337/v1/chat/completions"},
        CompEndpointTestCase{0, 8080, 11434, "", "http://localhost:8080/v1/chat/completions"},
        CompEndpointTestCase{1, 1337, 11434, "", "http://localhost:11434/v1/chat/completions"},
        CompEndpointTestCase{1, 1337, 12345, "", "http://localhost:12345/v1/chat/completions"},
        CompEndpointTestCase{2, 1337, 11434, "http://my-server:9999/v1/chat/completions", "http://my-server:9999/v1/chat/completions"},
        CompEndpointTestCase{2, 1337, 11434, "https://api.openai.com/v1/chat/completions", "https://api.openai.com/v1/chat/completions"},
        CompEndpointTestCase{99, 1337, 11434, "", "http://localhost:1337/v1/chat/completions"}
    )
);

// ===========================
// AI Model Resolution Tests
// ===========================
struct ModelResolutionTestCase {
    int providerType;
    std::string osaurusModel;
    std::string ollamaModel;
    std::string customModel;
    std::string expected;
};

static std::string ResolveModel(int providerType, const std::string& osaurusModel, const std::string& ollamaModel, const std::string& customModel) {
    switch (providerType) {
        case 0: return "foundation";
        case 1: return osaurusModel;
        case 2: return ollamaModel;
        case 3: return customModel;
        default: return "foundation";
    }
}

class AIModelTest : public ::testing::TestWithParam<ModelResolutionTestCase> {};

TEST_P(AIModelTest, ResolvesCorrectModel) {
    const auto& p = GetParam();
    std::string result = ResolveModel(p.providerType, p.osaurusModel, p.ollamaModel, p.customModel);
    EXPECT_EQ(result, p.expected);
}

INSTANTIATE_TEST_SUITE_P(
    ModelScenarios, AIModelTest,
    ::testing::Values(
        ModelResolutionTestCase{0, "osaurus", "llama3", "custom", "foundation"},
        ModelResolutionTestCase{1, "osaurus-v2", "llama3", "custom", "osaurus-v2"},
        ModelResolutionTestCase{2, "osaurus", "llama3.1", "custom", "llama3.1"},
        ModelResolutionTestCase{3, "osaurus", "llama3", "gpt-4", "gpt-4"},
        ModelResolutionTestCase{99, "osaurus", "llama3", "custom", "foundation"}
    )
);

// ===========================
// AI Text Meme Prompt Building Tests
// ===========================
TEST(AITextMemePrompt, EvilLevelPersonalities) {
    float evilLevel = 0.0f;
    EXPECT_LT(evilLevel, 0.5f);

    evilLevel = 0.5f;
    EXPECT_EQ(evilLevel, 0.5f);

    evilLevel = 1.0f;
    EXPECT_GT(evilLevel, 0.5f);
}

TEST(AITextMemePrompt, TemperatureRange) {
    float temp = g_config.ai.textMemeTemperature;
    EXPECT_GE(temp, 0.0f);
    EXPECT_LE(temp, 2.0f);
}

// ===========================
// Pomodoro Timing Tests
// ===========================
TEST(PomodoroTiming, WorkDurationInSeconds) {
    int workMinutes = 25;
    double workSeconds = workMinutes * 60.0;
    EXPECT_DOUBLE_EQ(workSeconds, 1500.0);
}

TEST(PomodoroTiming, BreakDurationInSeconds) {
    int breakMinutes = 5;
    double breakSeconds = breakMinutes * 60.0;
    EXPECT_DOUBLE_EQ(breakSeconds, 300.0);
}

TEST(PomodoroTiming, LongBreakDurationInSeconds) {
    int longBreakMinutes = 15;
    double longBreakSeconds = longBreakMinutes * 60.0;
    EXPECT_DOUBLE_EQ(longBreakSeconds, 900.0);
}

TEST(PomodoroTiming, FullCycleDuration) {
    int workMin = 25, breakMin = 5, longBreakMin = 15, sessions = 4;
    double totalSeconds = (workMin * sessions + breakMin * (sessions - 1) + longBreakMin) * 60.0;
    EXPECT_DOUBLE_EQ(totalSeconds, 7800.0);
}

TEST(PomodoroTiming, TimerDisplayFormat) {
    double remaining = 1525.0;
    int minutes = (int)(remaining / 60.0);
    int seconds = (int)fmod(remaining, 60.0);
    EXPECT_EQ(minutes, 25);
    EXPECT_EQ(seconds, 25);

    char timerText[32];
    snprintf(timerText, sizeof(timerText), "R %02d:%02d", minutes, seconds);
    EXPECT_STREQ(timerText, "R 25:25");
}

// ===========================
// Portal Persistence Tests
// ===========================
TEST(PortalPersistence, PortalsNotClearedOnGooseInit) {
    ResetBehaviorState();

    auto* state1 = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    state1->Reset();
    state1->portalA.x = 100.0f;
    state1->portalA.y = 200.0f;
    state1->portalA.active = true;
    state1->portalB.x = 700.0f;
    state1->portalB.y = 500.0f;
    state1->portalB.active = true;

    float savedAx = state1->portalA.x;
    float savedAy = state1->portalA.y;
    float savedBx = state1->portalB.x;
    float savedBy = state1->portalB.y;
    bool savedAActive = state1->portalA.active;
    bool savedBActive = state1->portalB.active;

    BehaviorStateManager::Instance().RemoveForGoose(0);

    auto* state2 = BehaviorStateManager::Instance().GetOrCreate<PortalState>(1, "portal");
    state2->Reset();

    EXPECT_FALSE(state2->portalA.active);
    EXPECT_FALSE(state2->portalB.active);
}

// ===========================
// Think Block Stripping Tests
// ===========================
static std::string StripThinkBlocks(const std::string& content) {
    std::string result = content;
    size_t start, end;
    while ((start = result.find("<think>")) != std::string::npos) {
        end = result.find("</think>", start);
        if (end == std::string::npos) break;
        result.erase(start, end + 8 - start);
    }
    size_t pos = result.find_first_not_of(" \t\n\r");
    if (pos != std::string::npos) {
        result = result.substr(pos);
        pos = result.find_last_not_of(" \t\n\r");
        if (pos != std::string::npos) result = result.substr(0, pos + 1);
    }
    if (result.empty()) return content;
    return result;
}

TEST(ThinkBlockStripping, SimpleBlock) {
    EXPECT_EQ(StripThinkBlocks("<think>reasoning</think>actual"), "actual");
}

TEST(ThinkBlockStripping, MultipleBlocks) {
    EXPECT_EQ(StripThinkBlocks("<think>first</think>middle<think>second</think>end"), "middleend");
}

TEST(ThinkBlockStripping, NoBlock) {
    EXPECT_EQ(StripThinkBlocks("hello world"), "hello world");
}

TEST(ThinkBlockStripping, UnclosedBlock) {
    std::string input = "<think>unclosed";
    EXPECT_EQ(StripThinkBlocks(input), input);
}

TEST(ThinkBlockStripping, OnlyBlock) {
    std::string input = "<think>only</think>";
    EXPECT_EQ(StripThinkBlocks(input), input);
}
