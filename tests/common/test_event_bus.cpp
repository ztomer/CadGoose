#include <gtest/gtest.h>
#include "event_bus.h"
#include <atomic>

// Helper to capture events
struct EventCapture {
    std::atomic<int> count{0};
    int lastGooseId = -1;
    float lastX = 0, lastY = 0;
    double lastTime = 0;
    float lastAmount = 0;
    const char* lastItemType = nullptr;
    int lastPhase = -1;
    float lastVx = 0, lastVy = 0;
    int lastType = -1;
    float lastBallVx = 0, lastBallVy = 0;
    float lastFromX = 0, lastFromY = 0, lastToX = 0, lastToY = 0;
    int lastPortalId = -1;
    double lastStuckDuration = 0;
};

TEST(EventBusTest, SingletonInstance) {
    auto& bus1 = EventBus::Instance();
    auto& bus2 = EventBus::Instance();
    EXPECT_EQ(&bus1, &bus2);
}

TEST(EventBusTest, SubscribeAndPublishGooseHonked) {
    EventBus::Instance().Clear();
    EventCapture capture;

    auto sub = EventBus::Instance().Subscribe<GooseHonkedEvent>([&capture](const GooseHonkedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastX = e.x;
        capture.lastY = e.y;
        capture.lastTime = e.time;
    });

    GooseHonkedEvent event{42, 100.0f, 200.0f, 1.5};
    EventBus::Instance().Publish(event);

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 42);
    EXPECT_FLOAT_EQ(capture.lastX, 100.0f);
    EXPECT_FLOAT_EQ(capture.lastY, 200.0f);
    EXPECT_DOUBLE_EQ(capture.lastTime, 1.5);

    EventBus::Instance().Unsubscribe(sub);
}

TEST(EventBusTest, MultipleSubscribers) {
    EventBus::Instance().Clear();
    std::atomic<int> count1{0}, count2{0};

    auto sub1 = EventBus::Instance().Subscribe<GooseHonkedEvent>([&count1](const GooseHonkedEvent&) { count1++; });
    auto sub2 = EventBus::Instance().Subscribe<GooseHonkedEvent>([&count2](const GooseHonkedEvent&) { count2++; });

    GooseHonkedEvent event{1, 0, 0, 0};
    EventBus::Instance().Publish(event);

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);

    EventBus::Instance().Unsubscribe(sub1);
    EventBus::Instance().Unsubscribe(sub2);
}

TEST(EventBusTest, UnsubscribeStopsDelivery) {
    EventBus::Instance().Clear();
    std::atomic<int> count{0};

    auto sub = EventBus::Instance().Subscribe<GooseHonkedEvent>([&count](const GooseHonkedEvent&) { count++; });

    GooseHonkedEvent event{1, 0, 0, 0};
    EventBus::Instance().Publish(event);
    EXPECT_EQ(count, 1);

    EventBus::Instance().Unsubscribe(sub);
    EventBus::Instance().Publish(event);
    EXPECT_EQ(count, 1); // Should not increment after unsubscribe
}

TEST(EventBusTest, DifferentEventTypesIndependent) {
    EventBus::Instance().Clear();
    std::atomic<int> honkCount{0}, damageCount{0};

    EventBus::Instance().Subscribe<GooseHonkedEvent>([&honkCount](const GooseHonkedEvent&) { honkCount++; });
    EventBus::Instance().Subscribe<GooseDamagedEvent>([&damageCount](const GooseDamagedEvent&) { damageCount++; });

    EventBus::Instance().Publish(GooseHonkedEvent{1, 0, 0, 0});
    EXPECT_EQ(honkCount, 1);
    EXPECT_EQ(damageCount, 0);

    EventBus::Instance().Publish(GooseDamagedEvent{1, 10.0f, 0});
    EXPECT_EQ(honkCount, 1);
    EXPECT_EQ(damageCount, 1);
}

TEST(EventBusTest, GooseDamagedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<GooseDamagedEvent>([&capture](const GooseDamagedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastAmount = e.amount;
        capture.lastTime = e.time;
    });

    EventBus::Instance().Publish(GooseDamagedEvent{7, 25.5f, 3.14});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 7);
    EXPECT_FLOAT_EQ(capture.lastAmount, 25.5f);
    EXPECT_DOUBLE_EQ(capture.lastTime, 3.14);
}

