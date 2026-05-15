#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "mcp_server.h"
#include "config.h"

TEST(MCPResources, ListReturnsAllUris) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/list\"}");
    EXPECT_NE(resp.find("config://behaviors"), std::string::npos);
    EXPECT_NE(resp.find("config://behaviors/fun"), std::string::npos);
    EXPECT_NE(resp.find("config://behaviors/control"), std::string::npos);
    EXPECT_NE(resp.find("config://behaviors/info"), std::string::npos);
    EXPECT_NE(resp.find("config://behaviors/systems"), std::string::npos);
}

TEST(MCPResources, ListHasMimeType) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/list\"}");
    EXPECT_NE(resp.find("mimeType"), std::string::npos);
    EXPECT_NE(resp.find("application/json"), std::string::npos);
}

TEST(MCPResources, ReadBehaviors) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"config://behaviors\"}}");
    EXPECT_NE(resp.find("\"id\":1"), std::string::npos);
    EXPECT_NE(resp.find("ball"), std::string::npos);
    EXPECT_NE(resp.find("honcker"), std::string::npos);
    EXPECT_NE(resp.find("nametag"), std::string::npos);
    EXPECT_NE(resp.find("health"), std::string::npos);
    EXPECT_NE(resp.find("honcker_hotkey"), std::string::npos);
}

TEST(MCPResources, ReadFun) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"config://behaviors/fun\"}}");
    EXPECT_NE(resp.find("ball"), std::string::npos);
    EXPECT_NE(resp.find("breadCrumbs"), std::string::npos);
    EXPECT_NE(resp.find("hats"), std::string::npos);
    EXPECT_NE(resp.find("rainbow"), std::string::npos);
    EXPECT_NE(resp.find("acid"), std::string::npos);
    EXPECT_NE(resp.find("anger"), std::string::npos);
    EXPECT_EQ(resp.find("honcker"), std::string::npos);
}

TEST(MCPResources, ReadControl) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"config://behaviors/control\"}}");
    EXPECT_NE(resp.find("honcker"), std::string::npos);
    EXPECT_NE(resp.find("jail"), std::string::npos);
    EXPECT_NE(resp.find("portals"), std::string::npos);
    EXPECT_NE(resp.find("drag"), std::string::npos);
    EXPECT_EQ(resp.find("ball"), std::string::npos);
}

TEST(MCPResources, ReadInfo) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"config://behaviors/info\"}}");
    EXPECT_NE(resp.find("nametag"), std::string::npos);
    EXPECT_NE(resp.find("presence"), std::string::npos);
    EXPECT_NE(resp.find("configGUI"), std::string::npos);
    EXPECT_EQ(resp.find("health"), std::string::npos);
}

TEST(MCPResources, ReadSystems) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"config://behaviors/systems\"}}");
    EXPECT_NE(resp.find("health"), std::string::npos);
    EXPECT_NE(resp.find("ai"), std::string::npos);
    EXPECT_NE(resp.find("pomodoro"), std::string::npos);
    EXPECT_EQ(resp.find("ball"), std::string::npos);
}

TEST(MCPResources, ReadUnknown) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/read\","
        "\"params\":{\"uri\":\"config://nonexistent\"}}");
    EXPECT_NE(resp.find("\"code\":-32602"), std::string::npos);
}

TEST(MCPResources, ReadMissingUri) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/read\","
        "\"params\":{}}");
    EXPECT_NE(resp.find("\"code\":-32602"), std::string::npos);
}

TEST(MCPDirect, CallToolWrapsResponse) {
    std::string resp = MCP_CallTool("get_config", "{}");
    EXPECT_NE(resp.find("\"type\":\"text\""), std::string::npos);
    EXPECT_NE(resp.find("fun"), std::string::npos);
}

TEST(MCPDirect, UnknownTool) {
    std::string resp = MCP_CallTool("nonexistent_tool", "{}");
    EXPECT_NE(resp.find("error"), std::string::npos);
    EXPECT_NE(resp.find("unknown"), std::string::npos);
}

TEST(MCPDirect, ToolNameInResponse) {
    std::string resp = MCP_CallTool("spawn_goose", "{}");
    EXPECT_TRUE(resp.find("error") != std::string::npos ||
                resp.find("ok") != std::string::npos);
}

TEST(MCPDirect, EnableDisableBehavior) {
    std::string resp = MCP_CallTool("enable_behavior", "{\"id\":\"ball\"}");
    EXPECT_TRUE(resp.find("error") != std::string::npos ||
                resp.find("ok") != std::string::npos);
    resp = MCP_CallTool("disable_behavior", "{\"id\":\"ball\"}");
    EXPECT_TRUE(resp.find("error") != std::string::npos ||
                resp.find("ok") != std::string::npos);
}

