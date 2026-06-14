#include "gtest/gtest.h"
#include "actor.h"
#include "world.h"
#include <functional>

// Mock actor subclass specifically designed to test concurrent/synchronous modifications
// of the Actor list during tick and render loops.
class MockModifyActor : public Actor {
public:
    MockModifyActor(const char* actorType, int actorId) 
        : m_type(actorType), m_id(actorId), m_tickCount(0), m_renderCount(0), m_alive(true) {}

    const char* type() const override { return m_type; }
    int id() const override { return m_id; }
    bool isAlive() const override { return m_alive; }
    void setAlive(bool alive) { m_alive = alive; }

    void tick(WorldContext& ctx, double dt, double time) override {
        m_tickCount++;
        if (m_tickCallback) {
            m_tickCallback(this);
        }
    }

    void render(IRenderer* renderer) override {
        m_renderCount++;
        if (m_renderCallback) {
            m_renderCallback(this);
        }
    }

    int m_tickCount;
    int m_renderCount;
    std::function<void(MockModifyActor*)> m_tickCallback;
    std::function<void(MockModifyActor*)> m_renderCallback;
    bool m_alive;
    const char* m_type;
    int m_id;
};

class ActorManagerSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean any existing actors first
        auto& mgr = ActorManager::Instance();
        mgr.destroyAllOfType("mock_type_a");
        mgr.destroyAllOfType("mock_type_b");
        mgr.destroyAllOfType("mock_type_c");
    }

    void TearDown() override {
        auto& mgr = ActorManager::Instance();
        mgr.destroyAllOfType("mock_type_a");
        mgr.destroyAllOfType("mock_type_b");
        mgr.destroyAllOfType("mock_type_c");
    }
};

// Test Case 1: Actor deletes itself during tick.
// This mimics the ball behavior toggling cleanup which deletes s_ballActor.
TEST_F(ActorManagerSafetyTest, SelfDeletionDuringTickIsSafe) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};

    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);

    mgr.add(a1);
    mgr.add(a2);

    // a1 deletes itself when ticked
    a1->m_tickCallback = [&mgr](MockModifyActor* self) {
        mgr.remove(self);
        delete self;
    };

    // This should run safely without crashing or dereferencing deleted a1.
    EXPECT_NO_THROW({
        mgr.tickAll(ctx, 1.0/60.0, 0.0);
    });

    // a2 should still have been ticked
    EXPECT_EQ(a2->m_tickCount, 1);
}

// Test Case 2: Actor deletes the *next* actor in the loop during tick.
TEST_F(ActorManagerSafetyTest, NextDeletionDuringTickIsSafe) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};

    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);
    auto* a3 = new MockModifyActor("mock_type_c", 3);

    mgr.add(a1);
    mgr.add(a2);
    mgr.add(a3);

    // a1 deletes a2 (the next actor) when ticked
    a1->m_tickCallback = [&mgr, a2](MockModifyActor* self) {
        mgr.remove(a2);
        delete a2;
    };

    EXPECT_NO_THROW({
        mgr.tickAll(ctx, 1.0/60.0, 0.0);
    });

    // a1 and a3 should be ticked, but not deleted a2
    EXPECT_EQ(a1->m_tickCount, 1);
    EXPECT_EQ(a3->m_tickCount, 1);
}

// Test Case 3: Actor deletes the *previous* actor in the loop during tick.
TEST_F(ActorManagerSafetyTest, PreviousDeletionDuringTickIsSafe) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};

    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);

    mgr.add(a1);
    mgr.add(a2);

    // a2 deletes a1 (the previous actor) when ticked
    a2->m_tickCallback = [&mgr, a1](MockModifyActor* self) {
        mgr.remove(a1);
        delete a1;
    };

    EXPECT_NO_THROW({
        mgr.tickAll(ctx, 1.0/60.0, 0.0);
    });

    EXPECT_EQ(a1->m_tickCount, 1); // was already ticked
    EXPECT_EQ(a2->m_tickCount, 1);
}

// Test Case 4: Actor deletes itself during render.
TEST_F(ActorManagerSafetyTest, SelfDeletionDuringRenderIsSafe) {
    auto& mgr = ActorManager::Instance();

    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);

    mgr.add(a1);
    mgr.add(a2);

    // a1 deletes itself when rendered
    a1->m_renderCallback = [&mgr](MockModifyActor* self) {
        mgr.remove(self);
        delete self;
    };

    EXPECT_NO_THROW({
        mgr.renderAll(nullptr);
    });

    EXPECT_EQ(a2->m_renderCount, 1);
}

// Test Case 5: Actor deletes the *next* actor in the loop during render.
TEST_F(ActorManagerSafetyTest, NextDeletionDuringRenderIsSafe) {
    auto& mgr = ActorManager::Instance();

    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);
    auto* a3 = new MockModifyActor("mock_type_c", 3);

    mgr.add(a1);
    mgr.add(a2);
    mgr.add(a3);

    // a1 deletes a2 (the next actor) when rendered
    a1->m_renderCallback = [&mgr, a2](MockModifyActor* self) {
        mgr.remove(a2);
        delete a2;
    };

    EXPECT_NO_THROW({
        mgr.renderAll(nullptr);
    });

    EXPECT_EQ(a1->m_renderCount, 1);
    EXPECT_EQ(a3->m_renderCount, 1);
}

TEST_F(ActorManagerSafetyTest, FindByTypeReturnsCorrectActor) {
    auto& mgr = ActorManager::Instance();
    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);
    mgr.add(a1);
    mgr.add(a2);

    Actor* found = mgr.findByType("mock_type_a");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 1);

    found = mgr.findByType("mock_type_a", 1);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 1);

    found = mgr.findByType("mock_type_b", 2);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 2);
}

TEST_F(ActorManagerSafetyTest, FindByTypeNotFound) {
    auto& mgr = ActorManager::Instance();
    EXPECT_EQ(mgr.findByType("nonexistent"), nullptr);
    EXPECT_EQ(mgr.findByType("nonexistent", 99), nullptr);
}

TEST_F(ActorManagerSafetyTest, FindByTypeWithId) {
    auto& mgr = ActorManager::Instance();
    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_a", 2);
    mgr.add(a1);
    mgr.add(a2);

    Actor* found = mgr.findByType("mock_type_a", 1);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 1);

    found = mgr.findByType("mock_type_a", 2);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), 2);
}
