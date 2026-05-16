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
    { section, key, label, "", type, memberPtr, 0.0f, 1000.0f, 0.1f, "", onChangeCb }

#define CONFIG_BOOL(section, key, label, memberPtr, onChangeCb) \
    { section, key, label, "", CFG_BOOL, memberPtr, 0, 1, 1, "", onChangeCb }

#define CONFIG_BOOL_EX(section, key, label, explanation, memberPtr, onChangeCb) \
    { section, key, label, explanation, CFG_BOOL, memberPtr, 0, 1, 1, "", onChangeCb }

#define CONFIG_INT(section, key, label, memberPtr, minVal, maxVal, onChangeCb) \
    { section, key, label, "", CFG_INT, memberPtr, minVal, maxVal, 1, "", onChangeCb }

#define CONFIG_INT_EX(section, key, label, explanation, memberPtr, minVal, maxVal, onChangeCb) \
    { section, key, label, explanation, CFG_INT, memberPtr, minVal, maxVal, 1, "", onChangeCb }

#define CONFIG_FLOAT(section, key, label, memberPtr, minVal, maxVal, stepVal, onChangeCb) \
    { section, key, label, "", CFG_FLOAT, memberPtr, minVal, maxVal, stepVal, "", onChangeCb }

#define CONFIG_FLOAT_EX(section, key, label, explanation, memberPtr, minVal, maxVal, stepVal, onChangeCb) \
    { section, key, label, explanation, CFG_FLOAT, memberPtr, minVal, maxVal, stepVal, "", onChangeCb }

#define CONFIG_STRING(section, key, label, memberPtr, onChangeCb) \
    { section, key, label, "", CFG_STRING, memberPtr, 0.0f, 0.0f, 0.0f, "", onChangeCb }

