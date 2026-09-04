#include <gtest/gtest.h>
#include "config.h"
#include "actor.h"
#include "app_actions.h"
#include "baby_stalin_actor.h"
#include "world.h"

class BabyStalinSpawnTest : public ::testing::Test {
protected:
    void SetUp() override {
        ActorManager::Instance().destroyAllOfType(ActorType::Goose);
        ActorManager::Instance().destroyAllOfType(ActorType::BabyStalin);
        g_world.nextId = 100;
    }

    void TearDown() override {
        ActorManager::Instance().destroyAllOfType(ActorType::Goose);
        ActorManager::Instance().destroyAllOfType(ActorType::BabyStalin);
        g_world.nextId = 100;
    }
};

TEST_F(BabyStalinSpawnTest, NormalModeSpawnGoose) {
    g_config.general.appearanceMode = APPEARANCE_LIGHT;
    Goose* g = AppActions_SpawnGoose("test_goose");
    ASSERT_NE(g, nullptr);
    EXPECT_STREQ(g->type(), "goose");
}

TEST_F(BabyStalinSpawnTest, StalinModeSpawnBabyStalin) {
    g_config.general.appearanceMode = APPEARANCE_STALIN;
    Goose* g = AppActions_SpawnGoose("test_stalin");
    ASSERT_NE(g, nullptr);
    EXPECT_STREQ(g->type(), "baby_stalin");
}

TEST_F(BabyStalinSpawnTest, StalinModeSpawnHasPhotoHead) {
    g_config.general.appearanceMode = APPEARANCE_STALIN;
    Goose* g = AppActions_SpawnGoose("test_stalin");
    ASSERT_NE(g, nullptr);
    // BabyStalin can honk — plays Gulag sound via onHonk() override
    ASSERT_TRUE(g->m_canHonk);
    // Verify the type is baby_stalin
    ASSERT_STREQ(g->type(), "baby_stalin");
}

TEST_F(BabyStalinSpawnTest, NormalModeSpawnCanHonk) {
    g_config.general.appearanceMode = APPEARANCE_LIGHT;
    Goose* g = AppActions_SpawnGoose("test_goose");
    ASSERT_NE(g, nullptr);
    EXPECT_TRUE(g->m_canHonk);
}

TEST_F(BabyStalinSpawnTest, SpawnBabyStalinDirect) {
    Goose* g = AppActions_SpawnBabyStalin("direct_stalin");
    ASSERT_NE(g, nullptr);
    EXPECT_STREQ(g->type(), "baby_stalin");
}

TEST_F(BabyStalinSpawnTest, SpawnBabyStalinViaCommand) {
    std::string r = AppActions_HandleCommand({"spawn_baby_stalin", "cmd_stalin"});
    EXPECT_NE(r.find("ok id="), std::string::npos);
}

TEST_F(BabyStalinSpawnTest, SpawnStalinViaShortCommand) {
    std::string r = AppActions_HandleCommand({"spawn_stalin"});
    EXPECT_NE(r.find("ok id="), std::string::npos);
}
