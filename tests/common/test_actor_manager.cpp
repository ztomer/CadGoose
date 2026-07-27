#include "gtest/gtest.h"
#include "actor.h"
#include "goose.h"
#include "world.h"
#include <functional>

// Mock actor subclass specifically designed to test concurrent/synchronous modifications
// of the Actor list during tick and render loops.
class MockModifyActor : public Actor {
public:
    MockModifyActor(const char* actorType, int actorId) 
        : m_type(actorType), m_id(actorId), m_tickCount(0), m_renderCount(0), m_alive(true) {}

    const char* type() const override { return m_type; }
    ActorType actorType() const override { return ActorType::Goose; }
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

    // a2 deletes a1 (the previous actor) when ticked. Snapshot a1's tick count
    // BEFORE freeing it: the assertion below used to read a1->m_tickCount after
    // the delete, which is a use-after-free in the test itself. It only passed
    // because mimalloc leaves the freed block intact; against the system
    // allocator the scribbled memory read back 0 and the test failed.
    int a1TicksAtDeletion = -1;
    a2->m_tickCallback = [&mgr, a1, &a1TicksAtDeletion](MockModifyActor* self) {
        a1TicksAtDeletion = a1->m_tickCount;
        mgr.remove(a1);
        delete a1;
    };

    EXPECT_NO_THROW({
        mgr.tickAll(ctx, 1.0/60.0, 0.0);
    });

    EXPECT_EQ(a1TicksAtDeletion, 1); // a1 was already ticked before a2 deleted it
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

// ── Oracle: edge cases ──

TEST_F(ActorManagerSafetyTest, AddDuringTickDoesNotTickNewActor) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};
    auto* existing = new MockModifyActor("mock_type_a", 1);
    auto* latecomer = new MockModifyActor("mock_type_b", 2);
    mgr.add(existing);

    existing->m_tickCallback = [&mgr, latecomer](MockModifyActor*) {
        mgr.add(latecomer);
    };

    mgr.tickAll(ctx, 1.0/60.0, 0.0);
    EXPECT_EQ(existing->m_tickCount, 1);
    EXPECT_EQ(latecomer->m_tickCount, 0) << "Actor added during tick should not be ticked until next frame";
}

TEST_F(ActorManagerSafetyTest, RemoveNonExistentIsSafe) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};
    auto* a = new MockModifyActor("mock_type_a", 1);
    // Remove without ever adding
    EXPECT_NO_THROW(mgr.remove(a));
    delete a;
}

TEST_F(ActorManagerSafetyTest, TickWithNoActorsIsNoOp) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};
    EXPECT_NO_THROW(mgr.tickAll(ctx, 0.016, 0.0));
    EXPECT_NO_THROW(mgr.renderAll(nullptr));
}

TEST_F(ActorManagerSafetyTest, MultipleDeletionsDuringTick) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};
    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);
    auto* a3 = new MockModifyActor("mock_type_c", 3);
    auto* a4 = new MockModifyActor("mock_type_c", 4);
    mgr.add(a1);
    mgr.add(a2);
    mgr.add(a3);
    mgr.add(a4);

    std::vector<MockModifyActor*> dead;
    a2->m_tickCallback = [&mgr, &dead, a1, a4](MockModifyActor*) {
        mgr.remove(a1);    // remove previous
        dead.push_back(a1);
        mgr.remove(a4);    // remove later (after a3 in snapshot)
        dead.push_back(a4);
    };

    EXPECT_NO_THROW(mgr.tickAll(ctx, 0.016, 0.0));
    EXPECT_EQ(a2->m_tickCount, 1);
    EXPECT_EQ(a3->m_tickCount, 1);
}

TEST_F(ActorManagerSafetyTest, AddAndRemoveSameActorDuringTick) {
    auto& mgr = ActorManager::Instance();
    WorldContext ctx{};
    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);
    mgr.add(a1);
    mgr.add(a2);

    a1->m_tickCallback = [&mgr](MockModifyActor* self) {
        mgr.remove(self);
        mgr.add(self);
    };

    EXPECT_NO_THROW(mgr.tickAll(ctx, 0.016, 0.0));
}

TEST_F(ActorManagerSafetyTest, FindByTypeSkippedForNonActive) {
    auto& mgr = ActorManager::Instance();
    auto* a = new MockModifyActor("mock_type_a", 1);
    mgr.add(a);
    a->setActive(false);

    EXPECT_EQ(mgr.findByType("mock_type_a"), nullptr);
    EXPECT_EQ(mgr.findByType(ActorType::Goose), nullptr);
}

TEST_F(ActorManagerSafetyTest, DestroyAllOfTypeRemovesCorrectly) {
    auto& mgr = ActorManager::Instance();
    mgr.destroyAllOfType("mock_type_a");
    auto* a1 = new MockModifyActor("mock_type_a", 1);
    auto* a2 = new MockModifyActor("mock_type_b", 2);
    mgr.add(a1);
    mgr.add(a2);

    mgr.destroyAllOfType("mock_type_a");
    EXPECT_EQ(mgr.findByType("mock_type_a"), nullptr);
    EXPECT_NE(mgr.findByType("mock_type_b"), nullptr);
}

TEST_F(ActorManagerSafetyTest, GeeseCacheInvalidatesOnRemove) {
    auto& mgr = ActorManager::Instance();
    EXPECT_TRUE(mgr.getGeese().empty());
    Goose* g = new Goose(1, "Gander", 1920, 1080);
    mgr.add(g);
    EXPECT_EQ(mgr.getGeese().size(), 1u);

    mgr.remove(g);
    delete g;
    EXPECT_TRUE(mgr.getGeese().empty());
}