TEST(EventBusTest, ItemDroppedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<ItemDroppedEvent>([&capture](const ItemDroppedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastX = e.x;
        capture.lastY = e.y;
        capture.lastItemType = e.itemType;
    });

    EventBus::Instance().Publish(ItemDroppedEvent{3, 50.0f, 75.0f, "toy"});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 3);
    EXPECT_FLOAT_EQ(capture.lastX, 50.0f);
    EXPECT_FLOAT_EQ(capture.lastY, 75.0f);
    EXPECT_STREQ(capture.lastItemType, "toy");
}

TEST(EventBusTest, ItemEatenEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<ItemEatenEvent>([&capture](const ItemEatenEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastItemType = e.itemType;
    });

    EventBus::Instance().Publish(ItemEatenEvent{5, 10.0f, 20.0f, "breadcrumb"});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 5);
    EXPECT_STREQ(capture.lastItemType, "breadcrumb");
}

TEST(EventBusTest, GooseJailedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<GooseJailedEvent>([&capture](const GooseJailedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastX = e.x;
        capture.lastY = e.y;
    });

    EventBus::Instance().Publish(GooseJailedEvent{2, 300.0f, 400.0f});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 2);
    EXPECT_FLOAT_EQ(capture.lastX, 300.0f);
    EXPECT_FLOAT_EQ(capture.lastY, 400.0f);
}

TEST(EventBusTest, GooseFreedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<GooseFreedEvent>([&capture](const GooseFreedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
    });

    EventBus::Instance().Publish(GooseFreedEvent{9});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 9);
}

TEST(EventBusTest, PomodoroPhaseChangedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<PomodoroPhaseChangedEvent>([&capture](const PomodoroPhaseChangedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastPhase = e.phase;
        capture.lastTime = e.time;
    });

    EventBus::Instance().Publish(PomodoroPhaseChangedEvent{1, 1, 100.0});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 1);
    EXPECT_EQ(capture.lastPhase, 1);
    EXPECT_DOUBLE_EQ(capture.lastTime, 100.0);
}

TEST(EventBusTest, GooseStuckEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<GooseStuckEvent>([&capture](const GooseStuckEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastStuckDuration = e.stuckDuration;
    });

    EventBus::Instance().Publish(GooseStuckEvent{4, 500.0f, 600.0f, 5.0});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 4);
    EXPECT_DOUBLE_EQ(capture.lastStuckDuration, 5.0);
}

TEST(EventBusTest, CursorFastMoveEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<CursorFastMoveEvent>([&capture](const CursorFastMoveEvent& e) {
        capture.count++;
        capture.lastVx = e.vx;
        capture.lastVy = e.vy;
        capture.lastX = e.x;
        capture.lastY = e.y;
    });

    EventBus::Instance().Publish(CursorFastMoveEvent{100.0f, -50.0f, 200.0f, 300.0f});

    EXPECT_EQ(capture.count, 1);
    EXPECT_FLOAT_EQ(capture.lastVx, 100.0f);
    EXPECT_FLOAT_EQ(capture.lastVy, -50.0f);
    EXPECT_FLOAT_EQ(capture.lastX, 200.0f);
    EXPECT_FLOAT_EQ(capture.lastY, 300.0f);
}

TEST(EventBusTest, ToySpawnedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<ToySpawnedEvent>([&capture](const ToySpawnedEvent& e) {
        capture.count++;
        capture.lastX = e.x;
        capture.lastY = e.y;
        capture.lastType = e.type;
    });

    EventBus::Instance().Publish(ToySpawnedEvent{150.0f, 250.0f, 1});

    EXPECT_EQ(capture.count, 1);
    EXPECT_FLOAT_EQ(capture.lastX, 150.0f);
    EXPECT_FLOAT_EQ(capture.lastY, 250.0f);
    EXPECT_EQ(capture.lastType, 1);
}

TEST(EventBusTest, BallKickedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<BallKickedEvent>([&capture](const BallKickedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastBallVx = e.ballVx;
        capture.lastBallVy = e.ballVy;
    });

    EventBus::Instance().Publish(BallKickedEvent{6, 100.0f, 200.0f, 50.0f, -30.0f});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 6);
    EXPECT_FLOAT_EQ(capture.lastBallVx, 50.0f);
    EXPECT_FLOAT_EQ(capture.lastBallVy, -30.0f);
}

