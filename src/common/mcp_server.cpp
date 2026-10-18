#include "mcp_server.h"
#include "command_socket.h"
#include "config.h"
#include "accept_loop.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static constexpr const char* kDefaultMcpSocketPath = "/tmp/desktop-goose-mcp.sock";
static std::mutex g_socketPathMutex;
static std::string g_socketPath = kDefaultMcpSocketPath;

std::string MCP_InternalSocketPath() {
    std::lock_guard<std::mutex> lock(g_socketPathMutex);
    return g_socketPath;
}

std::string MCP_SetInternalSocketPath(std::string path) {
    std::lock_guard<std::mutex> lock(g_socketPathMutex);
    if (path.empty()) path = kDefaultMcpSocketPath;
    std::swap(g_socketPath, path);
    return path;
}

// JSON-RPC utilities (defined in mcp_json_rpc.cpp)
std::string JsonEscape(const std::string& s);
std::string MakeJsonResponse(const std::string& id, const std::string& resultJson);
std::string MakeJsonError(const std::string& id, int code, const std::string& message);
std::string ExtractMethod(const std::string& json);
std::string ExtractId(const std::string& json);
std::string ExtractArg(const std::string& json, const std::string& key);

// Handler functions (defined in mcp_handlers.cpp)
std::string HandleInitialize(const std::string& requestedVersion = "");
std::string HandleToolsList();
std::string HandleResourcesList();
std::string HandleResourcesRead(const std::string& uri);
std::string ExecuteTool(const std::string& name, const std::string& argsJson);

// The internal server: one socket, one thread (include/accept_loop.h).
static AcceptLoop g_server;
static std::string g_serverSocketPath;  // the path bound, for the unlink at Stop

static bool WriteAll(int fd, const std::string& data) {
    size_t written = 0;
    while (written < data.size()) {
        ssize_t rc = write(fd, data.data() + written, data.size() - written);
        if (rc <= 0) return false;
        written += (size_t)rc;
    }
    return true;
}

static constexpr size_t kMaxRequestSize = 64 * 1024; // 64KB max request

static void HandleConnection(int clientFd) {
    std::string data;
    char buffer[4096];
    while (true) {
        ssize_t rc = read(clientFd, buffer, sizeof(buffer) - 1);
        if (rc == 0) break;
        if (rc < 0) {
            if (errno == EINTR) continue;
            break;
        }
        data.append(buffer, (size_t)rc);
        if (data.size() > kMaxRequestSize) {
            fprintf(stderr, "[MCP] Request too large (%zu bytes), closing connection\n", data.size());
            close(clientFd);
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
        // SEP-2575 negotiation: echo a supported version, else answer latest.
        // Extra params (e.g. _meta, clientInfo) are ignored.
        std::string requested = ExtractArg(line, "protocolVersion");
        return MakeJsonResponse(id, HandleInitialize(requested));
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
    if (g_server.Running()) return true;

    const std::string socketPath = MCP_InternalSocketPath();

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    if (socketPath.size() >= sizeof(addr.sun_path)) {
        fprintf(stderr, "[MCP] Socket path too long: %zu\n", socketPath.size());
        return false;
    }

    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "[MCP] Failed to create socket\n");
        return false;
    }

    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, socketPath.c_str(), socketPath.size() + 1);

    unlink(socketPath.c_str());
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "[MCP] Failed to bind socket\n");
        close(fd);
        return false;
    }

    if (listen(fd, 8) != 0) {
        fprintf(stderr, "[MCP] Failed to listen on socket\n");
        close(fd);
        unlink(socketPath.c_str());
        return false;
    }

    if (!g_server.Start(fd, [](int clientFd) {
            HandleConnection(clientFd);
            close(clientFd);
        })) {
        fprintf(stderr, "[MCP] Failed to start the internal server thread\n");
        unlink(socketPath.c_str());
        return false;
    }
    g_serverSocketPath = socketPath;
    fprintf(stderr, "[MCP] Internal server started on %s\n", socketPath.c_str());
    return true;
}

void MCP_StopInternalServer() {
    if (!g_server.Running()) return;
    g_server.Stop();
    unlink(g_serverSocketPath.c_str());
    fprintf(stderr, "[MCP] Internal server stopped\n");
}

bool MCP_IsInternalRunning() {
    return g_server.Running();
}

int MCP_RunStdioServer() {
    fprintf(stderr, "[MCP] Starting stdio MCP server\n");
    std::string buffer;
    while (std::getline(std::cin, buffer)) {
        if (!buffer.empty()) {
            std::string response = MCP_HandleRequest(buffer);
            if (!response.empty()) {
                std::string out = response + "\n";
                WriteAll(STDOUT_FILENO, out);
                fflush(stdout);
            }
        }
    }
    return 0;
}
