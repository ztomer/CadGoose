#include "mcp_server.h"
#include "command_socket.h"
#include "config.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

extern std::string JsonEscape(const std::string& s);
extern std::string ExtractArg(const std::string& json, const std::string& key);

// --- Helper: serialize a bool to JSON "true"/"false" ---
static std::string BoolJson(bool v) { return v ? "true" : "false"; }

// --- Helper: serialize a behavior group to JSON ---
struct BoolField { const char* name; bool* ptr; };

static std::string SerializeBoolGroup(const std::vector<BoolField>& fields) {
    std::string j = "{";
    for (size_t i = 0; i < fields.size(); i++) {
        if (i > 0) j += ",";
        j += std::string("\"") + fields[i].name + "\":" + BoolJson(*fields[i].ptr);
    }
    j += "}";
    return j;
}

static std::string ConfigToJson() {
    std::string j = "{";
    j += "\"fun\":" + SerializeBoolGroup({
        {"ball", &g_config.behaviors.fun.ball},
        {"breadCrumbs", &g_config.behaviors.fun.breadCrumbs},
        {"hats", &g_config.behaviors.fun.hats},
        {"rainbow", &g_config.behaviors.fun.rainbow},
        {"acid", &g_config.behaviors.fun.acid},
        {"anger", &g_config.behaviors.fun.anger}
    }) + ",";
    j += "\"control\":" + SerializeBoolGroup({
        {"honcker", &g_config.behaviors.control.honcker},
        {"jail", &g_config.behaviors.control.jail},
        {"portals", &g_config.behaviors.control.portals},
        {"drag", &g_config.behaviors.control.drag}
    }) + ",";
    j += "\"info\":" + SerializeBoolGroup({
        {"nametag", &g_config.behaviors.info.nametag},
        {"presence", &g_config.behaviors.info.presence},
        {"configGUI", &g_config.behaviors.info.configGUI}
    }) + ",";
    j += "\"systems\":" + SerializeBoolGroup({
        {"health", &g_config.behaviors.systems.health},
        {"ai", &g_config.behaviors.systems.ai},
        {"pomodoro", &g_config.behaviors.systems.pomodoro}
    }) + ",";
    j += "\"honcker_hotkey\":\"" + JsonEscape(g_config.behaviors.honcker.hotkey) + "\",";
    j += "\"jail_hotkey_o\":\"" + JsonEscape(g_config.behaviors.jail.hotkeyO) + "\",";
    j += "\"jail_hotkey_p\":\"" + JsonEscape(g_config.behaviors.jail.hotkeyP) + "\",";
    j += "\"portal_hotkey_1\":\"" + JsonEscape(g_config.portal.hotkey1) + "\",";
    j += "\"portal_hotkey_2\":\"" + JsonEscape(g_config.portal.hotkey2) + "\",";
    j += "\"portal_hotkey_0\":\"" + JsonEscape(g_config.portal.hotkey0) + "\",";
    j += "\"breadcrumbs_hotkey\":\"" + JsonEscape(g_config.behaviors.breadCrumbs.hotkey) + "\"";
    j += "}";
    return j;
}

