#include "mcp_server.h"
#include "command_socket.h"
#include "config.h"
#include <string>
#include <vector>

extern std::string JsonEscape(const std::string& s);
extern std::string ExtractArg(const std::string& json, const std::string& key);

static std::string ConfigToJson() {
    std::string j = "{";
    j += "\"fun\":{";
    j += "\"ball\":" + std::string(g_config.behaviors.fun.ball ? "true" : "false") + ",";
    j += "\"breadCrumbs\":" + std::string(g_config.behaviors.fun.breadCrumbs ? "true" : "false") + ",";
    j += "\"hats\":" + std::string(g_config.behaviors.fun.hats ? "true" : "false") + ",";
    j += "\"rainbow\":" + std::string(g_config.behaviors.fun.rainbow ? "true" : "false") + ",";
    j += "\"acid\":" + std::string(g_config.behaviors.fun.acid ? "true" : "false") + ",";
    j += "\"anger\":" + std::string(g_config.behaviors.fun.anger ? "true" : "false");
    j += "},";
    j += "\"control\":{";
    j += "\"honcker\":" + std::string(g_config.behaviors.control.honcker ? "true" : "false") + ",";
    j += "\"jail\":" + std::string(g_config.behaviors.control.jail ? "true" : "false") + ",";
    j += "\"portals\":" + std::string(g_config.behaviors.control.portals ? "true" : "false") + ",";
    j += "\"drag\":" + std::string(g_config.behaviors.control.drag ? "true" : "false");
    j += "},";
    j += "\"info\":{";
    j += "\"nametag\":" + std::string(g_config.behaviors.info.nametag ? "true" : "false") + ",";
    j += "\"presence\":" + std::string(g_config.behaviors.info.presence ? "true" : "false") + ",";
    j += "\"configGUI\":" + std::string(g_config.behaviors.info.configGUI ? "true" : "false");
    j += "},";
    j += "\"systems\":{";
    j += "\"health\":" + std::string(g_config.behaviors.systems.health ? "true" : "false") + ",";
    j += "\"ai\":" + std::string(g_config.behaviors.systems.ai ? "true" : "false") + ",";
    j += "\"pomodoro\":" + std::string(g_config.behaviors.systems.pomodoro ? "true" : "false");
    j += "},";
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

std::string HandleToolsList() {
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
        ",{\"name\":\"get_config\",\"description\":\"Get configuration values from the goose system. Returns all config if no key specified.\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"key\":{\"type\":\"string\",\"description\":\"Optional config key path (e.g. 'behaviors.fun.ball', 'general.appearanceMode')\"}}}}"
        ",{\"name\":\"set_config\",\"description\":\"Set a configuration value on the goose system\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"key\":{\"type\":\"string\",\"description\":\"Config key path (e.g. 'behaviors.fun.ball')\"},\"value\":{\"type\":[\"string\",\"number\",\"boolean\"],\"description\":\"Value to set\"}}}}"
        ",{\"name\":\"set_hotkey\",\"description\":\"Change a behavior hotkey (e.g. honcker_hotkey, jail_hotkey_o)\",\"inputSchema\":{\"type\":\"object\",\"properties\":{\"hotkey\":{\"type\":\"string\",\"description\":\"Hotkey field name (e.g. 'honcker_hotkey', 'jail_hotkey_o')\"},\"value\":{\"type\":\"string\",\"description\":\"New hotkey string (e.g. 'f', 'cmd+shift+p')\"}}}}"
    "]}";
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
        json = "{";
        json += "\"ball\":" + std::string(g_config.behaviors.fun.ball ? "true" : "false") + ",";
        json += "\"breadCrumbs\":" + std::string(g_config.behaviors.fun.breadCrumbs ? "true" : "false") + ",";
        json += "\"hats\":" + std::string(g_config.behaviors.fun.hats ? "true" : "false") + ",";
        json += "\"rainbow\":" + std::string(g_config.behaviors.fun.rainbow ? "true" : "false") + ",";
        json += "\"acid\":" + std::string(g_config.behaviors.fun.acid ? "true" : "false") + ",";
        json += "\"anger\":" + std::string(g_config.behaviors.fun.anger ? "true" : "false");
        json += "}";
    } else if (uri == "config://behaviors/control") {
        json = "{";
        json += "\"honcker\":" + std::string(g_config.behaviors.control.honcker ? "true" : "false") + ",";
        json += "\"jail\":" + std::string(g_config.behaviors.control.jail ? "true" : "false") + ",";
        json += "\"portals\":" + std::string(g_config.behaviors.control.portals ? "true" : "false") + ",";
        json += "\"drag\":" + std::string(g_config.behaviors.control.drag ? "true" : "false");
        json += "}";
    } else if (uri == "config://behaviors/info") {
        json = "{";
        json += "\"nametag\":" + std::string(g_config.behaviors.info.nametag ? "true" : "false") + ",";
        json += "\"presence\":" + std::string(g_config.behaviors.info.presence ? "true" : "false") + ",";
        json += "\"configGUI\":" + std::string(g_config.behaviors.info.configGUI ? "true" : "false");
        json += "}";
    } else if (uri == "config://behaviors/systems") {
        json = "{";
        json += "\"health\":" + std::string(g_config.behaviors.systems.health ? "true" : "false") + ",";
        json += "\"ai\":" + std::string(g_config.behaviors.systems.ai ? "true" : "false") + ",";
        json += "\"pomodoro\":" + std::string(g_config.behaviors.systems.pomodoro ? "true" : "false");
        json += "}";
    } else {
        return "";
    }
    return "{\"contents\":[{\"uri\":\"" + JsonEscape(uri) + "\",\"mimeType\":\"application/json\",\"text\":\"" + JsonEscape(json) + "\"}]}";
}