TEST(MCPEdgeCase, JsonWithExtraFields) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\","
        "\"extra\":\"ignored\"}");
    EXPECT_NE(resp.find("\"protocolVersion\":\"2024-11-05\""), std::string::npos);
}

TEST(MCPEdgeCase, StringIdWithSpecialChars) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":\"test-id-123\",\"method\":\"initialize\"}");
    EXPECT_NE(resp.find("\"protocolVersion\":\"2024-11-05\""), std::string::npos);
    EXPECT_NE(resp.find("\"id\":\"test-id-123\""), std::string::npos);
}

TEST(MCPEdgeCase, BooleanId) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":true,\"method\":\"initialize\"}");
    EXPECT_NE(resp.find("\"protocolVersion\":\"2024-11-05\""), std::string::npos);
}

TEST(MCPEdgeCase, NumericIdZero) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"initialize\"}");
    EXPECT_NE(resp.find("\"protocolVersion\":\"2024-11-05\""), std::string::npos);
    EXPECT_NE(resp.find("\"id\":0"), std::string::npos);
}

TEST(MCPEdgeCase, SetAllTogglesThenVerifyGetConfig) {
    std::vector<bool*> toggles = {
        &g_config.behaviors.fun.ball,
        &g_config.behaviors.fun.hats,
        &g_config.behaviors.control.honcker,
        &g_config.behaviors.info.nametag,
        &g_config.behaviors.systems.health
    };
    for (auto* ptr : toggles) {
        bool orig = *ptr;
        *ptr = true;
        std::string resp = MCP_CallTool("get_config", "{}");
        EXPECT_NE(resp.find("true"), std::string::npos)
            << "get_config should contain 'true' for enabled toggle";
        *ptr = orig;
    }
}

TEST(MCPEdgeCase, ToolsCallViaHandleRequestMissingName) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\","
        "\"params\":{}}");
    EXPECT_NE(resp.find("-32602"), std::string::npos);
}

TEST(MCPEdgeCase, ToolsCallViaHandleRequestUnknownTool) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"bogus_tool\"}}");
    EXPECT_NE(resp.find("unknown"), std::string::npos);
}

TEST(MCPEdgeCase, CanCallMCPHandleRequestReentrant) {
    std::string resp1 = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    std::string resp2 = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/list\"}");
    EXPECT_NE(resp1.find("spawn_goose"), std::string::npos);
    EXPECT_NE(resp2.find("config://behaviors"), std::string::npos);
}

TEST(MCPEdgeCase, AllDefaultHotkeysAppearInGetConfig) {
    std::string resp = MCP_CallTool("get_config", "{}");
    for (const auto& val : {"f", "o", "p", "1", "2", "0"}) {
        EXPECT_NE(resp.find(R"(\")" + std::string(val) + R"(\")"),
                  std::string::npos)
            << "default hotkey value \"" << val << "\" should appear in config";
    }
    EXPECT_NE(resp.find("right shift"), std::string::npos);
}

TEST(MCPEdgeCase, SetAndGetHotkeyRoundTrip) {
    std::string orig = g_config.behaviors.jail.hotkeyO;
    std::string resp = MCP_CallTool("set_hotkey",
        "{\"hotkey\":\"jail_hotkey_o\",\"value\":\"cmd+shift+j\"}");
    EXPECT_NE(resp.find("ok"), std::string::npos);
    EXPECT_EQ(g_config.behaviors.jail.hotkeyO, "cmd+shift+j");
    resp = MCP_CallTool("get_config", "{}");
    EXPECT_NE(resp.find("cmd+shift+j"), std::string::npos);
    g_config.behaviors.jail.hotkeyO = orig;
}

TEST(MCPEdgeCase, MismatchedToolName) {
    std::string resp = MCP_CallTool("tools/list", "");
    EXPECT_NE(resp.find("error"), std::string::npos);
    EXPECT_NE(resp.find("unknown"), std::string::npos);
}

TEST(MCPEdgeCase, LargeHotkeyValue) {
    std::string orig = g_config.behaviors.honcker.hotkey;
    std::string longKey = "ctrl+alt+cmd+shift+fn+space";
    MCP_CallTool("set_hotkey",
        "{\"hotkey\":\"honcker_hotkey\",\"value\":\"" + longKey + "\"}");
    EXPECT_EQ(g_config.behaviors.honcker.hotkey, longKey);
    g_config.behaviors.honcker.hotkey = orig;
}

TEST(MCPEdgeCase, GetConfigResponseStartsWithContentWrapper) {
    std::string resp = MCP_CallTool("get_config", "{}");
    EXPECT_NE(resp.find(R"("content")"), std::string::npos);
    EXPECT_NE(resp.find(R"("type":"text")"), std::string::npos);
}