TEST(EventBusTest, BreadcrumbDroppedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<BreadcrumbDroppedEvent>([&capture](const BreadcrumbDroppedEvent& e) {
        capture.count++;
        capture.lastX = e.x;
        capture.lastY = e.y;
    });

    EventBus::Instance().Publish(BreadcrumbDroppedEvent{42.0f, 84.0f});

    EXPECT_EQ(capture.count, 1);
    EXPECT_FLOAT_EQ(capture.lastX, 42.0f);
    EXPECT_FLOAT_EQ(capture.lastY, 84.0f);
}

TEST(EventBusTest, GooseTeleportedEvent) {
    EventBus::Instance().Clear();
    EventCapture capture;

    EventBus::Instance().Subscribe<GooseTeleportedEvent>([&capture](const GooseTeleportedEvent& e) {
        capture.count++;
        capture.lastGooseId = e.gooseId;
        capture.lastPortalId = e.portalId;
        capture.lastFromX = e.fromX;
        capture.lastFromY = e.fromY;
        capture.lastToX = e.toX;
        capture.lastToY = e.toY;
    });

    EventBus::Instance().Publish(GooseTeleportedEvent{3, 1, 100.0f, 200.0f, 500.0f, 600.0f});

    EXPECT_EQ(capture.count, 1);
    EXPECT_EQ(capture.lastGooseId, 3);
    EXPECT_EQ(capture.lastPortalId, 1);
    EXPECT_FLOAT_EQ(capture.lastFromX, 100.0f);
    EXPECT_FLOAT_EQ(capture.lastFromY, 200.0f);
    EXPECT_FLOAT_EQ(capture.lastToX, 500.0f);
    EXPECT_FLOAT_EQ(capture.lastToY, 600.0f);
}

TEST(EventBusTest, ClearRemovesAllSubscriptions) {
    EventBus::Instance().Clear();
    std::atomic<int> count{0};

    EventBus::Instance().Subscribe<GooseHonkedEvent>([&count](const GooseHonkedEvent&) { count++; });
    EventBus::Instance().Subscribe<GooseDamagedEvent>([&count](const GooseDamagedEvent&) { count++; });

    EventBus::Instance().Clear();

    EventBus::Instance().Publish(GooseHonkedEvent{1, 0, 0, 0});
    EventBus::Instance().Publish(GooseDamagedEvent{1, 0, 0});

    EXPECT_EQ(count, 0);
}

TEST(EventBusTest, SubscriptionIdsAreUnique) {
    EventBus::Instance().Clear();

    auto id1 = EventBus::Instance().Subscribe<GooseHonkedEvent>([](const GooseHonkedEvent&) {});
    auto id2 = EventBus::Instance().Subscribe<GooseHonkedEvent>([](const GooseHonkedEvent&) {});
    auto id3 = EventBus::Instance().Subscribe<GooseDamagedEvent>([](const GooseDamagedEvent&) {});

    EXPECT_NE(id1, id2);
    EXPECT_NE(id1, id3);
    EXPECT_NE(id2, id3);

    EventBus::Instance().Unsubscribe(id1);
    EventBus::Instance().Unsubscribe(id2);
    EventBus::Instance().Unsubscribe(id3);
}

TEST(EventBusTest, PublishWithNoSubscribers) {
    EventBus::Instance().Clear();
    // Should not crash
    EventBus::Instance().Publish(GooseHonkedEvent{1, 0, 0, 0});
    EventBus::Instance().Publish(GooseDamagedEvent{1, 0, 0});
    EventBus::Instance().Publish(CursorFastMoveEvent{0, 0, 0, 0});
}

TEST(EventBusTest, CrossBehaviorScenario) {
    // Simulate: honcker publishes honk event, anger behavior reacts
    EventBus::Instance().Clear();

    std::atomic<int> angerLevel{0};

    // Anger behavior subscribes to honk events
    EventBus::Instance().Subscribe<GooseHonkedEvent>([&angerLevel](const GooseHonkedEvent&) {
        angerLevel++;
    });

    // Honcker behavior publishes
    EventBus::Instance().Publish(GooseHonkedEvent{1, 100, 100, 0});
    EventBus::Instance().Publish(GooseHonkedEvent{1, 100, 100, 0});
    EventBus::Instance().Publish(GooseHonkedEvent{1, 100, 100, 0});

    EXPECT_EQ(angerLevel, 3);
}

TEST(EventBusTest, UnsubscribeNonExistent) {
    EventBus::Instance().Clear();
    // Should not crash
    EventBus::Instance().Unsubscribe(99999);
}