struct ConfigOption {
  const char *section;
  const char *key;
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

struct DebugConfig {
  bool toTerminal = false;
  bool visuals = false;
};

// Appearance modes: 0=Light (default goose), 1=Dark (Canada goose), 2=System (follow macOS), 3=Custom
static const int APPEARANCE_LIGHT = 0;
static const int APPEARANCE_DARK = 1;
static const int APPEARANCE_SYSTEM = 2;
static const int APPEARANCE_CUSTOM = 3;

struct GeneralConfig {
  float globalScale = 1.0f;
  bool audioEnabled = true;
  bool audioMuted = false;
  bool memesEnabled = true;
  bool canadaGooseMode = false; // deprecated, use appearanceMode
  int appearanceMode = APPEARANCE_SYSTEM;
  std::string lightThemeRole = "Default";
  std::string darkThemeRole = "Canadian";
  std::string failsafeHotkey = "cmd+shift+escape";
};

struct ScreenConfig {
  int defaultWidth = 1920;
  int defaultHeight = 1080;
};

struct AssetConfig {
  int memePlaceholderW = 200;
  int memePlaceholderH = 200;
  int textPlaceholderW = 300;
  int textPlaceholderH = 150;
  int notePlaceholderW = 300;
  int notePlaceholderH = 150;
};

struct MovementConfig {
  float baseWalkSpeed = 180.0f;
  float baseRunSpeed = 480.0f;
  float maxForce = 1000.0f;
  float moveDistanceThreshold = 1.0f;
  float friction = 0.85f;
  float maxSpeed = 600.0f;
  float runSpeedMultiplier = 1.25f;
  float initDirectionMax = 360.0f;
  float speedLerpRate = 0.05f;
  float arrivalRadius = 50.0f;
  float runDistanceThreshold = 1200.0f;
  float directionBlendRate = 0.15f;
};

struct PhysicsConfig {
  float screenMargin = 50.0f;
  float bounceFactorWall = 0.5f;
  float bounceFactorCorner = 0.8f;
  float cornerMargin = 100.0f;
  float dragMinDt = 0.001f;
  float dragVelocityThreshold = 10.0f;
  float dragRotationSpeed = 12.0f;
  float isoScaleX = 1.3f;
  float isoScaleY = 0.4f;
  float edgeAvoidMargin = 40.0f;
  float edgeLookAheadSpeed = 0.4f;
  float edgeLookAheadBase = 30.0f;
  float edgeAvoidForce = 3.0f;
  float screenClampTight = 20.0f;
  float screenClampExpanded = 50.0f;
  float screenClampBounce = 50.0f;
  float steerSeekForce = 4.0f;
  float curveFadeDistance = 200.0f;
  float curveTangentForce = 0.8f;
  float curveFadeMinVel = 10.0f;
  float directionRotateMinVel = 1.0f;
  float directionToCursorDist = 2.0f;
  float snapDistance = 1.0f;
  float directionReverseMultiplier = -1.0f;
  float minValidScale = 0.01f;
};

struct SpawnConfig {
  float marginX = 200.0f;
  float marginY = 100.0f;
  float targetReachedThresholdReturn = 60.0f;
  float targetReachedThresholdNormal = 30.0f;
  float targetReachedMinReturn = 50.0f;
  float targetReachedMinNormal = 25.0f;
  float randomTargetMarginX = 100.0f;
  float randomTargetMarginY = 100.0f;
  float itemDropMarginX = 300.0f;
  float itemDropMarginY = 200.0f;
  float itemPickupDistance = 28.0f;
  float catchThresholdBase = 22.0f;
  float catchThresholdMin = 15.0f;
  float wanderTargetMargin = 400.0f;
  float wanderTargetOffset = 200.0f;
  float fetchEdgeMargin = 80.0f;
  float maxFetchingGeese = 3;
  float separationMaxDistance = 70.0f;
  float separationForceMultiplier = 1.5f;
  float separationMinDistance = 0.1f;
};

struct RigConfig {
  float underbodyY = 9.0f;
  float bodyY = 14.0f;
  float neckBaseX = 15.0f;
  float neckHeightIdle = 20.0f;
  float neckHeightMoving = 10.0f;
  float neckExtIdle = 3.0f;
  float neckExtMoving = 16.0f;
  float headBaseX = 3.0f;
  float headBaseY = 2.0f;
  float head1OffsetX = 2.0f;
  float head1OffsetY = 4.0f;
  float head1OffsetZ = 3.0f;
  float head2OffsetX = 4.0f;
  float head2OffsetY = -2.0f;
  float beakBaseOffset = 4.0f;
  float beakLen = 12.0f;
  float beakWidth = 16.0f;
  float neckLerpRate = 0.1f;
  float runSpeedThreshold = 150.0f;
  float strideMax = 15.0f;
  float footSpacing = 8.0f;
  float stepLiftHeight = 3.0f;
  float footOffsetY = 2.0f;
  float headForwardBias = 2.0f;
};

struct CursorConfig {
  bool chaseEnabled = true;
  int chaseChance = 1;
  bool multiMonitorEnabled = true;
};

struct SnatchConfig {
  float radiusBase = 40.0f;
  float radiusRange = 80.0f;
  float angularSpeedBase = 1.5f;
  float speedMultiplier = 1.25f;
  float offsetMax = 120.0f;
  float duration = 3.0f;
  float pullDistance = 140.0f;
  float lateralBiasLimit = 0.75f;
  float forwardBiasScale = 0.25f;
  float forwardBiasMin = -0.35f;
  float forwardBiasMax = 0.15f;
  float tier3MaxStep = 500.0f;
  float tier3MinDelta = 1.0f;
  float angularSpeedRandomRange = 200;
};

struct MudConfig {
  bool enabled = true;
  int chance = 15;
  float lifetime = 15.0f;
};

struct HonkConfig {
  float minGap = 0.60;
  float idleMin = 6.0;
  float idleMax = 14.0;
  float genericCooldown = 0.90;
  float chaseCooldown = 1.80;
  float fetchCooldown = 1.20;
  float idleCheckAhead = 2.0;
  int idleChanceDivisor = 3;
  int wanderHonkDivisor = 15;
};

struct StepConfig {
  float timeFetch = 0.15f;
  float timeWander = 0.2f;
  float timeSnatch = 0.12f;
  float stepTriggerWalk = 5.0f;
  float stepTriggerRun = 9.0f;
  float overshootWalk = 1.5f;
  float overshootRun = 4.0f;
  float durationWalk = 0.16f;
  float durationRun = 0.085f;
  float liftWalk = 3.0f;
  float liftRun = 7.0f;
  float snapDistance = 90.0f;
  float distFactorBase = 22.0f;
  float distFactorMin = 0.9f;
  float distFactorMax = 1.45f;
  float durationMin = 0.055f;
  float durationMax = 0.18f;
  float leftFootAngle = -15.0f;
  float rightFootAngle = 15.0f;
  float footSpacing = 4.0f;
  float zeroVelocityThreshold = 0.001f;
  float minDuration = 0.001f;
};

struct ItemConfig {
  float pickupCooldown = 2.0f;
  float itemLifetime = 15.0f;
  float memeLifetime = 30.0f;
  float textLifetime = 30.0f;
  int maxDroppedMemes = 20;
  int maxDroppedTexts = 20;
  int memePickupChance = 10;
  int fetchBaseChance = 2;
  int maxFetchBias = 100;
  int maxFetchGeese = 3;
  int memeFetchBiasMax = 30;
  int noteFetchBiasMax = 20;
  int attackMouseBiasMax = 25;
  int heistChancePercent = 10;
  int heistApproachMargin = 8;
  float fetchCooldown = 4.0f;
};

struct ColorRGB {
  float r, g, b;
};

struct ColorRGBA {
  float r, g, b, a;
};

struct RenderConfig {
  float shadowOffsetX = 2.0f;
  float shadowOffsetY = 10.0f;
  float shadowWidth = 40.0f;
  float shadowHeight = 30.0f;
  float footSize = 8.0f;
  float bodyWidth = 22.0f;
  float bodyHeight = 22.0f;
  float neckSize = 13.0f;
  float head1Size = 15.0f;
  float head2Size = 10.0f;
  float beakWidth = 16.0f;
  float beakHeight = 8.0f;
  float beakMaxWidth = 9.0f;
  float eyeSize = 4.0f;
  float eyeOffsetXBack = -3.0f;
  float eyeOffsetXFront = 3.0f;
  float eyeOffsetY = -4.0f;
  float eyeFacingThreshold = 0.5f;
  float clickRadius = 30.0f;
  float footprintWidth = 12.0f;
  float footprintHeight = 8.0f;
  float droppedItemSize = 20.0f;
  float frameRate = 60.0f;
  float frameDt = 0.016667f;
  int debugTickMod = 60;
  float isoScaleX = 1.0f;
  float isoScaleY = 0.7f;
  float squashFactor = 0.92f;
  float facingBackThreshold = 0.55f;