static bool SetConfigValue(const std::string& key, const std::string& valueJson) {
    if (key == "behaviors.fun.ball") g_config.behaviors.fun.ball = (valueJson == "true");
    else if (key == "behaviors.fun.breadCrumbs") g_config.behaviors.fun.breadCrumbs = (valueJson == "true");
    else if (key == "behaviors.fun.hats") g_config.behaviors.fun.hats = (valueJson == "true");
    else if (key == "behaviors.fun.rainbow") g_config.behaviors.fun.rainbow = (valueJson == "true");
    else if (key == "behaviors.fun.acid") g_config.behaviors.fun.acid = (valueJson == "true");
    else if (key == "behaviors.fun.anger") g_config.behaviors.fun.anger = (valueJson == "true");
    else if (key == "behaviors.control.honcker") g_config.behaviors.control.honcker = (valueJson == "true");
    else if (key == "behaviors.control.jail") g_config.behaviors.control.jail = (valueJson == "true");
    else if (key == "behaviors.control.portals") g_config.behaviors.control.portals = (valueJson == "true");
    else if (key == "behaviors.control.drag") g_config.behaviors.control.drag = (valueJson == "true");
    else if (key == "behaviors.info.nametag") g_config.behaviors.info.nametag = (valueJson == "true");
    else if (key == "behaviors.info.presence") g_config.behaviors.info.presence = (valueJson == "true");
    else if (key == "behaviors.info.configGUI") g_config.behaviors.info.configGUI = (valueJson == "true");
    else if (key == "behaviors.info.visible") g_config.behaviors.info.visible = (valueJson == "true");
    else if (key == "behaviors.systems.health") g_config.behaviors.systems.health = (valueJson == "true");
    else if (key == "behaviors.systems.ai") g_config.behaviors.systems.ai = (valueJson == "true");
    else if (key == "behaviors.systems.pomodoro") g_config.behaviors.systems.pomodoro = (valueJson == "true");
    else return false;
    return true;
}

std::string MCP_GetOpenAITools() {
    return "["
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"spawn_goose\","
            "\"description\":\"Spawn a new goose on the desktop\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"name\":{\"type\":\"string\",\"description\":\"Optional name for the goose\"}"
            "}}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"clear_geese\","
            "\"description\":\"Remove all geese from the desktop\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{}}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"honk\","
            "\"description\":\"Make a goose honk\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{}}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"fetch\","
            "\"description\":\"Make a goose fetch an item\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"type\":{\"type\":\"string\",\"description\":\"Item type: 'meme' or 'text'\"}"
            "}}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"goose_status\","
            "\"description\":\"Get the current status of the goose system\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{}}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"open_preferences\","
            "\"description\":\"Open the goose preferences window\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{}}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"send_chat\","
            "\"description\":\"Send a chat message to the goose AI\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"message\":{\"type\":\"string\",\"description\":\"Message to send to the goose\"}"
            "},\"required\":[\"message\"]}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"enable_behavior\","
            "\"description\":\"Enable a goose behavior by ID (e.g. 'ball', 'hats', 'rainbow')\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"id\":{\"type\":\"string\",\"description\":\"Behavior ID to enable\"}"
            "},\"required\":[\"id\"]}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"disable_behavior\","
            "\"description\":\"Disable a goose behavior by ID\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"id\":{\"type\":\"string\",\"description\":\"Behavior ID to disable\"}"
            "},\"required\":[\"id\"]}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"get_config\","
            "\"description\":\"Get configuration values from the goose system. Returns all config if no key specified.\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"key\":{\"type\":\"string\",\"description\":\"Optional config key path (e.g. 'behaviors.fun.ball')\"}"
            "}}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"set_config\","
            "\"description\":\"Set a configuration value on the goose system\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"key\":{\"type\":\"string\",\"description\":\"Config key path (e.g. 'behaviors.fun.ball')\"},"
                "\"value\":{\"type\":[\"string\",\"number\",\"boolean\"],\"description\":\"Value to set\"}"
            "},\"required\":[\"key\",\"value\"]}"
        "}},"
        "{\"type\":\"function\",\"function\":{"
            "\"name\":\"set_hotkey\","
            "\"description\":\"Change a behavior hotkey (e.g. honcker_hotkey, jail_hotkey_o)\","
            "\"parameters\":{\"type\":\"object\",\"properties\":{"
                "\"hotkey\":{\"type\":\"string\",\"description\":\"Hotkey field name (e.g. 'honcker_hotkey', 'jail_hotkey_o')\"},"
                "\"value\":{\"type\":\"string\",\"description\":\"New hotkey string (e.g. 'f', 'cmd+shift+p')\"}"
            "},\"required\":[\"hotkey\",\"value\"]}"
        "}}"
    "]";
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
        if (hotkey == "honcker_hotkey") g_config.behaviors.honcker.hotkey = value;
        else if (hotkey == "jail_hotkey_o") g_config.behaviors.jail.hotkeyO = value;
        else if (hotkey == "jail_hotkey_p") g_config.behaviors.jail.hotkeyP = value;
        else if (hotkey == "portal_hotkey_1") g_config.portal.hotkey1 = value;
        else if (hotkey == "portal_hotkey_2") g_config.portal.hotkey2 = value;
        else if (hotkey == "portal_hotkey_0") g_config.portal.hotkey0 = value;
        else if (hotkey == "breadcrumbs_hotkey") g_config.behaviors.breadCrumbs.hotkey = value;
        else return "error: unknown hotkey field '" + hotkey + "'";
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
