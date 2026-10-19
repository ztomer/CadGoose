#include "mcp_config_json.h"
#include "config.h"
#include "hotkey_cache.h"
#include <string>
#include <vector>

extern std::string JsonEscape(const std::string& s);

// Serializers for the MCP config tools. They read g_config's plain bool
// fields, so they run on the main thread only (see mcp_config_json.h).

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

static std::string GroupJson(const std::string& group) {
    if (group == "fun") {
        return SerializeBoolGroup({
            {"ball", &g_config.behaviors.fun.ball},
            {"breadCrumbs", &g_config.behaviors.fun.breadCrumbs},
            {"hats", &g_config.behaviors.fun.hats},
            {"rainbow", &g_config.behaviors.fun.rainbow},
            {"acid", &g_config.behaviors.fun.acid},
            {"anger", &g_config.behaviors.fun.anger}
        });
    }
    if (group == "control") {
        return SerializeBoolGroup({
            {"honcker", &g_config.behaviors.control.honcker},
            {"jail", &g_config.behaviors.control.jail},
            {"portals", &g_config.behaviors.control.portals},
            {"drag", &g_config.behaviors.control.drag}
        });
    }
    if (group == "info") {
        return SerializeBoolGroup({
            {"nametag", &g_config.behaviors.info.nametag},
            {"presence", &g_config.behaviors.info.presence},
            {"configGUI", &g_config.behaviors.info.configGUI}
        });
    }
    if (group == "systems") {
        return SerializeBoolGroup({
            {"health", &g_config.behaviors.systems.health},
            {"ai", &g_config.behaviors.systems.ai},
            {"pomodoro", &g_config.behaviors.systems.pomodoro}
        });
    }
    return "";
}


static std::string AllConfigJson() {
    std::string j = "{";
    j += "\"fun\":" + GroupJson("fun") + ",";
    j += "\"control\":" + GroupJson("control") + ",";
    j += "\"info\":" + GroupJson("info") + ",";
    j += "\"systems\":" + GroupJson("systems") + ",";
    // Hotkey strings are read under the hotkey-cache mutex (Hotkey_Name).
    j += "\"honcker_hotkey\":\"" + JsonEscape(Hotkey_Name(HotkeyId::HonckerHonk)) + "\",";
    j += "\"jail_hotkey_o\":\"" + JsonEscape(Hotkey_Name(HotkeyId::JailSet)) + "\",";
    j += "\"jail_hotkey_p\":\"" + JsonEscape(Hotkey_Name(HotkeyId::JailToggle)) + "\",";
    j += "\"portal_hotkey_1\":\"" + JsonEscape(Hotkey_Name(HotkeyId::Portal1)) + "\",";
    j += "\"portal_hotkey_2\":\"" + JsonEscape(Hotkey_Name(HotkeyId::Portal2)) + "\",";
    j += "\"portal_hotkey_0\":\"" + JsonEscape(Hotkey_Name(HotkeyId::Portal0)) + "\",";
    j += "\"breadcrumbs_hotkey\":\"" + JsonEscape(Hotkey_Name(HotkeyId::Breadcrumbs)) + "\"";
    j += "}";
    return j;
}

std::string McpConfigJson(const std::string& group) {
    return group.empty() ? AllConfigJson() : GroupJson(group);
}
