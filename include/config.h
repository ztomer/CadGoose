#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <filesystem>
#include <vector>
#include <functional>
#include <optional>
#include <toml.hpp>
#include "ring_buffer.h"

enum ConfigType { CFG_BOOL, CFG_INT, CFG_FLOAT, CFG_STRING };

#define CONFIG_OPTION(section, key, label, type, memberPtr, onChangeCb) \
    { section, key, "", label, "", type, memberPtr, 0.0f, 1000.0f, 0.1f, "", onChangeCb }

#define CONFIG_BOOL(section, key, label, memberPtr, onChangeCb) \
    { section, key, "", label, "", CFG_BOOL, memberPtr, 0, 1, 1, "", onChangeCb }

#define CONFIG_BOOL_EX(section, key, label, explanation, memberPtr, onChangeCb) \
    { section, key, "", label, explanation, CFG_BOOL, memberPtr, 0, 1, 1, "", onChangeCb }

#define CONFIG_INT(section, key, label, memberPtr, minVal, maxVal, onChangeCb) \
    { section, key, "", label, "", CFG_INT, memberPtr, minVal, maxVal, 1, "", onChangeCb }

#define CONFIG_INT_EX(section, key, label, explanation, memberPtr, minVal, maxVal, onChangeCb) \
    { section, key, "", label, explanation, CFG_INT, memberPtr, minVal, maxVal, 1, "", onChangeCb }

#define CONFIG_FLOAT(section, key, label, memberPtr, minVal, maxVal, stepVal, onChangeCb) \
    { section, key, "", label, "", CFG_FLOAT, memberPtr, minVal, maxVal, stepVal, "", onChangeCb }

#define CONFIG_FLOAT_EX(section, key, label, explanation, memberPtr, minVal, maxVal, stepVal, onChangeCb) \
    { section, key, "", label, explanation, CFG_FLOAT, memberPtr, minVal, maxVal, stepVal, "", onChangeCb }

#define CONFIG_STRING(section, key, label, memberPtr, onChangeCb) \
    { section, key, "", label, "", CFG_STRING, memberPtr, 0.0f, 0.0f, 0.0f, "", onChangeCb }

struct ConfigOption {
  const char *section;
  const char *key;
  const char *lookupKey; // unique key for lookup map (defaults to key if empty)
  const char *label;
  const char *explanation;
  ConfigType type;
  void *ptr;
  float min;
  float max;
  float step;
  const char *suffix;
  std::function<void()> onChange = nullptr;
};

#include "config_sections.h"


struct Config {
  DebugConfig debug;
  GeneralConfig general;
  ScreenConfig screen;
  AssetConfig asset;
  MovementConfig movement;
  PhysicsConfig physics;
  SpawnConfig spawn;
  RigConfig rig;
  CursorConfig cursor;
  SnatchConfig snatch;
  MudConfig mud;
  HonkConfig honk;
  StepConfig step;
  ItemConfig item;
  RenderConfig render;
  ColorConfig color;
  BehaviorConfig behaviors;

   enum ProviderType { kProviderFoundation = 0, kProviderOsaurus = 1, kProviderOllama = 2, kProviderCustom = 3 };
  struct PortalConfig { std::string hotkey1 = "1"; std::string hotkey2 = "2"; std::string hotkey0 = "0"; float p1Width = 80.0f; float p1Height = 80.0f; float p2Width = 80.0f; float p2Height = 80.0f; float width = 80.0f; } portal;
  struct ModelProfile {
    std::string pattern;           // glob match on model name (e.g. "qwen*", "gemma*")
    float temperature = 0.8f;
    int maxTokens = 200;
    int timeoutSecs = 30;
    bool hasReasoningContent = false;  // model outputs thinking via reasoning_content
    bool prependJsonTrigger = false;   // prepend "Output JSON now." to system prompt
  };

  // Default AI endpoint URLs
  static constexpr const char* kDefaultChatEndpoint = "http://localhost:1337/v1/chat/completions";
  static constexpr const char* kDefaultModelsEndpoint = "http://localhost:1337/v1/models";

  struct AIConfig {
    int providerType = 0; // ProviderType enum (stored as int for config registry)
    int osaurusPort = 1337;
    int ollamaPort = 11434;
    int customPort = 1337;
    std::string osaurusModel = "foundation";
    std::string ollamaModel = "llama3";
    std::string customEndpoint = "http://localhost:1337/v1/chat/completions";
    std::string customModel = "foundation";
    std::string keychainService;
    float evilLevel = 0.5f;
    bool showStatusBar = true;
    bool enableMCP = false;
    int mcpPort = 31072;
    bool textMemeEnabled = false;
    float textMemeTemperature = 1.2f;
    bool textMemeAutoSave = false;
    int textMemeMaxQueue = 5;
    int chatMaxHistory = 100;
    bool localLlmEnabled = false;
    std::string localLlmModelPath;
    std::vector<std::string> localLlmSearchPaths; // Additional paths to search for CoreML models
  } ai;
  RingBuffer<std::string, 10> gooseNames;
};

extern Config g_config;

extern double g_time;

extern std::vector<ConfigOption> g_configRegistry;

void OnConfigChange();

std::filesystem::path ConfigDirPath();

