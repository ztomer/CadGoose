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

namespace {

std::thread g_serverThread;
std::atomic<bool> g_serverRunning{false};
int g_serverFd = -1;

static std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

static std::string MakeJsonResponse(const std::string& id, const std::string& resultJson) {
    return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + resultJson + "}\n";
}

static std::string MakeJsonError(const std::string& id, int code, const std::string& message) {
    return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"error\":{\"code\":" + std::to_string(code) + ",\"message\":\"" + JsonEscape(message) + "\"}}\n";
}

static std::string ExtractMethod(const std::string& json) {
    auto pos = json.find("\"method\"");
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + 7);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

static std::string ExtractId(const std::string& json) {
    auto pos = json.find("\"id\"");
    if (pos == std::string::npos) return "null";
    pos = json.find(':', pos + 3);
    if (pos == std::string::npos) return "null";
    pos = json.find_first_not_of(" \t", pos + 1);
    if (pos == std::string::npos) return "null";
    if (json[pos] == '"') {
        auto end = json.find('"', pos + 1);
        if (end == std::string::npos) return "null";
        return json.substr(pos, end - pos + 1);
    }
    auto end = json.find_first_of(",}\n", pos);
    if (end == std::string::npos) return json.substr(pos);
    std::string id = json.substr(pos, end - pos);
    while (!id.empty() && id.back() == ' ') id.pop_back();
    return id;
}

static std::string ExtractArg(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos = json.find_first_not_of(" \t", pos + 1);
    if (pos == std::string::npos) return "";
    if (json[pos] == '"') {
        auto end = json.find('"', pos + 1);
        if (end == std::string::npos) return "";
        std::string val = json.substr(pos + 1, end - pos - 1);
        std::string unescaped;
        for (size_t i = 0; i < val.size(); i++) {
            if (val[i] == '\\' && i + 1 < val.size()) {
                if (val[i+1] == 'n') unescaped += '\n';
                else if (val[i+1] == 't') unescaped += '\t';
                else if (val[i+1] == 'r') unescaped += '\r';
                else if (val[i+1] == '"') unescaped += '"';
                else if (val[i+1] == '\\') unescaped += '\\';
                else { unescaped += val[i]; unescaped += val[i+1]; }
                i++;
            } else {
                unescaped += val[i];
            }
        }
        return unescaped;
    }
    auto end = json.find_first_of(",}\n", pos);
    if (end == std::string::npos) return json.substr(pos);
    return json.substr(pos, end - pos);
}

static std::string HandleInitialize() {
    return "{\"protocolVersion\":\"2024-11-05\",\"capabilities\":{\"tools\":{}},\"serverInfo\":{\"name\":\"goose-mcp\",\"version\":\"1.0\"}}";
}

static std::string HandleToolsList() {
    return "{\"tools\":["
        "{\"name\":\"spawn_goose\",\"description\":\"Spawn a new goose on the desktop\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"name\":{\"type\":\"string\",\"description\":\"Optional name for the goose\"}}}}"
        ",{\"name\":\"clear_geese\",\"description\":\"Remove all geese from the desktop\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}"
        ",{\"name\":\"honk\",\"description\":\"Make a goose honk\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}"
        ",{\"name\":\"fetch\",\"description\":\"Make a goose fetch an item\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"type\":{\"type\":\"string\",\"description\":\"Item type: 'meme' or 'text'\"}}}}"
        ",{\"name\":\"goose_status\",\"description\":\"Get the current status of the goose system\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}"
        ",{\"name\":\"open_preferences\",\"description\":\"Open the goose preferences window\",\"inputSchema\":{\"type\":\"object\",\"properties\":{}}}"
        ",{\"name\":\"send_chat\",\"description\":\"Send a chat message to the goose AI\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"message\":{\"type\":\"string\",\"description\":\"Message to send to the goose\"}}}}"
        ",{\"name\":\"enable_behavior\",\"description\":\"Enable a goose behavior by ID (e.g. 'ball', 'hats', 'rainbow')\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"id\":{\"type\":\"string\",\"description\":\"Behavior ID to enable\"}}}}"
        ",{\"name\":\"disable_behavior\",\"description\":\"Disable a goose behavior by ID\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"id\":{\"type\":\"string\",\"description\":\"Behavior ID to disable\"}}}}"
    "]}";
}

static std::string ExecuteTool(const std::string& name, const std::string& argsJson) {
    std::string cmd = name;
    if (name == "spawn_goose") {
        std::string gooseName = ExtractArg(argsJson, "name");
        if (!gooseName.empty()) cmd = "spawn\t" + gooseName;
        else cmd = "spawn";
    } else if (name == "clear_geese") {
        cmd = "clear";
    } else if (name == "honk") {
        cmd = "honk";
    } else if (name == "fetch") {
        std::string type = ExtractArg(argsJson, "type");
        cmd = "fetch";
        if (!type.empty()) cmd += "\t" + type;
    } else if (name == "goose_status") {
        cmd = "status";
    } else if (name == "open_preferences") {
        cmd = "prefs";
    } else if (name == "send_chat") {
        std::string msg = ExtractArg(argsJson, "message");
        if (msg.empty()) return "{\"content\":[{\"type\":\"text\",\"text\":\"error: 'message' argument is required\"}]}";
        cmd = "send\t" + msg;
    } else if (name == "enable_behavior") {
        std::string id = ExtractArg(argsJson, "id");
        if (id.empty()) return "{\"content\":[{\"type\":\"text\",\"text\":\"error: 'id' argument is required\"}]}";
        cmd = "enable\t" + id;
    } else if (name == "disable_behavior") {
        std::string id = ExtractArg(argsJson, "id");
        if (id.empty()) return "{\"content\":[{\"type\":\"text\",\"text\":\"error: 'id' argument is required\"}]}";
        cmd = "disable\t" + id;
    } else {
        return "{\"content\":[{\"type\":\"text\",\"text\":\"error: unknown tool: " + JsonEscape(name) + "\"}]}";
    }

    std::vector<std::string> cmdArgs;
    size_t start = 0;
    for (size_t i = 0; i <= cmd.size(); i++) {
        if (i == cmd.size() || cmd[i] == '\t') {
            cmdArgs.push_back(cmd.substr(start, i - start));
            start = i + 1;
        }
    }

    std::string response;
    std::string error;
    if (!CommandSocket_Send(cmdArgs, &response, &error)) {
        if (!error.empty()) {
            return "{\"content\":[{\"type\":\"text\",\"text\":\"error: " + JsonEscape(error) + "\"}]}";
        }
        return "{\"content\":[{\"type\":\"text\",\"text\":\"error: failed to send command to goose\"}]}";
    }

    std::string trimmed;
    for (char c : response) {
        if (c == '\n' || c == '\r') continue;
        trimmed += c;
    }
    return "{\"content\":[{\"type\":\"text\",\"text\":\"" + JsonEscape(trimmed) + "\"}]}";
}

static std::string ProcessRequest(const std::string& line) {
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
        return MakeJsonResponse(id, "{\"content\":" + result + "}");
    }

    if (method == "notifications/initialized") {
        return "";
    }

    return MakeJsonError(id, -32601, "Method not found: " + method);
}

static void WriteAll(int fd, const std::string& data) {
    size_t written = 0;
    while (written < data.size()) {
        ssize_t rc = write(fd, data.data() + written, data.size() - written);
        if (rc <= 0) return;
        written += (size_t)rc;
    }
}

static void HandleConnection(int clientFd) {
    std::string data;
    char buffer[4096];
    while (true) {
        ssize_t rc = read(clientFd, buffer, sizeof(buffer) - 1);
        if (rc <= 0) break;
        buffer[rc] = '\0';
        data += buffer;
        if (data.find('\n') != std::string::npos) break;
    }

    size_t pos = 0;
    while (pos < data.size()) {
        auto nl = data.find('\n', pos);
        if (nl == std::string::npos) break;
        std::string line = data.substr(pos, nl - pos);
        pos = nl + 1;
        std::string response = ProcessRequest(line);
        if (!response.empty()) {
            WriteAll(clientFd, response);
        }
    }
}

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
                std::string response = ProcessRequest(buffer);
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