#include "ai_mcp_bridge.h"
#include "mcp_server.h"
#include <algorithm>
#include <sstream>
#include <cctype>

std::vector<std::string> AI_TokenizeMessage(const std::string& message) {
    std::vector<std::string> tokens;
    std::istringstream iss(message);
    std::string word;
    while (iss >> word) {
        std::string cleaned;
        cleaned.reserve(word.size());
        for (char c : word) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
                cleaned += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        if (!cleaned.empty()) {
            tokens.push_back(cleaned);
        }
    }
    return tokens;
}

bool AI_MatchTokens(const std::vector<std::string>& tokens,
                    const std::vector<std::string>& pattern) {
    if (tokens.size() < pattern.size()) return false;
    for (size_t i = 0; i < pattern.size(); i++) {
        if (tokens[i] != pattern[i]) return false;
    }
    return true;
}

static bool HasToken(const std::vector<std::string>& tokens, const std::string& target) {
    return std::find(tokens.begin(), tokens.end(), target) != tokens.end();
}

bool AI_TryMCPCommand(const std::string& message, std::string& response) {
    auto tokens = AI_TokenizeMessage(message);
    if (tokens.empty()) return false;

    // "enable <behavior>" or "turn on <behavior>"
    if (tokens.size() >= 2 && AI_MatchTokens(tokens, {"enable"})) {
        std::string result = MCP_CallTool("enable_behavior",
            "{\"id\":\"" + tokens[1] + "\"}");
        if (result.find("error") != std::string::npos) {
            response = "HONK! Couldn't enable \"" + tokens[1] + "\": " + result;
        } else {
            response = "HONK! Enabled \"" + tokens[1] + "\" behavior!";
        }
        return true;
    }
    if (tokens.size() >= 3 && AI_MatchTokens(tokens, {"turn", "on"})) {
        std::string result = MCP_CallTool("enable_behavior",
            "{\"id\":\"" + tokens[2] + "\"}");
        if (result.find("error") != std::string::npos) {
            response = "HONK! Couldn't enable \"" + tokens[2] + "\": " + result;
        } else {
            response = "HONK! Turned on \"" + tokens[2] + "\" behavior!";
        }
        return true;
    }

    // "disable <behavior>" or "turn off <behavior>"
    if (tokens.size() >= 2 && AI_MatchTokens(tokens, {"disable"})) {
        std::string result = MCP_CallTool("disable_behavior",
            "{\"id\":\"" + tokens[1] + "\"}");
        if (result.find("error") != std::string::npos) {
            response = "HONK! Couldn't disable \"" + tokens[1] + "\": " + result;
        } else {
            response = "HONK! Disabled \"" + tokens[1] + "\" behavior!";
        }
        return true;
    }
    if (tokens.size() >= 3 && AI_MatchTokens(tokens, {"turn", "off"})) {
        std::string result = MCP_CallTool("disable_behavior",
            "{\"id\":\"" + tokens[2] + "\"}");
        if (result.find("error") != std::string::npos) {
            response = "HONK! Couldn't disable \"" + tokens[2] + "\": " + result;
        } else {
            response = "HONK! Turned off \"" + tokens[2] + "\" behavior!";
        }
        return true;
    }

    // "honk"
    if (tokens.size() == 1 && tokens[0] == "honk") {
        std::string result = MCP_CallTool("honk", "{}");
        if (result.find("error") != std::string::npos) {
            response = "HONK! HONK! (sending honk: " + result + ")";
        } else {
            response = "*HONK!* 🦆";
        }
        return true;
    }

    // "spawn [name]" or "spawn a goose named [name]"
    if (!tokens.empty() && tokens[0] == "spawn") {
        std::string name;
        bool hasGoose = false;
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i] == "goose" || tokens[i] == "geese") hasGoose = true;
        }
        // If there's a word after "spawn" that is not "a" or "goose", use it as name
        for (size_t i = 1; i < tokens.size(); i++) {
            if (tokens[i] != "a" && tokens[i] != "goose" && tokens[i] != "geese") {
                name = tokens[i];
                break;
            }
        }
        std::string args = name.empty() ? "{}" : "{\"name\":\"" + name + "\"}";
        std::string result = MCP_CallTool("spawn_goose", args);
        if (result.find("error") != std::string::npos) {
            response = "HONK! Spawn failed: " + result;
        } else if (!name.empty()) {
            response = "HONK! Spawned goose \"" + name + "\"!";
        } else {
            response = "HONK! New goose spawned!";
        }
        return true;
    }

    // "clear geese" or "clear all geese" or "remove geese"
    if (tokens[0] == "clear" || tokens[0] == "remove") {
        if (HasToken(tokens, "geese") || HasToken(tokens, "goose") || HasToken(tokens, "all")) {
            std::string result = MCP_CallTool("clear_geese", "{}");
            if (result.find("error") != std::string::npos) {
                response = "HONK! Cleared? " + result;
            } else {
                response = "HONK! All geese cleared! The desktop is empty... for now.";
            }
            return true;
        }
    }

    // "status", "goose status", "get status"
    if (tokens[0] == "status" || tokens[0] == "report" ||
        (tokens.size() >= 2 && tokens[0] == "goose" && tokens[1] == "status") ||
        (tokens.size() >= 2 && tokens[0] == "get" && tokens[1] == "status")) {
        std::string result = MCP_CallTool("goose_status", "{}");
        if (result.find("error") != std::string::npos) {
            response = "HONK! Status check: " + result;
        } else {
            response = "HONK! " + result;
        }
        return true;
    }

    // "open preferences" or "show preferences" or "open prefs"
    if (tokens[0] == "open" || tokens[0] == "show") {
        if (HasToken(tokens, "preferences") || HasToken(tokens, "prefs") ||
            HasToken(tokens, "settings") || HasToken(tokens, "config")) {
            std::string result = MCP_CallTool("open_preferences", "{}");
            if (result.find("error") != std::string::npos) {
                response = "HONK! Can't open prefs: " + result;
            } else {
                response = "HONK! Opening preferences...";
            }
            return true;
        }
    }

    // "fetch" or "fetch <type>"
    if (tokens[0] == "fetch") {
        std::string type;
        if (tokens.size() >= 2) {
            if (tokens[1] == "meme" || tokens[1] == "text") {
                type = tokens[1];
            }
        }
        std::string args = type.empty() ? "{}" : "{\"type\":\"" + type + "\"}";
        std::string result = MCP_CallTool("fetch", args);
        if (result.find("error") != std::string::npos) {
            response = "HONK! Fetch failed: " + result;
        } else {
            response = "HONK! Fetching " + (type.empty() ? "something" : type) + "...";
        }
        return true;
    }

    return false;
}