std::string HandleInitialize() {
    return "{"
        "\"protocolVersion\":\"2024-11-05\","
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

std::string HandleResourcesRead(const std::string& uri) {
    std::string json;
    if (uri == "config://behaviors") {
        json = ConfigToJson();
    } else if (uri == "config://behaviors/fun") {
        json = SerializeBoolGroup({
            {"ball", &g_config.behaviors.fun.ball},
            {"breadCrumbs", &g_config.behaviors.fun.breadCrumbs},
            {"hats", &g_config.behaviors.fun.hats},
            {"rainbow", &g_config.behaviors.fun.rainbow},
            {"acid", &g_config.behaviors.fun.acid},
            {"anger", &g_config.behaviors.fun.anger}
        });
    } else if (uri == "config://behaviors/control") {
        json = SerializeBoolGroup({
            {"honcker", &g_config.behaviors.control.honcker},
            {"jail", &g_config.behaviors.control.jail},
            {"portals", &g_config.behaviors.control.portals},
            {"drag", &g_config.behaviors.control.drag}
        });
    } else if (uri == "config://behaviors/info") {
        json = SerializeBoolGroup({
            {"nametag", &g_config.behaviors.info.nametag},
            {"presence", &g_config.behaviors.info.presence},
            {"configGUI", &g_config.behaviors.info.configGUI}
        });
    } else if (uri == "config://behaviors/systems") {
        json = SerializeBoolGroup({
            {"health", &g_config.behaviors.systems.health},
            {"ai", &g_config.behaviors.systems.ai},
            {"pomodoro", &g_config.behaviors.systems.pomodoro}
        });
    } else {
        return "";
    }
    return "{\"contents\":[{\"uri\":\"" + JsonEscape(uri) + "\",\"mimeType\":\"application/json\",\"text\":\"" + JsonEscape(json) + "\"}]}";
}

static bool SetConfigValue(const std::string& key, const std::string& valueJson) {
    static const std::unordered_map<std::string, bool*> boolFields = {
        {"behaviors.fun.ball", &g_config.behaviors.fun.ball},
        {"behaviors.fun.breadCrumbs", &g_config.behaviors.fun.breadCrumbs},
        {"behaviors.fun.hats", &g_config.behaviors.fun.hats},
        {"behaviors.fun.rainbow", &g_config.behaviors.fun.rainbow},
        {"behaviors.fun.acid", &g_config.behaviors.fun.acid},
        {"behaviors.fun.anger", &g_config.behaviors.fun.anger},
        {"behaviors.control.honcker", &g_config.behaviors.control.honcker},
        {"behaviors.control.jail", &g_config.behaviors.control.jail},
        {"behaviors.control.portals", &g_config.behaviors.control.portals},
        {"behaviors.control.drag", &g_config.behaviors.control.drag},
        {"behaviors.info.nametag", &g_config.behaviors.info.nametag},
        {"behaviors.info.presence", &g_config.behaviors.info.presence},
        {"behaviors.info.configGUI", &g_config.behaviors.info.configGUI},
        {"behaviors.info.visible", &g_config.behaviors.info.visible},
        {"behaviors.systems.health", &g_config.behaviors.systems.health},
        {"behaviors.systems.ai", &g_config.behaviors.systems.ai},
        {"behaviors.systems.pomodoro", &g_config.behaviors.systems.pomodoro},
    };
    auto it = boolFields.find(key);
    if (it == boolFields.end()) return false;
    *it->second = (valueJson == "true");
    return true;
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
        return ConfigToJson();
    } else if (name == "set_config") {
        std::string key = ExtractArg(argsJson, "key");
        std::string value = ExtractArg(argsJson, "value");
        if (key.empty()) return "error: 'key' argument is required";
        if (value.empty()) return "error: 'value' argument is required";
        if (!SetConfigValue(key, value)) {
            return "error: unknown config key '" + key + "'";
        }
        return "ok: " + key + " set to " + value;
    } else if (name == "set_hotkey") {
        std::string hotkey = ExtractArg(argsJson, "hotkey");
        std::string value = ExtractArg(argsJson, "value");
        if (hotkey.empty() || value.empty()) return "error: 'hotkey' and 'value' arguments required";
        static const std::unordered_map<std::string, std::string*> hotkeyFields = {
            {"honcker_hotkey", &g_config.behaviors.honcker.hotkey},
            {"jail_hotkey_o", &g_config.behaviors.jail.hotkeyO},
            {"jail_hotkey_p", &g_config.behaviors.jail.hotkeyP},
            {"portal_hotkey_1", &g_config.portal.hotkey1},
            {"portal_hotkey_2", &g_config.portal.hotkey2},
            {"portal_hotkey_0", &g_config.portal.hotkey0},
            {"breadcrumbs_hotkey", &g_config.behaviors.breadCrumbs.hotkey},
        };
        auto it = hotkeyFields.find(hotkey);
        if (it == hotkeyFields.end()) return "error: unknown hotkey field '" + hotkey + "'";
        *it->second = value;
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

    std::string response;
    std::string error;
    if (!CommandSocket_Send(cmdArgs, &response, &error)) {
        if (!error.empty()) return "error: " + error;
        return "error: failed to send command to goose";
    }

    std::string trimmed;
    for (char c : response) {
        if (c == '\n' || c == '\r') continue;
        trimmed += c;
    }
    return trimmed;
}
