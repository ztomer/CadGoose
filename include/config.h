#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <filesystem>
#include <vector>
#include <functional>
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

void Config_InitRegistry();
void Config_Init();
void Config_SaveAll();
void Config_SaveGooseNames();
void Config_LoadAll();
void Config_Load(const toml::basic_value<toml::type_config>& config);
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
