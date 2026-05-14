#include "mcp_server.h"
#include "config.h"
#include <thread>
#include <atomic>
#include <string>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static std::thread g_httpThread;
static std::atomic<bool> g_httpRunning{false};
static int g_httpFd = -1;

static std::string ReadHTTP(int fd) {
    std::string data;
    char buf[4096];
    // Read until we have at least the headers (containing \r\n\r\n)
    while (data.find("\r\n\r\n") == std::string::npos) {
        ssize_t rc = read(fd, buf, sizeof(buf));
        if (rc <= 0) return data;
        data.append(buf, (size_t)rc);
    }
    // Parse Content-Length and read the body
    auto clPos = data.find("Content-Length:");
    if (clPos != std::string::npos) {
        auto valStart = data.find_first_of("0123456789", clPos);
        auto valEnd = data.find_first_not_of("0123456789", valStart);
        if (valStart != std::string::npos) {
            int cl = std::stoi(data.substr(valStart, valEnd - valStart));
            size_t bodyStart = data.find("\r\n\r\n") + 4;
            size_t needed = bodyStart + (size_t)cl;
            while (data.size() < needed) {
                ssize_t n = read(fd, buf, sizeof(buf));
                if (n <= 0) break;
                data.append(buf, (size_t)n);
            }
        }
    }
    return data;
}

static void HandleHTTPConnection(int clientFd) {
    std::string raw = ReadHTTP(clientFd);
    if (raw.empty()) { close(clientFd); return; }

    auto firstLine = raw.find("\r\n");
    if (firstLine == std::string::npos) { close(clientFd); return; }
    std::string requestLine = raw.substr(0, firstLine);

    bool isPost = (requestLine.find("POST") == 0);

    auto bodyStart = raw.find("\r\n\r\n");
    if (bodyStart == std::string::npos) { close(clientFd); return; }
    std::string body = raw.substr(bodyStart + 4);

    std::string responseBody;
    int httpStatus = 200;

    if (isPost && !body.empty()) {
        responseBody = MCP_HandleRequest(body);
        if (responseBody.empty()) {
            responseBody = "{\"jsonrpc\":\"2.0\",\"id\":null,\"result\":{}}\n";
        }
    } else {
        httpStatus = isPost ? 400 : 405;
        responseBody = "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":-32000,\"message\":\"Method not allowed\"}}\n";
    }

    std::string httpResponse =
        "HTTP/1.1 " + std::to_string(httpStatus) + " " + (httpStatus == 200 ? "OK" : httpStatus == 400 ? "Bad Request" : "Method Not Allowed") + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(responseBody.size()) + "\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n" +
        responseBody;

    size_t written = 0;
    while (written < httpResponse.size()) {
        ssize_t rc = write(clientFd, httpResponse.data() + written, httpResponse.size() - written);
        if (rc <= 0) break;
        written += (size_t)rc;
    }
    close(clientFd);
}

bool MCP_StartHTTPServer() {
    if (g_httpRunning.load()) return true;

    g_httpFd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_httpFd < 0) {
        fprintf(stderr, "[MCP-HTTP] Failed to create socket\n");
        return false;
    }

    int opt = 1;
    setsockopt(g_httpFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)g_config.ai.mcpPort);

    if (bind(g_httpFd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "[MCP-HTTP] Failed to bind port %d\n", g_config.ai.mcpPort);
        close(g_httpFd);
        g_httpFd = -1;
        return false;
    }

    if (listen(g_httpFd, 8) != 0) {
        fprintf(stderr, "[MCP-HTTP] Failed to listen\n");
        close(g_httpFd);
        g_httpFd = -1;
        return false;
    }

    g_httpRunning.store(true);

    g_httpThread = std::thread([]() {
        fprintf(stderr, "[MCP-HTTP] Server started on port %d\n", g_config.ai.mcpPort);
        while (g_httpRunning.load()) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = accept(g_httpFd, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientFd < 0) {
                if (!g_httpRunning.load()) break;
                continue;
            }
            HandleHTTPConnection(clientFd);
        }
        fprintf(stderr, "[MCP-HTTP] Server stopped\n");
    });

    return true;
}

void MCP_StopHTTPServer() {
    if (!g_httpRunning.exchange(false)) return;

    if (g_httpFd >= 0) {
        close(g_httpFd);
        g_httpFd = -1;
    }

    if (g_httpThread.joinable()) {
        g_httpThread.join();
    }
}

bool MCP_IsHTTPRunning() {
    return g_httpRunning.load();
}