  // Dropped item UI
  float closeButtonSize = 20.0f;
  float closeButtonMargin = 16.0f;
  float textNoteFontSize = 14.0f;
  float textNotePadding = 5.0f;
  ColorRGB memePlaceholderColor = {0.9f, 0.7f, 0.9f};
  ColorRGB closeButtonColor = {0.9f, 0.1f, 0.1f};
  ColorRGB closeButtonStroke = {1.0f, 1.0f, 1.0f};
};

struct ColorConfig {
  ColorRGB goose = {0.82f, 0.82f, 0.82f};
  ColorRGB beak = {1.0f, 0.6f, 0.0f};
  ColorRGB eye = {0.1f, 0.1f, 0.1f};
  ColorRGBA eyeHighlight = {1.0f, 0.8f, 0.0f, 0.8f};
  ColorRGB shadow = {0.0f, 0.0f, 0.0f};
  ColorRGB footprint = {0.3f, 0.3f, 0.3f};
  float footprintAlphaMultiplier = 0.6f;
  ColorRGB droppedItem = {0.5f, 0.5f, 0.5f};
  // Canada Goose colors
  ColorRGB canadaHead = {0.82f, 0.82f, 0.82f};
  ColorRGB canadaNeck = {0.05f, 0.05f, 0.05f};
  ColorRGB canadaBody = {0.35f, 0.28f, 0.22f};
  ColorRGB canadaOutline = {0.15f, 0.12f, 0.1f};
  ColorRGB canadaBeak = {0.22f, 0.22f, 0.22f};
  ColorRGB canadaEye = {0.1f, 0.1f, 0.1f};
  // Custom appearance colors (used when appearanceMode == APPEARANCE_CUSTOM)
  ColorRGB customBody = {0.82f, 0.82f, 0.82f};
  ColorRGB customNeck = {0.82f, 0.82f, 0.82f};
  ColorRGB customHead = {0.82f, 0.82f, 0.82f};
  ColorRGB customBeak = {1.0f, 0.6f, 0.0f};
  ColorRGB customEye = {0.1f, 0.1f, 0.1f};
  ColorRGB customOutline = {0.82f, 0.82f, 0.82f};
  // Active theme colors (dynamically updated)
  ColorRGB currentBody = {0.82f, 0.82f, 0.82f};
  ColorRGB currentNeck = {0.82f, 0.82f, 0.82f};
  ColorRGB currentHead = {0.82f, 0.82f, 0.82f};
  ColorRGB currentBeak = {1.0f, 0.6f, 0.0f};
  ColorRGB currentEye = {0.1f, 0.1f, 0.1f};
  ColorRGB currentOutline = {0.82f, 0.82f, 0.82f};
};

struct BehaviorConfig {
  struct Fun {
    bool ball = false;
    bool breadCrumbs = false;
        bool hats = false;
    bool rainbow = false;
    bool acid = false;
    bool anger = false;
    bool autumnLeaves = true;
    bool avoidance = true;
    bool boredom = false;
    bool peeking = true;
    bool affirmations = false;
    bool interactiveDrops = false;
    bool toysEnabled = true;
  } fun;

