#include <gtest/gtest.h>
#include <string>
#include "mcp_server.h"

TEST(MCPProtocol, Initialize) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}");
    EXPECT_NE(resp.find("\"protocolVersion\":\"2024-11-05\""), std::string::npos);
    EXPECT_NE(resp.find("\"id\":1"), std::string::npos);
}

TEST(MCPProtocol, NotificationIgnored) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}");
    EXPECT_TRUE(resp.empty());
}

TEST(MCPProtocol, UnknownMethod) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"foobar\"}");
    EXPECT_NE(resp.find("\"id\":99"), std::string::npos);
    EXPECT_NE(resp.find("\"code\":-32601"), std::string::npos);
}

TEST(MCPProtocol, MissingMethod) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1}");
    EXPECT_NE(resp.find("\"code\":-32600"), std::string::npos);
}

TEST(MCPProtocol, EmptyLine) {
    std::string resp = MCP_HandleRequest("");
    EXPECT_TRUE(resp.empty());
}

TEST(MCPProtocol, MalformedJson) {
    std::string resp = MCP_HandleRequest("{broken");
    EXPECT_NE(resp.find("\"code\":-32600"), std::string::npos);
}

TEST(MCPProtocol, IdCanBeString) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":\"req-1\",\"method\":\"initialize\"}");
    EXPECT_NE(resp.find("\"protocolVersion\":\"2024-11-05\""), std::string::npos);
    EXPECT_NE(resp.find("\"id\":\"req-1\""), std::string::npos);
}

TEST(MCPTools, ListContainsExpectedTools) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    EXPECT_NE(resp.find("spawn_goose"), std::string::npos);
    EXPECT_NE(resp.find("clear_geese"), std::string::npos);
    EXPECT_NE(resp.find("honk"), std::string::npos);
    EXPECT_NE(resp.find("fetch"), std::string::npos);
    EXPECT_NE(resp.find("goose_status"), std::string::npos);
    EXPECT_NE(resp.find("open_preferences"), std::string::npos);
    EXPECT_NE(resp.find("send_chat"), std::string::npos);
    EXPECT_NE(resp.find("enable_behavior"), std::string::npos);
    EXPECT_NE(resp.find("disable_behavior"), std::string::npos);
    EXPECT_NE(resp.find("get_config"), std::string::npos);
    EXPECT_NE(resp.find("set_config"), std::string::npos);
    EXPECT_NE(resp.find("set_hotkey"), std::string::npos);
}

TEST(MCPTools, ToolDescriptionsContainInputSchema) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    EXPECT_NE(resp.find("inputSchema"), std::string::npos);
    EXPECT_NE(resp.find("type"), std::string::npos);
}

TEST(MCPTools, ToolsListIdReflects) {
    std::string resp = MCP_HandleRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"tools/list\"}");
    EXPECT_NE(resp.find("\"id\":42"), std::string::npos);
}