// Every input ConfigDirPath() reads, captured as a value. The resolution is a
// pure function of it, so it is tested without setenv/unsetenv/chdir: those
// mutate process-global state that every other thread reads (getenv's pointer
// can dangle mid-read), which once dropped a config.toml and themes/ into the
// repo root under a torn relative name (ROADMAP G0.12).
struct ConfigDirSources {
    std::optional<std::string> configDirEnv;   // CADGOOSE_CONFIG_DIR
    std::optional<std::string> home;           // HOME
    std::optional<std::string> xdgConfigHome;  // XDG_CONFIG_HOME (Linux only)
    std::filesystem::path cwd;
};
// Reads the sources through `getenvFn` (std::getenv in production), copying
// each value at once; an empty value is treated as unset.
ConfigDirSources ConfigDirSources_FromEnv(const std::function<const char*(const char*)>& getenvFn,
                                          std::filesystem::path cwd);
ConfigDirSources ConfigDirSources_FromProcess();

// The settings directory of the app with this bundle id, when it is the
// INSTALLED app reading its own directory: ~/Library/Application Support/<bundle
// id> on macOS. Empty on Linux (that path is not bundle-id based) and empty
// when there is no home directory.
//
// The bundle id is a parameter, not a constant baked into the function, so the
// isolation is a property that can be pinned: two ids, two directories. Two
// apps that share one directory share one config.toml, and the two schemas
// have diverged (ROADMAP D1.1).
std::optional<std::filesystem::path> InstalledConfigDirPath(
    const ConfigDirSources& src, const std::string& bundleId);

// The directory this app used before it moved: the shared
// ~/Library/Application Support/CadGoose, which the macOS successor fork also
// reads and writes. Empty on Linux (there is no shared directory there).
std::optional<std::filesystem::path> LegacyConfigDirPath(const ConfigDirSources& src);

// The full resolution, for one bundle id. The environment override and the
// development tree come first (they are not the installed app's business),
// then the per-bundle-id directory.
std::filesystem::path ResolveConfigDir(const ConfigDirSources& src,
                                       const std::string& bundleId);
// ...for this app's bundle id.
std::filesystem::path ResolveConfigDir(const ConfigDirSources& src);

// What the one-time migration of the shared directory did.
enum class ConfigMigrationOutcome {
    kNothingToMigrate,  // no shared directory, or this app's own already exists
    kMigrated,          // the shared settings are now in this app's own directory
    kFailed,            // the copy failed; the app runs on its defaults
};

// Copy the shared directory's settings into this app's own directory, once.
//
// The shared directory is left where it is: the app that still reads it needs
// it. This app's own directory is created by the copy, so "it already exists"
// is the marker that says the migration is done, and nothing is touched then.
ConfigMigrationOutcome MigrateLegacyConfigDir(const std::filesystem::path& legacyDir,
                                              const std::filesystem::path& newDir);

// Run the migration for the running app, if there is a shared directory to
// migrate from. Called once per process, at the first resolution of the app's
// settings directory.
void MigrateLegacySettings();
// ...for these sources, so the decision can be tested without touching the
// process environment (ROADMAP G0.12).
void MigrateLegacySettings(const ConfigDirSources& src);

// Thread-safe in-process override that wins over every source above; the
// seam tests use instead of mutating CADGOOSE_CONFIG_DIR mid-suite. Returns
// the previous override so a caller can restore it (ScopedConfigDirOverride).
std::optional<std::filesystem::path> Config_ExchangeDirOverride(
    std::optional<std::filesystem::path> dir);

class ScopedConfigDirOverride {
public:
    explicit ScopedConfigDirOverride(std::filesystem::path dir)
        : m_previous(Config_ExchangeDirOverride(std::move(dir))) {}
    ~ScopedConfigDirOverride() { Config_ExchangeDirOverride(std::move(m_previous)); }
    ScopedConfigDirOverride(const ScopedConfigDirOverride&) = delete;
    ScopedConfigDirOverride& operator=(const ScopedConfigDirOverride&) = delete;
    ScopedConfigDirOverride(ScopedConfigDirOverride&&) = delete;
    ScopedConfigDirOverride& operator=(ScopedConfigDirOverride&&) = delete;

private:
    std::optional<std::filesystem::path> m_previous;
};

void Config_InitRegistry();
void Config_Init();
void Config_SaveAll();
void Config_SaveGooseNames();
void Config_LoadAll();
// What every platform's app start does to its settings: build the registry,
// then read config.toml over the defaults. The one start path both mains call,
// so a platform cannot initialise the registry and never load the file it
// saves to (ROADMAP M1.5: Linux did exactly that).
void Config_Startup();
void Config_Load(const toml::basic_value<toml::type_config>& config);

// config.toml schema version this build writes, as [meta] schema_version. A
// file without the key is version 1. Config_Load runs Config_MigrateSchema
// first, and every save stamps the current version, so each migration step
// runs once per file and never again (ROADMAP M1.1).
//   1 -> 2: audio_enabled = false was the shipped default until 2026-06-28 and
//           poisoned every config written from it; repair it to true once.
inline constexpr int kConfigSchemaVersion = 2;
int Config_SchemaVersionOf(const toml::basic_value<toml::type_config>& config);
toml::basic_value<toml::type_config> Config_MigrateSchema(toml::basic_value<toml::type_config> config);
std::string Config_GetPath();
std::filesystem::path Config_GetThemesDir();
const ConfigOption *Config_FindOptionByKey(const std::string &key);
bool Config_GetValueByKey(const std::string &key, std::string *valueOut,
                          std::string *errorOut = nullptr);
bool Config_SetValueByKey(const std::string &key, const std::string &value,
                          std::string *errorOut = nullptr);
bool Config_SaveNow(std::string *errorOut = nullptr);

void Config_UpdateActiveTheme();
bool Config_LoadThemeColors(const std::string& themeName, ColorRGB& body, ColorRGB& neck, ColorRGB& head, ColorRGB& beak, ColorRGB& eye, ColorRGB& outline);

std::string Config_EvilPersonality(float level);

#endif // CONFIG_H
