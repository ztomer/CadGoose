#include "config_registry.h"
#include "config_helpers.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

Config g_config;
double g_time = 0.0;
std::vector<ConfigOption> g_configRegistry;
std::unordered_map<std::string, ConfigOption*> g_configLookup;

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::filesystem::path ConfigDirPath() {
    if (const char* envDir = std::getenv("CADGOOSE_CONFIG_DIR")) {
        return fs::path(envDir);
    }
    if (fs::exists(fs::current_path() / "config" / "config.toml")) {
        return fs::current_path() / "config";
    }
#if defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        if (*home) return fs::path(home) / "Library" / "Application Support" / "CadGoose";
    }
    return fs::current_path();
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdg) return fs::path(xdg) / "desktop-goose";
    }
    if (const char* home = std::getenv("HOME")) {
        if (*home) return fs::path(home) / ".config" / "desktop-goose";
    }
    return fs::current_path();
#endif
}

std::string Config_GetPath() {
    return (ConfigDirPath() / "config.toml").string();
}

std::filesystem::path Config_GetThemesDir() {
    auto dir = ConfigDirPath() / "themes";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

static std::string Trim(const std::string& text) {
    const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (begin >= end) return "";
    return std::string(begin, end);
}

static bool ParseBoolValue(const std::string& rawValue, bool* outValue) {
    std::string lowered = rawValue;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") { *outValue = true; return true; }
    if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") { *outValue = false; return true; }
    return false;
}

static std::string OptionValueToString(const ConfigOption& opt) {
    if (opt.type == CFG_BOOL) return *(bool*)opt.ptr ? "1" : "0";
    if (opt.type == CFG_INT) return std::to_string(*(int*)opt.ptr);
    if (opt.type == CFG_FLOAT) { std::ostringstream stream; stream << *(float*)opt.ptr; return stream.str(); }
    return *(std::string*)opt.ptr;
}

const ConfigOption* Config_FindOptionByKey(const std::string& key) {
    auto it = g_configLookup.find(ToLower(key));
    if (it != g_configLookup.end()) return it->second;
    return nullptr;
}

static bool AssignValueToOption(const ConfigOption& opt, const std::string& rawValue, std::string* errorOut) {
    try {
        if (opt.type == CFG_BOOL) {
            bool parsed = false;
            if (!ParseBoolValue(Trim(rawValue), &parsed)) {
                if (errorOut) *errorOut = "Expected boolean for " + std::string(opt.key);
                return false;
            }
            *(bool*)opt.ptr = parsed;
            return true;
        }
        if (opt.type == CFG_INT) {
            int val = std::stoi(Trim(rawValue));
            val = std::clamp(val, static_cast<int>(opt.min), static_cast<int>(opt.max));
            *(int*)opt.ptr = val;
            return true;
        }
        if (opt.type == CFG_FLOAT) {
            float val = std::stof(Trim(rawValue));
            val = std::clamp(val, opt.min, opt.max);
            *(float*)opt.ptr = val;
            return true;
        }
        *(std::string*)opt.ptr = rawValue;
        return true;
    } catch (const std::exception&) {
        if (errorOut) *errorOut = "Invalid value for " + std::string(opt.key);
        return false;
    }
}

bool Config_GetValueByKey(const std::string& key, std::string* valueOut, std::string* errorOut) {
    const ConfigOption* opt = Config_FindOptionByKey(key);
    if (!opt) { if (errorOut) *errorOut = "Unknown config key: " + key; return false; }
    if (valueOut) *valueOut = OptionValueToString(*opt);
    return true;
}

bool Config_SetValueByKey(const std::string& key, const std::string& value, std::string* errorOut) {
    const ConfigOption* opt = Config_FindOptionByKey(key);
    if (!opt) { if (errorOut) *errorOut = "Unknown config key: " + key; return false; }
    if (!AssignValueToOption(*opt, value, errorOut)) return false;
    if (opt->onChange) opt->onChange();
    return true;
}

void OnConfigChange() {
    Config_UpdateActiveTheme();
    // (Rest of the function will just save, so we put it before saving)
    Config_SaveAll();
}

bool Config_SaveNow(std::string* errorOut) {
    Config_SaveAll();
    return true;
}

void Config_Init() {
    Config_InitRegistry();
    g_configLookup.clear();
    for (auto& opt : g_configRegistry) {
        g_configLookup[ToLower(opt.key)] = &opt;
    }
}

std::string Config_EvilPersonality(float level) {
    int state = std::min(static_cast<int>(round(level * 9)), 8);
    switch (state) {
        case 0: return "an adorable fluffy gosling";
        case 1: return "a friendly goose";
        case 2: return "a mischievous prankster goose";
        case 3: return "a sarcastic goose with attitude";
        case 4: return "a chaotic neutral goose";
        case 5: return "a grumpy goose having a bad day";
        case 6: return "a villainous goose scheming against humanity";
        case 7: return "an evil overlord goose bent on world domination";
        case 8: return "an absurdly eloquent goose dictator who has conquered Poland";
        default: return "a goose";
    }
}