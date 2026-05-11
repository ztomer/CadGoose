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
    EXPECT_EQ(count, 17) << "Expected 17 behaviors to be registered";

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

    auto* gooseManager = registry.Get("gooseManager");
    EXPECT_NE(gooseManager, nullptr) << "gooseManager should be registered";

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