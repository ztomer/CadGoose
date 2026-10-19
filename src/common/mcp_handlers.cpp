#include "mcp_server.h"
#include "command_socket.h"
#include "config.h"
#include "hotkey_cache.h"
#include <string>
#include <vector>
#include <unordered_map>

extern std::string JsonEscape(const std::string& s);
extern std::string ExtractArg(const std::string& json, const std::string& key);

// MCP config handlers run on the internal server's background thread, which
// must never touch the plain bool fields of g_config: the main thread owns
// them. Every read and write of them is a command over the socket, answered
// by AppActions_HandleCommand on the main thread (ROADMAP M1.6 set_config,
// M1.7 get_config and resources/read). Hotkey strings are the exception:
// Hotkey_SetName / Hotkey_Name take the hotkey-cache mutex.

// --- MCP protocol version negotiation (SEP-2575) ---
// Supported versions, oldest first. The latest entry is the default answer
// when the client asks for nothing or for something unknown.
static const char* kSupportedProtocolVersions[] = {
    "2024-11-05",
    "2025-03-26",
    "2025-06-18",
    "2025-11-25",
    "2026-07-28",
};
static constexpr const char* kLatestProtocolVersion = "2026-07-28";

static std::string NegotiateProtocolVersion(const std::string& requested) {
    for (const char* v : kSupportedProtocolVersions) {
        if (requested == v) return requested;
    }
    return kLatestProtocolVersion;
}

std::string HandleInitialize(const std::string& requestedVersion) {
    return "{"
        "\"protocolVersion\":\"" + NegotiateProtocolVersion(requestedVersion) + "\","
        "\"capabilities\":{"
            "\"tools\":{},"
            "\"resources\":{}"
        "},"
        "\"serverInfo\":{\"name\":\"goose-mcp\",\"version\":\"1.0\"}"
    "}";
}

// --- Tool definitions (single source of truth) ---
struct ToolDef {
    const char* name;
    const char* description;
    const char* paramsJson; // MCP params schema (without required wrapper)
    const char* required;   // comma-separated required param names, or ""
};

static const ToolDef kTools[] = {
    {"spawn_goose", "Spawn a new goose on the desktop",
     "{\"name\":{\"type\":\"string\",\"description\":\"Optional name for the goose\"}}", ""},
    {"clear_geese", "Remove all geese from the desktop", "{}", ""},
    {"honk", "Make a goose honk", "{}", ""},
    {"fetch", "Make a goose fetch an item",
     "{\"type\":{\"type\":\"string\",\"description\":\"Item type: 'meme' or 'text'\"}}", ""},
    {"goose_status", "Get the current status of the goose system", "{}", ""},
    {"open_preferences", "Open the goose preferences window", "{}", ""},
    {"send_chat", "Send a chat message to the goose AI",
     "{\"message\":{\"type\":\"string\",\"description\":\"Message to send to the goose\"}}", "message"},
    {"enable_behavior", "Enable a goose behavior by ID (e.g. 'ball', 'hats', 'rainbow')",
     "{\"id\":{\"type\":\"string\",\"description\":\"Behavior ID to enable\"}}", "id"},
    {"disable_behavior", "Disable a goose behavior by ID",
     "{\"id\":{\"type\":\"string\",\"description\":\"Behavior ID to disable\"}}", "id"},
    {"get_config", "Get configuration values from the goose system. Returns all config if no key specified.",
     "{\"key\":{\"type\":\"string\",\"description\":\"Optional config key path (e.g. 'behaviors.fun.ball')\"}}", ""},
    {"set_config", "Set a configuration value on the goose system",
     "{\"key\":{\"type\":\"string\",\"description\":\"Config key path (e.g. 'behaviors.fun.ball')\"},\"value\":{\"type\":[\"string\",\"number\",\"boolean\"],\"description\":\"Value to set\"}}", "key,value"},
    {"set_hotkey", "Change a behavior hotkey (e.g. honcker_hotkey, jail_hotkey_o)",
     "{\"hotkey\":{\"type\":\"string\",\"description\":\"Hotkey field name (e.g. 'honcker_hotkey', 'jail_hotkey_o')\"},\"value\":{\"type\":\"string\",\"description\":\"New hotkey string (e.g. 'f', 'cmd+shift+p')\"}}", "hotkey,value"},
};
static constexpr int kToolCount = sizeof(kTools) / sizeof(kTools[0]);

