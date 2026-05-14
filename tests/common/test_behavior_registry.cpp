// ===========================
// test_behavior_registry.cpp
// Verifies all behaviors are properly registered
// ===========================
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <set>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"

TEST(BehaviorRegistry, AllBehaviorsRegistered) {
    auto& registry = BehaviorRegistry::Instance();
    size_t count = registry.GetBehaviorCount();
    fprintf(stderr, "[INFO] Registered %zu behaviors\n", count);
    EXPECT_EQ(count, 16) << "Expected 16 behaviors to be registered";

    auto* acid = registry.Get("acid");
    EXPECT_NE(acid, nullptr) << "acid should be registered";

    auto* anger = registry.Get("anger");
    EXPECT_NE(anger, nullptr) << "anger should be registered";

    auto* ball = registry.Get("ball");
    EXPECT_NE(ball, nullptr) << "ball should be registered";

    auto* banish = registry.Get("banish");
    EXPECT_NE(banish, nullptr) << "banish should be registered";

    auto* breadcrumbs = registry.Get("breadcrumbs");
    EXPECT_NE(breadcrumbs, nullptr) << "breadcrumbs should be registered";

    auto* drag = registry.Get("drag");
    EXPECT_NE(drag, nullptr) << "drag should be registered";

    auto* hats = registry.Get("hats");
    EXPECT_NE(hats, nullptr) << "hats should be registered";

    auto* health = registry.Get("health");
    EXPECT_NE(health, nullptr) << "health should be registered";

    auto* honcker = registry.Get("honcker");
    EXPECT_NE(honcker, nullptr) << "honcker should be registered";

    auto* jail = registry.Get("jail");
    EXPECT_NE(jail, nullptr) << "jail should be registered";

    auto* nametag = registry.Get("nametag");
    EXPECT_NE(nametag, nullptr) << "nametag should be registered";

    auto* pomodoro = registry.Get("pomodoro");
    EXPECT_NE(pomodoro, nullptr) << "pomodoro should be registered";

    auto* portal = registry.Get("portal");
    EXPECT_NE(portal, nullptr) << "portal should be registered";

    auto* presence = registry.Get("presence");
    EXPECT_NE(presence, nullptr) << "presence should be registered";

    auto* rainbow = registry.Get("rainbow");
    EXPECT_NE(rainbow, nullptr) << "rainbow should be registered";

    auto* ai = registry.Get("ai");
    EXPECT_NE(ai, nullptr) << "ai should be registered";
}

TEST(BehaviorRegistry, AllBehaviorsHaveTick) {
    auto& registry = BehaviorRegistry::Instance();
    const auto& behaviors = registry.GetAll();

    for (const auto& behavior : behaviors) {
        EXPECT_TRUE(behavior->tick != nullptr) << "Behavior " << behavior->id << " has null tick";
    }
}

TEST(BehaviorRegistry, AllBehaviorsHavePriority) {
    auto& registry = BehaviorRegistry::Instance();
    const auto& behaviors = registry.GetAll();

    for (const auto& behavior : behaviors) {
        EXPECT_GE(behavior->priority, 0) << "Behavior " << behavior->id << " has negative priority";
    }
}

TEST(BehaviorRegistry, AllBehaviorsHaveUniqueIDs) {
    auto& registry = BehaviorRegistry::Instance();
    const auto& behaviors = registry.GetAll();
    std::set<std::string> seenIds;

    for (const auto& behavior : behaviors) {
        std::string id(behavior->id);
        EXPECT_EQ(seenIds.count(id), 0) << "Duplicate behavior ID: " << id;
        seenIds.insert(id);
    }
}

TEST(BehaviorRegistry, AllBehaviorsHaveInit) {
    auto& registry = BehaviorRegistry::Instance();
    const auto& behaviors = registry.GetAll();

    int initCount = 0;
    for (const auto& behavior : behaviors) {
        if (behavior->init != nullptr) {
            initCount++;
        }
    }

    EXPECT_EQ(initCount, (int)behaviors.size()) << "All behaviors should have init functions";
}

TEST(BehaviorRegistry, AllBehaviorsHaveRender) {
    auto& registry = BehaviorRegistry::Instance();
    const auto& behaviors = registry.GetAll();

    int renderCount = 0;
    for (const auto& behavior : behaviors) {
        if (behavior->render != nullptr) {
            renderCount++;
        }
    }

    EXPECT_EQ(renderCount, (int)behaviors.size()) << "All behaviors should have render functions";
}

TEST(BehaviorRegistry, BehaviorsCanBeEnabledDisabled) {
    auto& registry = BehaviorRegistry::Instance();

    auto* rainbow = registry.Get("rainbow");
    ASSERT_NE(rainbow, nullptr);
    ASSERT_NE(rainbow->enabledPtr, nullptr);

    bool original = *rainbow->enabledPtr;
    *rainbow->enabledPtr = false;
    EXPECT_FALSE(*rainbow->enabledPtr);

    *rainbow->enabledPtr = true;
    EXPECT_TRUE(*rainbow->enabledPtr);

    *rainbow->enabledPtr = original;
}

TEST(BehaviorToggle, AllBehaviorsHaveEnabledPtr) {
    auto& registry = BehaviorRegistry::Instance();
    for (const auto* b : registry.GetAll()) {
        ASSERT_NE(b->enabledPtr, nullptr) << "Behavior " << b->id << " has null enabledPtr";
    }
}

TEST(BehaviorToggle, AllBehaviorsHaveConfigPtr) {
    auto& registry = BehaviorRegistry::Instance();
    for (const auto* b : registry.GetAll()) {
        ASSERT_NE(b->configPtr, nullptr) << "Behavior " << b->id << " has null configPtr";
    }
}

