#include "mcp_server.h"
#include "command_socket.h"
#include "config.h"
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define MCP_SOCKET_PATH "/tmp/desktop-goose-mcp.sock"

// JSON-RPC utilities (defined in mcp_json_rpc.cpp)
std::string JsonEscape(const std::string& s);
std::string MakeJsonResponse(const std::string& id, const std::string& resultJson);
std::string MakeJsonError(const std::string& id, int code, const std::string& message);
std::string ExtractMethod(const std::string& json);
std::string ExtractId(const std::string& json);
std::string ExtractArg(const std::string& json, const std::string& key);

// Handler functions (defined in mcp_handlers.cpp)
std::string HandleInitialize();
std::string HandleToolsList();
std::string HandleResourcesList();
std::string HandleResourcesRead(const std::string& uri);
std::string ExecuteTool(const std::string& name, const std::string& argsJson);

// Server globals
static std::thread g_serverThread;
static std::atomic<bool> g_serverRunning{false};
static int g_serverFd = -1;

static void WriteAll(int fd, const std::string& data) {
    size_t written = 0;
    while (written < data.size()) {
        ssize_t rc = write(fd, data.data() + written, data.size() - written);
        if (rc <= 0) return;
        written += (size_t)rc;
    }
}

static constexpr size_t kMaxRequestSize = 64 * 1024; // 64KB max request

static void HandleConnection(int clientFd) {
    std::string data;
    char buffer[4096];
    while (true) {
        ssize_t rc = read(clientFd, buffer, sizeof(buffer) - 1);
        if (rc <= 0) break;
        buffer[rc] = '\0';
        data += buffer;
        if (data.size() > kMaxRequestSize) {
            fprintf(stderr, "[MCP] Request too large (%zu bytes), closing connection\n", data.size());
            return;
        }
        if (data.find('\n') != std::string::npos) break;
    }

    size_t pos = 0;
    while (pos < data.size()) {
        auto nl = data.find('\n', pos);
        if (nl == std::string::npos) break;
        std::string line = data.substr(pos, nl - pos);
        pos = nl + 1;
        std::string response = MCP_HandleRequest(line);
        if (!response.empty()) {
            WriteAll(clientFd, response);
        }
    }
}

std::string MCP_CallTool(const std::string& name, const std::string& argsJson) {
    std::string result = ExecuteTool(name, argsJson);
    return "{\"content\":[{\"type\":\"text\",\"text\":\"" + JsonEscape(result) + "\"}]}";
}

std::string MCP_HandleRequest(const std::string& line) {
    if (line.empty()) return "";

    std::string method = ExtractMethod(line);
    std::string id = ExtractId(line);

    if (method.empty()) {
        return MakeJsonError(id, -32600, "Invalid Request: missing method");
    }

    if (method == "initialize") {
        return MakeJsonResponse(id, HandleInitialize());
    }

    if (method == "tools/list") {
        return MakeJsonResponse(id, HandleToolsList());
    }

    if (method == "tools/call") {
        std::string name = ExtractArg(line, "name");
        if (name.empty()) {
            return MakeJsonError(id, -32602, "Invalid params: missing tool name");
        }
        std::string argsJson;
        auto pos = line.find("\"arguments\"");
        if (pos != std::string::npos) {
            pos = line.find('{', pos + 10);
            if (pos != std::string::npos) {
                int depth = 0;
                for (size_t i = pos; i < line.size(); i++) {
                    if (line[i] == '{') depth++;
                    else if (line[i] == '}') { depth--; if (depth == 0) { argsJson = line.substr(pos, i - pos + 1); break; } }
                }
            }
        }
        std::string result = ExecuteTool(name, argsJson);
        return MakeJsonResponse(id, "{\"content\":[{\"type\":\"text\",\"text\":\"" + JsonEscape(result) + "\"}]}");
    }

    if (method == "resources/list") {
        return MakeJsonResponse(id, HandleResourcesList());
    }

    if (method == "resources/read") {
        std::string uri = ExtractArg(line, "uri");
        if (uri.empty()) {
            return MakeJsonError(id, -32602, "Invalid params: missing resource URI");
        }
        std::string result = HandleResourcesRead(uri);
        if (result.empty()) {
            return MakeJsonError(id, -32602, "Resource not found: " + uri);
        }
        return MakeJsonResponse(id, result);
    }

    if (method == "notifications/initialized") {
        return "";
    }

    return MakeJsonError(id, -32601, "Method not found: " + method);
}

bool MCP_StartInternalServer() {
    if (g_serverRunning.load()) return true;

    const std::string socketPath = MCP_SOCKET_PATH;

    g_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_serverFd < 0) {
        fprintf(stderr, "[MCP] Failed to create socket\n");
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    unlink(socketPath.c_str());
    if (bind(g_serverFd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "[MCP] Failed to bind socket\n");
        close(g_serverFd);
        g_serverFd = -1;
        return false;
    }

    if (listen(g_serverFd, 8) != 0) {
        fprintf(stderr, "[MCP] Failed to listen on socket\n");
        close(g_serverFd);
        g_serverFd = -1;
        unlink(socketPath.c_str());
        return false;
    }

    g_serverRunning.store(true);

    g_serverThread = std::thread([]() {
        fprintf(stderr, "[MCP] Internal server started on %s\n", MCP_SOCKET_PATH);
        while (g_serverRunning.load()) {
            struct sockaddr_un clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = accept(g_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientFd < 0) {
                if (!g_serverRunning.load()) break;
                continue;
            }
            HandleConnection(clientFd);
            close(clientFd);
        }
        unlink(MCP_SOCKET_PATH);
    });

    return true;
}

void MCP_StopInternalServer() {
    if (!g_serverRunning.exchange(false)) return;

    if (g_serverFd >= 0) {
        close(g_serverFd);
        g_serverFd = -1;
    }

    if (g_serverThread.joinable()) {
        g_serverThread.join();
    }

    fprintf(stderr, "[MCP] Internal server stopped\n");
}

bool MCP_IsInternalRunning() {
    return g_serverRunning.load();
}

int MCP_RunStdioServer() {
    fprintf(stderr, "[MCP] Starting stdio MCP server\n");
    std::string buffer;
    while (true) {
        char c;
        ssize_t rc = read(STDIN_FILENO, &c, 1);
        if (rc <= 0) break;

        if (c == '\n') {
            if (!buffer.empty()) {
                std::string response = MCP_HandleRequest(buffer);
                if (!response.empty()) {
                    WriteAll(STDOUT_FILENO, response);
                    fflush(stdout);
                }
                buffer.clear();
            }
        } else {
            buffer += c;
        }
    }
    return 0;
}
