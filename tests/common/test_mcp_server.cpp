#include <gtest/gtest.h>
#include "mcp_server.h"
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

// A socket path this process owns: the default /tmp/desktop-goose-mcp.sock
// belongs to a running CadGoose, which these tests used to unlink, and to
// any concurrent test binary (ROADMAP G0.3). Short, for sun_path's 104 bytes.
static const std::string kTestSockPath =
    "/tmp/cadgoose-mcp-test-" + std::to_string(getpid()) + ".sock";
#define TEST_MCP_SOCK_PATH kTestSockPath.c_str()

class McpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_savedPath = MCP_SetInternalSocketPath(kTestSockPath);
        unlink(TEST_MCP_SOCK_PATH);
    }

    void TearDown() override {
        if (MCP_IsInternalRunning()) {
            MCP_StopInternalServer();
        }
        unlink(TEST_MCP_SOCK_PATH);
        MCP_SetInternalSocketPath(m_savedPath);
    }

    std::string m_savedPath;

    std::string SendRequest(const std::string& json) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) return {};

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, TEST_MCP_SOCK_PATH, sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            close(fd);
            return {};
        }

        std::string msg = json + "\n";
        write(fd, msg.c_str(), msg.size());
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

TEST_F(McpServerTest, StartStopCycle) {
    EXPECT_FALSE(MCP_IsInternalRunning());
    EXPECT_TRUE(MCP_StartInternalServer());
    EXPECT_TRUE(MCP_IsInternalRunning());
    MCP_StopInternalServer();
    EXPECT_FALSE(MCP_IsInternalRunning());
}

// Stop used to close the listening fd while the server thread sat in accept()
// on it, and Darwin's close() can then wait on that accept forever: the suite
// hung in McpServerTest's TearDown on iteration 18 of --gtest_repeat=20
// (ROADMAP G0.13). A hang cannot be asserted from the hung thread, so the
// cycles run on a worker under a deadline.
TEST_F(McpServerTest, StartStopNeverHangs) {
    constexpr int kCycles = 2000;
    std::atomic<int> cycles{0};
    std::atomic<bool> done{false};
    std::thread worker([&] {
        for (int i = 0; i < kCycles; ++i) {
            if (!MCP_StartInternalServer()) break;
            // A request first, as the hung test did: the thread is then
            // back in accept() for the second time when Stop arrives.
            if (i % 2 == 0) SendRequest("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}");
            MCP_StopInternalServer();
            ++cycles;
        }
        done = true;
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (!done && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!done) {
        worker.detach();  // stuck in close(); nothing can join it
        FAIL() << "start/stop hung after " << cycles.load() << " cycles";
    }
    worker.join();
    EXPECT_EQ(cycles.load(), kCycles);
}

TEST_F(McpServerTest, StartIdempotent) {
    EXPECT_TRUE(MCP_StartInternalServer());
    EXPECT_TRUE(MCP_StartInternalServer());
    EXPECT_TRUE(MCP_IsInternalRunning());
    MCP_StopInternalServer();
}

TEST_F(McpServerTest, StopIdempotent) {
    MCP_StopInternalServer();
    EXPECT_FALSE(MCP_IsInternalRunning());
}

TEST_F(McpServerTest, InitializeViaSocket) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string resp = SendRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}");
    ASSERT_FALSE(resp.empty());
    EXPECT_NE(resp.find("\"jsonrpc\":\"2.0\""), std::string::npos);
    EXPECT_NE(resp.find("\"id\":1"), std::string::npos);
    EXPECT_NE(resp.find("protocolVersion"), std::string::npos);
}

TEST_F(McpServerTest, ToolsListViaSocket) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string resp = SendRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}");
    ASSERT_FALSE(resp.empty());
    EXPECT_NE(resp.find("\"id\":2"), std::string::npos);
    EXPECT_NE(resp.find("spawn_goose"), std::string::npos);
}

TEST_F(McpServerTest, ResourcesListViaSocket) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string resp = SendRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"resources/list\"}");
    ASSERT_FALSE(resp.empty());
    EXPECT_NE(resp.find("\"id\":3"), std::string::npos);
    EXPECT_NE(resp.find("config://"), std::string::npos);
}

TEST_F(McpServerTest, ToolsCallSpawnGooseViaSocket) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string resp = SendRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":4,\"method\":\"tools/call\","
        "\"params\":{\"name\":\"spawn_goose\"}}");
    ASSERT_FALSE(resp.empty());
    EXPECT_NE(resp.find("\"id\":4"), std::string::npos);
}

TEST_F(McpServerTest, UnknownMethodViaSocket) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string resp = SendRequest(
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"nonexistent\"}");
    ASSERT_FALSE(resp.empty());
    EXPECT_NE(resp.find("Method not found"), std::string::npos);
}

TEST_F(McpServerTest, EmptyLineReturnsEmpty) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string resp = SendRequest("");
    EXPECT_TRUE(resp.empty());
}

TEST_F(McpServerTest, NotificationsInitializedReturnsEmpty) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string resp = SendRequest(
        "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}");
    EXPECT_TRUE(resp.empty());
}

TEST_F(McpServerTest, RequestTooLarge) {
    ASSERT_TRUE(MCP_StartInternalServer());
    std::string big(66000, 'A');
    std::string json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"" + big + "\"}";
    std::string resp = SendRequest(json);
    EXPECT_TRUE(resp.empty());
}

// Start reports failure, and leaves nothing running, when its socket path
// cannot be bound.
TEST_F(McpServerTest, APathTooLongForAUnixSocketIsRefused) {
    MCP_SetInternalSocketPath(kTestSockPath + std::string(200, 'x'));
    EXPECT_FALSE(MCP_StartInternalServer());
    EXPECT_FALSE(MCP_IsInternalRunning());
}

TEST_F(McpServerTest, AMissingDirectoryFailsTheBind) {
    MCP_SetInternalSocketPath(kTestSockPath + "-no-such-dir/s");
    EXPECT_FALSE(MCP_StartInternalServer());
    EXPECT_FALSE(MCP_IsInternalRunning());
}