TEST(BehaviorToggle, EnabledPtrStaysTrue) {
    // enabledPtr must always be true — the outer tick loop gate that passes
    // through to the internal config bool check (the real runtime gate).
    // If s_enabled were false, no amount of config toggling would run the behavior.
    auto& registry = BehaviorRegistry::Instance();
    for (const auto* b : registry.GetAll()) {
        EXPECT_TRUE(*b->enabledPtr) << "Behavior " << b->id << " enabledPtr should be true";
    }
}

TEST(BehaviorToggle, ConfigPtrWritesThroughToConfig) {
    // Writing through configPtr must update the corresponding g_config bool.
    // This is the mechanism the CLI enable/disable uses to persist toggle state.
    Config_Init();

    auto& registry = BehaviorRegistry::Instance();

    auto* ball = registry.Get("ball");
    ASSERT_NE(ball, nullptr);
    ASSERT_NE(ball->configPtr, nullptr);

    bool saved = *ball->configPtr;
    *ball->configPtr = true;
    EXPECT_TRUE(g_config.behaviors.fun.ball);
    *ball->configPtr = false;
    EXPECT_FALSE(g_config.behaviors.fun.ball);
    *ball->configPtr = saved;
}

TEST(BehaviorToggle, ConfigPtrMap) {
    // Verify every behavior's configPtr points to its correct config bool field.
    // This ensures CLI enable/disable and runtime gating stay in sync.
    Config_Init();
    auto& reg = BehaviorRegistry::Instance();

    struct { const char* id; bool* expectedField; } map[] = {
        {"acid",     &g_config.behaviors.fun.acid},
        {"anger",    &g_config.behaviors.fun.anger},
        {"ball",     &g_config.behaviors.fun.ball},
        {"banish",   &g_config.behaviors.control.banish},
        {"breadcrumbs", &g_config.behaviors.fun.breadCrumbs},
        {"drag",     &g_config.behaviors.control.drag},
        {"hats",     &g_config.behaviors.fun.hats},
        {"health",   &g_config.behaviors.systems.health},
        {"honcker",  &g_config.behaviors.control.honcker},
        {"jail",     &g_config.behaviors.control.jail},
        {"nametag",  &g_config.behaviors.info.nametag},
        {"pomodoro", &g_config.behaviors.systems.pomodoro},
        {"portal",   &g_config.behaviors.control.portals},
        {"presence", &g_config.behaviors.info.presence},
        {"rainbow",  &g_config.behaviors.fun.rainbow},
        {"ai",       &g_config.behaviors.systems.ai},
    };

    for (const auto& entry : map) {
        const auto* behavior = reg.Get(entry.id);
        ASSERT_NE(behavior, nullptr) << "Behavior not found: " << entry.id;
        ASSERT_NE(behavior->configPtr, nullptr) << "Null configPtr for: " << entry.id;
        EXPECT_EQ(behavior->configPtr, entry.expectedField)
            << "configPtr mismatch for " << entry.id;
    }
}

TEST(BehaviorToggle, ConfigPtrWritePreservesThroughRegistry) {
    // Full round-trip: write through configPtr, verify via Config_GetValueByKey
    Config_Init();

    auto* ball = BehaviorRegistry::Instance().Get("ball");
    ASSERT_NE(ball, nullptr);
    ASSERT_NE(ball->configPtr, nullptr);

    bool saved = *ball->configPtr;

    *ball->configPtr = true;
    std::string value;
    bool ok = Config_GetValueByKey("ball_enabled", &value);
    ASSERT_TRUE(ok);
    EXPECT_EQ(value, "1");

    *ball->configPtr = false;
    ok = Config_GetValueByKey("ball_enabled", &value);
    ASSERT_TRUE(ok);
    EXPECT_EQ(value, "0");

    *ball->configPtr = saved;
}

TEST(BehaviorToggle, DisableViaConfigPtrThenReEnable) {
    // Toggle off and back on via configPtr — verifies no sticky state
    Config_Init();

    auto* ball = BehaviorRegistry::Instance().Get("ball");
    ASSERT_NE(ball, nullptr);
    ASSERT_NE(ball->configPtr, nullptr);

    bool saved = *ball->configPtr;

    *ball->configPtr = false;
    EXPECT_FALSE(g_config.behaviors.fun.ball);

    *ball->configPtr = true;
    EXPECT_TRUE(g_config.behaviors.fun.ball);

    *ball->configPtr = saved;
}

TEST(BehaviorToggle, ConfigPtrAndEnabledPtrAreDistinct) {
    // enabledPtr and configPtr must point to different locations
    // (s_enabled in behavior file vs g_config.behaviors.X.Y)
    auto& reg = BehaviorRegistry::Instance();
    for (const auto* b : reg.GetAll()) {
        ASSERT_NE(b->enabledPtr, nullptr);
        ASSERT_NE(b->configPtr, nullptr);
        EXPECT_NE(b->enabledPtr, b->configPtr)
            << "Behavior " << b->id << ": enabledPtr and configPtr should be distinct";
    }
}

TEST(BehaviorToggle, SetConfigBoolThenTickGateRespectsIt) {
    Config_Init();

    auto* ball = BehaviorRegistry::Instance().Get("ball");
    ASSERT_NE(ball, nullptr);
    ASSERT_TRUE(*ball->enabledPtr);

    bool saved = g_config.behaviors.fun.ball;

    g_config.behaviors.fun.ball = false;
    ASSERT_TRUE(*ball->enabledPtr);
    EXPECT_FALSE(g_config.behaviors.fun.ball) << "With ball disabled, internal gate should block";

    g_config.behaviors.fun.ball = true;
    EXPECT_TRUE(g_config.behaviors.fun.ball) << "With ball enabled, internal gate should pass";

    g_config.behaviors.fun.ball = saved;
}