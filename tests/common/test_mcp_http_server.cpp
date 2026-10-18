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
        // Port 0: the kernel picks a free port, read back via MCP_HTTPPort(),
        // so concurrent test binaries never collide (ROADMAP G0.3).
        g_config.ai.mcpPort = 0;
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
        addr.sin_port = htons((uint16_t)MCP_HTTPPort());
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

TEST_F(McpHttpServerTest, PostLargeBodyTriggersContentRead) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    std::string body(5000, 'X');
    std::string req =
        "POST / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" + body;
    std::string resp = SendRequest(req);
    ASSERT_FALSE(resp.empty());
    EXPECT_TRUE(resp.find("HTTP/1.1 200") != std::string::npos);
}

// ── What the server refuses ──────────────────────────────────────────────
// A request over the 1 MB cap, however it is over, is dropped: the
// connection closes with no response, and the server keeps serving.

namespace {

// Sends everything it can (the server may close mid-stream, which must not
// raise SIGPIPE in the test process), then reads whatever comes back.
std::string SendRaw(int port, const std::string& bytes) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return "<no socket>";
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return "<no connection>";
    }
    size_t sent = 0;
    while (sent < bytes.size()) {
        const ssize_t n = write(fd, bytes.data() + sent, bytes.size() - sent);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
    }
    shutdown(fd, SHUT_WR);
    std::string response;
    char buf[4096];
    for (ssize_t n; (n = read(fd, buf, sizeof(buf))) > 0;) response.append(buf, static_cast<size_t>(n));
    close(fd);
    return response;
}

constexpr size_t kCap = 1024 * 1024;  // mcp_http_server.cpp kHTTPMaxSize

}  // namespace

TEST_F(McpHttpServerTest, HeadersPastTheCapAreDropped) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    EXPECT_EQ(SendRaw(MCP_HTTPPort(), std::string(kCap + 8192, 'a')), "");
    EXPECT_NE(SendRequest("GET / HTTP/1.1\r\n\r\n"), "") << "the server must keep serving";
}

TEST_F(McpHttpServerTest, AContentLengthPastTheCapIsRefused) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    const std::string req =
        "POST / HTTP/1.1\r\nContent-Length: " + std::to_string(kCap + 1) + "\r\n\r\n{}";
    EXPECT_EQ(SendRaw(MCP_HTTPPort(), req), "");
}

TEST_F(McpHttpServerTest, ABodyThatTakesTheRequestPastTheCapIsDropped) {
    ASSERT_TRUE(MCP_StartHTTPServer());
    // The length itself is allowed; headers plus that body are not.
    const std::string req = "POST / HTTP/1.1\r\nContent-Length: " + std::to_string(kCap) +
                            "\r\n\r\n" + std::string(kCap, ' ');
    EXPECT_EQ(SendRaw(MCP_HTTPPort(), req), "");
}

TEST_F(McpHttpServerTest, APortAlreadyInUseFailsToStart) {
    const int holder = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(holder, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof(addr);
    ASSERT_EQ(bind(holder, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), 0);
    ASSERT_EQ(listen(holder, 1), 0);
    ASSERT_EQ(getsockname(holder, reinterpret_cast<sockaddr*>(&addr), &len), 0);

    g_config.ai.mcpPort = ntohs(addr.sin_port);  // the holder's, kernel-chosen
    EXPECT_FALSE(MCP_StartHTTPServer());
    EXPECT_FALSE(MCP_IsHTTPRunning());
    EXPECT_EQ(MCP_HTTPPort(), 0);
    close(holder);
}
