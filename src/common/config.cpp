#include "config_string.h"
#include "config_registry.h"
#include "config_helpers.h"
#include "hotkey_cache.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <atomic>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

Config g_config;
double g_time = 0.0;
std::vector<ConfigOption> g_configRegistry;

static std::unordered_map<std::string, ConfigOption*> g_configLookup;

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

ConfigDirSources ConfigDirSources_FromEnv(const std::function<const char*(const char*)>& getenvFn,
                                          fs::path cwd) {
    auto nonEmpty = [&getenvFn](const char* name) -> std::optional<std::string> {
        const char* v = getenvFn(name);
        if (v == nullptr || *v == '\0') return std::nullopt;
        return std::string(v);  // copied at once: never hold getenv's pointer
    };
    ConfigDirSources src;
    src.configDirEnv = nonEmpty("CADGOOSE_CONFIG_DIR");
    src.home = nonEmpty("HOME");
    src.xdgConfigHome = nonEmpty("XDG_CONFIG_HOME");
    src.cwd = std::move(cwd);
    return src;
}

ConfigDirSources ConfigDirSources_FromProcess() {
    std::error_code ec;
    return ConfigDirSources_FromEnv([](const char* name) { return std::getenv(name); },
                                    fs::current_path(ec));
}

// Empty values arrive as nullopt (ConfigDirSources_FromEnv): an empty CADGOOSE_CONFIG_DIR
// used to resolve to the relative path "", i.e. whatever the cwd was.
fs::path ResolveConfigDir(const ConfigDirSources& src) {
    if (src.configDirEnv) return fs::path(*src.configDirEnv);
    std::error_code ec;
    if (fs::exists(src.cwd / "config" / "config.toml", ec)) {
        return src.cwd / "config";
    }
#if defined(__APPLE__)
    if (src.home) return fs::path(*src.home) / "Library" / "Application Support" / "CadGoose";
#else
    if (src.xdgConfigHome) return fs::path(*src.xdgConfigHome) / "desktop-goose";
    if (src.home) return fs::path(*src.home) / ".config" / "desktop-goose";
#endif
    return src.cwd;
}

static std::mutex g_configDirOverrideMutex;
static std::optional<fs::path> g_configDirOverride;

std::optional<fs::path> Config_ExchangeDirOverride(std::optional<fs::path> dir) {
    std::lock_guard<std::mutex> lock(g_configDirOverrideMutex);
    std::swap(g_configDirOverride, dir);
    return dir;
}

std::filesystem::path ConfigDirPath() {
    {
        std::lock_guard<std::mutex> lock(g_configDirOverrideMutex);
        if (g_configDirOverride) return *g_configDirOverride;
    }
    return ResolveConfigDir(ConfigDirSources_FromProcess());
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
    if (!opt.ptr) return "";
    if (opt.type == CFG_BOOL) return *(bool*)opt.ptr ? "1" : "0";
    if (opt.type == CFG_INT) return std::to_string(*(int*)opt.ptr);
    if (opt.type == CFG_FLOAT) { std::ostringstream stream; stream << *(float*)opt.ptr; return stream.str(); }
    return ConfigOption_GetString(opt);
}

const ConfigOption* Config_FindOptionByKey(const std::string& key) {
    auto it = g_configLookup.find(ToLower(key));
    if (it != g_configLookup.end()) return it->second;
    return nullptr;
}

static bool AssignValueToOption(const ConfigOption& opt, const std::string& rawValue, std::string* errorOut) {
    if (!opt.ptr) {
        if (errorOut) *errorOut = "Null pointer for " + std::string(opt.key);
        return false;
    }
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
            if (opt.min <= opt.max) {
                val = std::clamp(val, static_cast<int>(opt.min), static_cast<int>(opt.max));
            }
            *(int*)opt.ptr = val;
            return true;
        }
        if (opt.type == CFG_FLOAT) {
            float val = std::stof(Trim(rawValue));
            if (opt.min <= opt.max) {
                val = std::clamp(val, opt.min, opt.max);
            }
            *(float*)opt.ptr = val;
            return true;
        }
        ConfigOption_SetString(opt, rawValue);
        return true;
    } catch (const std::invalid_argument&) {
        if (errorOut) *errorOut = "Invalid value for " + std::string(opt.key);
        return false;
    } catch (const std::out_of_range&) {
        if (errorOut) *errorOut = "Value out of range for " + std::string(opt.key);
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

#ifdef __APPLE__
#include "audio.h"
void (*g_updateStatusBarIconFn)() = nullptr;
#endif

void OnConfigChange() {
    try {
        Hotkey_SyncFromConfig();  // keep the lock-free hotkey key-code cache current
        Config_UpdateActiveTheme();
#ifdef __APPLE__
        Audio_SetEnabled(g_config.general.audioEnabled);
        Audio_SetMuted(g_config.general.audioMuted);
        if (g_updateStatusBarIconFn) {
            g_updateStatusBarIconFn();
        }
#endif
        Config_SaveAll();
    } catch (const std::exception& e) {
        fprintf(stderr, "[CONFIG] OnConfigChange error: %s\n", e.what());
    } catch (...) {
        fprintf(stderr, "[CONFIG] Unknown OnConfigChange error\n");
    }
}

bool Config_SaveNow(std::string* errorOut) {
    try {
        Config_SaveAll();
        return true;
    } catch (const std::exception& e) {
        if (errorOut) *errorOut = e.what();
        return false;
    }
}

static std::mutex g_configMutex;
static std::atomic<bool> g_configInitialized{false};

void Config_Init() {
    std::lock_guard<std::mutex> lock(g_configMutex);
    if (g_configInitialized.load()) {
        return;
    }
    Config_InitRegistry();
    g_configLookup.clear();
    for (auto& opt : g_configRegistry) {
        const char* lookup = opt.lookupKey && *opt.lookupKey ? opt.lookupKey : opt.key;
        std::string lower = ToLower(lookup);
        g_configLookup[lower] = &opt;
    }
    g_configInitialized.store(true);
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