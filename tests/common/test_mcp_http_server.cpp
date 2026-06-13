#include <gtest/gtest.h>
#include "mcp_server.h"
#include "config.h"
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>

extern Config g_config;

class McpHttpServerTest : public ::testing::Test {
protected:
    int originalPort;

    void SetUp() override {
        originalPort = g_config.ai.mcpPort;
        g_config.ai.mcpPort = 31073;
    }

    void TearDown() override {
        if (MCP_IsHTTPRunning()) {
            MCP_StopHTTPServer();
        }
        g_config.ai.mcpPort = originalPort;
    }

    std::string SendRequest(const std::string& httpReq) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return {};

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(31073);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            close(fd);
            return {};
        }

        write(fd, httpReq.c_str(), httpReq.size());
        shutdown(fd, SHUT_WR);

        std::string response;
        char buf[4096];
        while (true) {
            int n = (int)read(fd, buf, sizeof(buf));
            if (n <= 0) break;
            response.append(buf, (size_t)n);
        }
        close(fd);
        return response;
    }
};

TEST_F(McpHttpServerTest, StartStopCycle) {
    EXPECT_FALSE(MCP_IsHTTPRunning());
    EXPECT_TRUE(MCP_StartHTTPServer());
    EXPECT_TRUE(MCP_IsHTTPRunning());
    MCP_StopHTTPServer();
    EXPECT_FALSE(MCP_IsHTTPRunning());
}

TEST_F(McpHttpServerTest, StartIdempotent) {
    EXPECT_TRUE(MCP_StartHTTPServer());
    EXPECT_TRUE(MCP_StartHTTPServer());
    EXPECT_TRUE(MCP_IsHTTPRunning());
    MCP_StopHTTPServer();
}

TEST_F(McpHttpServerTest, StopIdempotent) {
    MCP_StopHTTPServer();
    EXPECT_FALSE(MCP_IsHTTPRunning());
}

TEST_F(McpHttpServerTest, PostInitializeReturns200) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    std::string body = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}";
    std::string req =
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;
    std::string resp = SendRequest(req);
    ASSERT_FALSE(resp.empty());
    EXPECT_TRUE(resp.find("HTTP/1.1 200") != std::string::npos)
        << "Expected HTTP 200, got: " << resp.substr(0, 50);
    EXPECT_NE(resp.find("protocolVersion"), std::string::npos);
}

TEST_F(McpHttpServerTest, GetReturns405) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    std::string req =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    std::string resp = SendRequest(req);
    ASSERT_FALSE(resp.empty());
    EXPECT_TRUE(resp.find("HTTP/1.1 405") != std::string::npos)
        << "Expected HTTP 405, got: " << resp.substr(0, 50);
}

TEST_F(McpHttpServerTest, PostEmptyBodyReturns400) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    std::string req =
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    std::string resp = SendRequest(req);
    ASSERT_FALSE(resp.empty());
    EXPECT_TRUE(resp.find("HTTP/1.1 400") != std::string::npos)
        << "Expected HTTP 400, got: " << resp.substr(0, 50);
}

TEST_F(McpHttpServerTest, PostToolsListReturnsTools) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    std::string body = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}";
    std::string req =
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;
    std::string resp = SendRequest(req);
    ASSERT_FALSE(resp.empty());
    EXPECT_TRUE(resp.find("HTTP/1.1 200") != std::string::npos);
    EXPECT_NE(resp.find("spawn_goose"), std::string::npos);
}

TEST_F(McpHttpServerTest, PostNotificationReturnsFallbackResponse) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    std::string body = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    std::string req =
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;
    std::string resp = SendRequest(req);
    ASSERT_FALSE(resp.empty());
    EXPECT_TRUE(resp.find("HTTP/1.1 200") != std::string::npos);
    EXPECT_NE(resp.find("id\":null"), std::string::npos);
}
