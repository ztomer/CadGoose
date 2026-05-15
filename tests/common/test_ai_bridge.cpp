#include <gtest/gtest.h>
#include <string>
#include "ai_mcp_bridge.h"
#include "mcp_server.h"
#include "config.h"

class AIMCPIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        savedBall = g_config.behaviors.fun.ball;
        savedHoncker = g_config.behaviors.control.honcker;
        savedHats = g_config.behaviors.fun.hats;
        savedRainbow = g_config.behaviors.fun.rainbow;
        savedAcid = g_config.behaviors.fun.acid;
        savedBreadcrumbs = g_config.behaviors.fun.breadCrumbs;
        savedJail = g_config.behaviors.control.jail;
        savedNametag = g_config.behaviors.info.nametag;
        savedHealth = g_config.behaviors.systems.health;
        savedAi = g_config.behaviors.systems.ai;
        savedPomodoro = g_config.behaviors.systems.pomodoro;
    }
    void TearDown() override {
        g_config.behaviors.fun.ball = savedBall;
        g_config.behaviors.control.honcker = savedHoncker;
        g_config.behaviors.fun.hats = savedHats;
        g_config.behaviors.fun.rainbow = savedRainbow;
        g_config.behaviors.fun.acid = savedAcid;
        g_config.behaviors.fun.breadCrumbs = savedBreadcrumbs;
        g_config.behaviors.control.jail = savedJail;
        g_config.behaviors.info.nametag = savedNametag;
        g_config.behaviors.systems.health = savedHealth;
        g_config.behaviors.systems.ai = savedAi;
        g_config.behaviors.systems.pomodoro = savedPomodoro;
    }
private:
    bool savedBall, savedHoncker, savedHats, savedRainbow, savedAcid;
    bool savedBreadcrumbs, savedJail, savedNametag, savedHealth, savedAi, savedPomodoro;
};

TEST_F(AIMCPIntegrationTest, EnableCommandRoutesCorrectly) {
    std::string response;
    bool handled = AI_TryMCPCommand("enable ball", response);
    EXPECT_TRUE(handled);
    EXPECT_NE(response.find("ball"), std::string::npos) << response;
    EXPECT_TRUE(response.find("Enabled") != std::string::npos ||
                response.find("Couldn't enable") != std::string::npos)
        << "Response should indicate success or failure: " << response;
}

TEST_F(AIMCPIntegrationTest, DisableCommandRoutesCorrectly) {
    std::string response;
    bool handled = AI_TryMCPCommand("disable ball", response);
    EXPECT_TRUE(handled);
    EXPECT_NE(response.find("ball"), std::string::npos) << response;
}

TEST_F(AIMCPIntegrationTest, TurnOnOffRoundTrip) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("turn on ball", response));
    EXPECT_NE(response.find("ball"), std::string::npos) << response;
    EXPECT_TRUE(response.find("Turned on") != std::string::npos ||
                response.find("Couldn't enable") != std::string::npos)
        << response;
    EXPECT_TRUE(AI_TryMCPCommand("turn off ball", response));
    EXPECT_NE(response.find("ball"), std::string::npos) << response;
}

TEST_F(AIMCPIntegrationTest, EnableMultipleBehaviors) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("enable hats", response));
    EXPECT_NE(response.find("hats"), std::string::npos) << response;
    EXPECT_TRUE(AI_TryMCPCommand("enable rainbow", response));
    EXPECT_NE(response.find("rainbow"), std::string::npos) << response;
}

TEST_F(AIMCPIntegrationTest, EnableNonexistentBehavior) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("enable nonexistent_behavior_xyz", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPEdgeCase, EmptyInput) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("", response));
    EXPECT_FALSE(AI_TryMCPCommand("   ", response));
}

TEST(AIMCPEdgeCase, CaseInsensitiveEnable) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("ENABLE BALL", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPEdgeCase, CaseInsensitiveHonk) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("HONK", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPEdgeCase, EnableWithExtraWords) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("please enable the ball behavior", response));
}

TEST(AIMCPEdgeCase, SpawnEmptyName) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("spawn", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPEdgeCase, HonkInSentenceNotMatched) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("I want to honk", response));
}

TEST(AIMCPEdgeCase, EnableInSentenceNotMatched) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("you should enable ball", response));
}

TEST(AIMCPEdgeCase, MultipleCommandsNotHandled) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("enable ball and also disable hats", response));
}

TEST(AIMCPEdgeCase, SpecialCharacters) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("enable ball!!!", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPEdgeCase, NumbersInMessage) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("1234 5678", response));
    EXPECT_TRUE(AI_TryMCPCommand("enable 123", response));
}

TEST(AIMCPEdgeCase, HonkCommandDoesNotTriggerHonkKeyword) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("honk", response));
    EXPECT_NE(response.find("HONK"), std::string::npos) << response;
}

TEST(AIMCPEdgeCase, FetchWithUnknownType) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("fetch document", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPEdgeCase, TokenizeDoesNotCrash) {
    auto tokens = AI_TokenizeMessage("a b c d e f g h i j k l m n o p q r s t u v w x y z");
    EXPECT_EQ(tokens.size(), 26u);
}

TEST(AIMCPEdgeCase, MultiTokenBehaviorNames) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("enable breadcrumbs", response));
    EXPECT_NE(response.find("breadcrumbs"), std::string::npos) << response;
}

TEST(AIMCPEdgeCase, NornalSentenceNotCommand) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("tell me a joke", response));
    EXPECT_FALSE(AI_TryMCPCommand("how are you today", response));
    EXPECT_FALSE(AI_TryMCPCommand("what does the goose say", response));
    EXPECT_FALSE(AI_TryMCPCommand("feed the goose", response));
    EXPECT_FALSE(AI_TryMCPCommand("play with goose", response));
}

TEST(AIMCPEdgeCase, PartialCommandPrefixNotEnough) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("en", response));
    EXPECT_FALSE(AI_TryMCPCommand("dis", response));
    EXPECT_FALSE(AI_TryMCPCommand("sp", response));
    EXPECT_FALSE(AI_TryMCPCommand("tur", response));
}

TEST(AIMCPStress, RepeatedEnableDisable) {
    for (int i = 0; i < 10; i++) {
        std::string response;
        EXPECT_TRUE(AI_TryMCPCommand("enable ball", response));
        EXPECT_TRUE(AI_TryMCPCommand("disable ball", response));
    }
}

TEST(AIMCPStress, AlternatingCommands) {
    std::string response;
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(AI_TryMCPCommand("enable ball", response));
        EXPECT_TRUE(AI_TryMCPCommand("honk", response));
        EXPECT_TRUE(AI_TryMCPCommand("disable ball", response));
        EXPECT_TRUE(AI_TryMCPCommand("status", response));
        EXPECT_TRUE(AI_TryMCPCommand("enable ball", response));
    }
}
