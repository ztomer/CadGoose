// test_mcp_integration.cpp
// Integration tests: start HTTP server, send real requests, verify responses
#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

#include "mcp_server.h"
#include "config.h"

static std::string SendHTTPRequest(const std::string& body, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "";

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons((uint16_t)port);

    // Retry connect a few times (server may still be starting)
    bool connected = false;
    for (int i = 0; i < 5; i++) {
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            connected = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!connected) { close(sock); return ""; }

    std::string request =
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;

    ssize_t sent = write(sock, request.data(), request.size());
    EXPECT_EQ((size_t)sent, request.size());

    std::string response;
    char buf[4096];
    while (true) {
        ssize_t rc = read(sock, buf, sizeof(buf));
        if (rc <= 0) break;
        response.append(buf, (size_t)rc);
    }
    close(sock);
    return response;
}

class MCPIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        g_config.ai.mcpPort = 31073; // Use non-default port to avoid conflicts
        MCP_StopHTTPServer();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ASSERT_TRUE(MCP_StartHTTPServer());
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Let server bind
    }

    void TearDown() override {
        MCP_StopHTTPServer();
    }

    int Port() const { return g_config.ai.mcpPort; }
};

TEST_F(MCPIntegrationTest, InitializeReturnsProtocolVersion) {
    std::string resp = SendHTTPRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}", Port());
    EXPECT_NE(resp.find("HTTP/1.1 200"), std::string::npos);
    EXPECT_NE(resp.find("protocolVersion"), std::string::npos);
    EXPECT_NE(resp.find("2024-11-05"), std::string::npos);
}

TEST_F(MCPIntegrationTest, ToolsListReturnsAllTools) {
    std::string resp = SendHTTPRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}", Port());
    EXPECT_NE(resp.find("HTTP/1.1 200"), std::string::npos);
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

TEST_F(MCPIntegrationTest, GetConfigReturnsJson) {
    std::string resp = SendHTTPRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"get_config\",\"arguments\":{}}}", Port());
    EXPECT_NE(resp.find("HTTP/1.1 200"), std::string::npos);
    EXPECT_NE(resp.find("fun"), std::string::npos);
    EXPECT_NE(resp.find("control"), std::string::npos);
}

TEST_F(MCPIntegrationTest, SetConfigTogglesBehavior) {
    // Enable ball
    std::string resp1 = SendHTTPRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"set_config\",\"arguments\":{\"key\":\"behaviors.fun.ball\",\"value\":\"true\"}}}", Port());
    EXPECT_NE(resp1.find("HTTP/1.1 200"), std::string::npos);
    EXPECT_TRUE(g_config.behaviors.fun.ball);

    // Disable ball
    std::string resp2 = SendHTTPRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"set_config\",\"arguments\":{\"key\":\"behaviors.fun.ball\",\"value\":\"false\"}}}", Port());
    EXPECT_NE(resp2.find("HTTP/1.1 200"), std::string::npos);
    EXPECT_FALSE(g_config.behaviors.fun.ball);
}

TEST_F(MCPIntegrationTest, SetConfigUnknownKeyReturnsError) {
    std::string resp = SendHTTPRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"set_config\",\"arguments\":{\"key\":\"behaviors.fake.key\",\"value\":\"true\"}}}", Port());
    EXPECT_NE(resp.find("HTTP/1.1 200"), std::string::npos);
    EXPECT_NE(resp.find("unknown config key"), std::string::npos);
}

TEST_F(MCPIntegrationTest, GetReturnsMethodNotAllowed) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { FAIL() << "socket() failed"; return; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons((uint16_t)Port());

    bool connected = false;
    for (int i = 0; i < 5; i++) {
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            connected = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!connected) { close(sock); FAIL() << "connect() failed"; return; }

    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    write(sock, request.data(), request.size());
    std::string response;
    char buf[4096];
    while (true) {
        ssize_t rc = read(sock, buf, sizeof(buf));
        if (rc <= 0) break;
        response.append(buf, (size_t)rc);
    }
    close(sock);

    EXPECT_NE(response.find("HTTP/1.1 405"), std::string::npos);
}
