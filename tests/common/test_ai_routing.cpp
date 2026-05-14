#include <gtest/gtest.h>
#include <string>
#include "ai_mcp_bridge.h"

TEST(AIMCPRouting, UnknownMessageReturnsFalse) {
    std::string response;
    EXPECT_FALSE(AI_TryMCPCommand("hello how are you", response));
    EXPECT_FALSE(AI_TryMCPCommand("what is the weather", response));
    EXPECT_FALSE(AI_TryMCPCommand("i like geese", response));
    EXPECT_FALSE(AI_TryMCPCommand("", response));
}

TEST(AIMCPRouting, EnableCommandRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("enable ball", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, EnableWithPunctuation) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("enable ball!", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, TurnOnRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("turn on ball", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, DisableCommandRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("disable ball", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, TurnOffRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("turn off ball", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, HonkCommandRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("honk", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, HonkWithPunctuation) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("HONK!", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, SpawnCommandRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("spawn goose", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, SpawnNamedGoose) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("spawn Gerald", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, SpawnNamedGooseLong) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("spawn a goose named Gerald", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, ClearGeeseRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("clear geese", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, ClearAllGeese) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("clear all geese", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, RemoveGeese) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("remove geese", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, StatusRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("status", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, GooseStatus) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("goose status", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, GetStatus) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("get status", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, ReportStatus) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("report", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, OpenPreferences) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("open preferences", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, ShowPreferences) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("show preferences", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, OpenSettings) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("open settings", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, OpenConfig) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("open config", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, OpenPrefs) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("open prefs", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, FetchRecognized) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("fetch", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, FetchMeme) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("fetch meme", response));
    EXPECT_FALSE(response.empty());
}

TEST(AIMCPRouting, FetchText) {
    std::string response;
    EXPECT_TRUE(AI_TryMCPCommand("fetch text", response));
    EXPECT_FALSE(response.empty());
}
