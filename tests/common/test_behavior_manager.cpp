#include "behaviors/states/all.h"
#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

#include "behavior.h"
#include "goose_math.h"
#include "goose.h"
#include "config.h"
#include "world.h"

TEST(BehaviorStateManager, Singleton) {
    auto& mgr1 = BehaviorStateManager::Instance();
    auto& mgr2 = BehaviorStateManager::Instance();
    EXPECT_EQ(&mgr1, &mgr2);
}

TEST(BehaviorStateManager, GetOrCreateNew) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* state = mgr.GetOrCreate<JailState>(1, "jail");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isJailed);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, GetOrCreateExisting) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* state1 = mgr.GetOrCreate<JailState>(1, "jail");
    state1->isJailed = true;

    auto* state2 = mgr.GetOrCreate<JailState>(1, "jail");
    EXPECT_EQ(state1, state2);
    EXPECT_TRUE(state2->isJailed);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, MultipleGooseIds) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* state1 = mgr.GetOrCreate<JailState>(1, "jail");
    auto* state2 = mgr.GetOrCreate<JailState>(2, "jail");

    EXPECT_NE(state1, state2);
    state1->isJailed = true;
    state2->isJailed = false;

    auto* state1Again = mgr.Get<JailState>(1, "jail");
    auto* state2Again = mgr.Get<JailState>(2, "jail");

    EXPECT_TRUE(state1Again->isJailed);
    EXPECT_FALSE(state2Again->isJailed);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, MultipleBehaviorTypes) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* jail = mgr.GetOrCreate<JailState>(1, "jail");
    auto* ball = mgr.GetOrCreate<BallState>(1, "ball");
    auto* acid = mgr.GetOrCreate<AcidState>(1, "acid");

    EXPECT_NE((void*)jail, (void*)ball);
    EXPECT_NE((void*)jail, (void*)acid);
    EXPECT_NE((void*)ball, (void*)acid);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, RemoveForGoose) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    mgr.GetOrCreate<JailState>(1, "jail");
    mgr.GetOrCreate<BallState>(1, "ball");
    mgr.GetOrCreate<JailState>(2, "jail");

    EXPECT_NE(mgr.Get<JailState>(1, "jail"), nullptr);
    EXPECT_NE(mgr.Get<BallState>(1, "ball"), nullptr);
    EXPECT_NE(mgr.Get<JailState>(2, "jail"), nullptr);

    mgr.RemoveForGoose(1);

    EXPECT_EQ(mgr.Get<JailState>(1, "jail"), nullptr);
    EXPECT_EQ(mgr.Get<BallState>(1, "ball"), nullptr);
    EXPECT_NE(mgr.Get<JailState>(2, "jail"), nullptr);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, ThreadSafety) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    std::atomic<int> successCount(0);
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&mgr, &successCount, t]() {
            for (int i = 0; i < 100; ++i) {
                auto* state = mgr.GetOrCreate<JailState>(t * 100 + i, "jail");
                if (state) successCount++;
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(successCount.load(), 400);
    mgr.ClearAll();
}

TEST(BehaviorStateManager, StateCount) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    EXPECT_EQ(mgr.GetStateCount(), 0);

    mgr.GetOrCreate<JailState>(1, "jail");
    mgr.GetOrCreate<JailState>(2, "jail");
    mgr.GetOrCreate<BallState>(1, "ball");

    EXPECT_EQ(mgr.GetStateCount(), 3);

    mgr.ClearAll();
    EXPECT_EQ(mgr.GetStateCount(), 0);
}