static std::string ToolToMcpJson(const ToolDef& t) {
    std::string j = "{\"name\":\"" + std::string(t.name) + "\",";
    j += "\"description\":\"" + std::string(t.description) + "\",";
    j += "\"inputSchema\":{\"type\":\"object\",\"properties\":" + std::string(t.paramsJson) + "}}";
    return j;
}

static std::string ToolToOpenAIJson(const ToolDef& t) {
    std::string j = "{\"type\":\"function\",\"function\":{";
    j += "\"name\":\"" + std::string(t.name) + "\",";
    j += "\"description\":\"" + std::string(t.description) + "\",";
    j += "\"parameters\":{\"type\":\"object\",\"properties\":" + std::string(t.paramsJson);
    if (t.required[0]) {
        j += ",\"required\":[";
        const char* p = t.required;
        bool first = true;
        while (*p) {
            const char* end = p;
            while (*end && *end != ',') end++;
            if (!first) j += ",";
            j += "\"" + std::string(p, end - p) + "\"";
            first = false;
            p = (*end == ',') ? end + 1 : end;
        }
        j += "]";
    }
    j += "}}}";
    return j;
}

std::string HandleToolsList() {
    std::string j = "{\"tools\":[";
    for (int i = 0; i < kToolCount; i++) {
        if (i > 0) j += ",";
        j += ToolToMcpJson(kTools[i]);
    }
    j += "]}";
    return j;
}

std::string HandleResourcesList() {
    return "{"
        "\"resources\":["
            "{\"uri\":\"config://behaviors\",\"name\":\"All Behavior Config\",\"mimeType\":\"application/json\",\"description\":\"All behavior toggle states and hotkey configs\"}"
            ",{\"uri\":\"config://behaviors/fun\",\"name\":\"Fun Behavior Toggles\",\"mimeType\":\"application/json\",\"description\":\"Fun behavior toggles (ball, hats, rainbow, etc.)\"}"
            ",{\"uri\":\"config://behaviors/control\",\"name\":\"Control Behavior Toggles\",\"mimeType\":\"application/json\",\"description\":\"Control behavior toggles (honcker, jail, portals, etc.)\"}"
            ",{\"uri\":\"config://behaviors/info\",\"name\":\"Info Behavior Toggles\",\"mimeType\":\"application/json\",\"description\":\"Info behavior toggles (nametag, presence, configGUI)\"}"
            ",{\"uri\":\"config://behaviors/systems\",\"name\":\"System Behavior Toggles\",\"mimeType\":\"application/json\",\"description\":\"System behavior toggles (health, ai, pomodoro)\"}"
        "]}";
}

// Sends one command to the app and returns its answer with line ends removed;
// *ok says whether the app answered at all.
static std::string AskApp(const std::vector<std::string>& cmdArgs, bool* ok) {
    std::string response;
    std::string error;
    *ok = CommandSocket_Send(cmdArgs, &response, &error);
    if (!*ok) {
        return error.empty() ? "error: failed to send command to goose" : "error: " + error;
    }
    std::string trimmed;
    for (char c : response) {
        if (c == '\n' || c == '\r') continue;
        trimmed += c;
    }
    return trimmed;
}