  struct Control {
    bool honcker = false;
    bool jail = false;
    bool portals = false;
    bool drag = false;
  } control;

  struct Info {
    bool nametag = false;
    bool presence = false;
    bool configGUI = false;
    bool visible = true;  // goose window visibility
  } info;

  struct Systems {
    bool health = false;
    bool ai = false;
    bool pomodoro = false;
  } systems;

  struct HonckerConfig { std::string hotkey = "f"; float size = 40.0f; float cooldown = 0.5f; } honcker;
  struct DragConfig { float radius = 45.0f; } drag;
  struct JailConfig { std::string hotkeyO = "o"; std::string hotkeyP = "p"; float size = 150.0f; } jail;
  struct NametagConfig { float size = 14.0f; } nametag;
  struct PresenceConfig { float interval = 1.0f; } presence;
  struct HealthConfig { float opacity = 0.8f; float maxHealth = 100.0f; float regenRate = 0.5f; float damageCooldown = 2.0f; float damageSpeedThreshold = 200.0f; float damageAmount = 5.0f; } health;
  struct AngerConfig { float increaseRate = 15.0f; float decreaseRate = 8.0f; float punchCooldown = 2.0; float punchDuration = 0.3f; float cursorRadius = 100.0f; float maxAnger = 100.0f; float punchThreshold = 80.0f; float minVisualThreshold = 10.0f; } anger;
  struct AcidConfig { float spinSpeed = 720.0f; float honkInterval = 0.15f; float rotationTotal = 1080.0f; int triggerChance = 300; } acid;
  struct RainbowConfig { float hueSpeed = 120.0f; } rainbow;
  struct PomodoroConfig {
    bool enabled = false;
    int workMinutes = 25;
    int breakMinutes = 5;
    int longBreakMinutes = 15;
    int sessionsBeforeLongBreak = 4;
    bool enableAggressiveMode = true;
    float aggressiveHonkInterval = 2.0f;
    float aggressiveSpeedMultiplier = 1.5f;
  } pomodoro;
  struct BallConfig {
    int count = 5;
    float size = 25.0f;
    float speed = 300.0f;
    float friction = 0.98f;
    float spawnChance = 300.0f;
    float spawnRange = 100.0f;
    float interactionRadius = 30.0f;
    float bounceFactor = 0.8f;
    bool soccerEnabled = true;
    bool beachEnabled = true;
    float soccerSize = 28.0f;
    float beachSize = 35.0f;
    float soccerSpeed = 350.0f;
    float beachSpeed = 200.0f;
    float soccerBounce = 0.75f;
    float beachBounce = 0.65f;
  } ball;
  struct BreadCrumbsConfig { int maxCrumbs = 50; float lifetime = 10.0f; float spawnDist = 15.0f; float size = 5.0f; std::string hotkey = "right shift"; } breadCrumbs;
  struct HatsConfig { std::string path; float sizeX = 32.0f; float sizeY = 24.0f; float offsetX = 0.0f; float offsetY = -15.0f; } hats;
  struct AffirmationsConfig {
    bool enabled = false;
    float interval = 300.0f;
    std::string customMessage;
  } affirmations;
  struct InteractiveDropsConfig {
    float puddleLifetime = 30.0f;
    float flowerGrowTime = 10.0f;
    float dropInterval = 120.0f;
  } interactiveDrops;
};

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
