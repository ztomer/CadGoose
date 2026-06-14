#include <gtest/gtest.h>
#include "actor.h"
#include "world.h"

class TestActor : public Actor {
public:
    TestActor() : Actor() {}
    const char* type() const override { return "test"; }
    void tick(WorldContext&, double, double) override {}
    void render(IRenderer*) override {}
    bool isAlive() const override { return true; }
};

TEST(ActorTest, DefaultIdReturnsZero) {
    TestActor actor;
    EXPECT_EQ(actor.id(), 0);
}

TEST(ActorTest, SetPosition) {
    TestActor actor;
    actor.setPosition(DevicePoint(10, 20));
    EXPECT_EQ(actor.position().x, 10);
    EXPECT_EQ(actor.position().y, 20);
}

TEST(ActorTest, SetRadius) {
    TestActor actor;
    actor.setRadius(5.0f);
    EXPECT_EQ(actor.radius(), 5.0f);
}

TEST(ActorTest, DefaultActiveState) {
    TestActor actor;
    EXPECT_TRUE(actor.isActive());
    actor.setActive(false);
    EXPECT_FALSE(actor.isActive());
    actor.setActive(true);
    EXPECT_TRUE(actor.isActive());
}