std::string HandleResourcesRead(const std::string& uri) {
    // The settings are read by the app on its main thread (ROADMAP M1.7).
    static const std::unordered_map<std::string, std::string> groups = {
        {"config://behaviors", ""},
        {"config://behaviors/fun", "fun"},
        {"config://behaviors/control", "control"},
        {"config://behaviors/info", "info"},
        {"config://behaviors/systems", "systems"},
    };
    auto it = groups.find(uri);
    if (it == groups.end()) return "";
    std::vector<std::string> cmdArgs = {"get_config"};
    if (!it->second.empty()) cmdArgs.push_back(it->second);
    bool ok = false;
    const std::string json = AskApp(cmdArgs, &ok);
    return "{\"contents\":[{\"uri\":\"" + JsonEscape(uri) + "\",\"mimeType\":\"" +
           (ok ? "application/json" : "text/plain") + "\",\"text\":\"" + JsonEscape(json) + "\"}]}";
}

std::string MCP_GetOpenAITools() {
    std::string j = "[";
    for (int i = 0; i < kToolCount; i++) {
        if (i > 0) j += ",";
        j += ToolToOpenAIJson(kTools[i]);
    }
    j += "]";
    return j;
}

std::string ExecuteTool(const std::string& name, const std::string& argsJson) {
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
        if (msg.empty()) return "error: 'message' argument is required";
        cmd = "send\t" + msg;
    } else if (name == "enable_behavior") {
        std::string id = ExtractArg(argsJson, "id");
        if (id.empty()) return "error: 'id' argument is required";
        cmd = "enable\t" + id;
    } else if (name == "disable_behavior") {
        std::string id = ExtractArg(argsJson, "id");
        if (id.empty()) return "error: 'id' argument is required";
        cmd = "disable\t" + id;
    } else if (name == "get_config") {
        std::string key = ExtractArg(argsJson, "key");
        if (!key.empty()) {
            return "config for '" + key + "' - use resources/config:// URIs for structured data";
        }
        // The bool settings belong to the main thread: ask the app (M1.7).
        cmd = "get_config";
    } else if (name == "set_config") {
        std::string key = ExtractArg(argsJson, "key");
        std::string value = ExtractArg(argsJson, "value");
        if (key.empty()) return "error: 'key' argument is required";
        if (value.empty()) return "error: 'value' argument is required";
        // The bool fields belong to the main thread (the tick loop and
        // Config_SaveAll read them there), so the write goes through the
        // command socket like every other mutating tool, and the app applies
        // it on its own thread (ROADMAP M1.6). Writing g_config here was a
        // data race in the app and, in `--mcp` stdio mode, a write to this
        // process's copy that the running goose never saw.
        cmd = "set_config\t" + key + "\t" + value;
    } else if (name == "set_hotkey") {
        std::string hotkey = ExtractArg(argsJson, "hotkey");
        std::string value = ExtractArg(argsJson, "value");
        if (hotkey.empty() || value.empty()) return "error: 'hotkey' and 'value' arguments required";
        static const std::unordered_map<std::string, HotkeyId> hotkeyIds = {
            {"honcker_hotkey", HotkeyId::HonckerHonk},
            {"jail_hotkey_o", HotkeyId::JailSet},
            {"jail_hotkey_p", HotkeyId::JailToggle},
            {"portal_hotkey_1", HotkeyId::Portal1},
            {"portal_hotkey_2", HotkeyId::Portal2},
            {"portal_hotkey_0", HotkeyId::Portal0},
            {"breadcrumbs_hotkey", HotkeyId::Breadcrumbs},
        };
        auto it = hotkeyIds.find(hotkey);
        if (it == hotkeyIds.end()) return "error: unknown hotkey field '" + hotkey + "'";
        // Hotkey_SetName updates the g_config string under the mutex and refreshes
        // the lock-free key-code cache the behavior ticks read.
        Hotkey_SetName(it->second, value);
        return "ok: " + hotkey + " set to '" + value + "'";
    } else {
        return "error: unknown tool: " + name;
    }

    std::vector<std::string> cmdArgs;
    size_t start = 0;
    for (size_t i = 0; i <= cmd.size(); i++) {
        if (i == cmd.size() || cmd[i] == '\t') {
            cmdArgs.push_back(cmd.substr(start, i - start));
            start = i + 1;
        }
    }

    bool ok = false;
    return AskApp(cmdArgs, &ok);
}
