#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

enum ConfigType { CFG_BOOL, CFG_INT, CFG_FLOAT, CFG_STRING };

struct ConfigOption {
  const char *section;
  const char *key;
  const char *label;
  ConfigType type;
  void *ptr;
  float min;
  float max;
  float step;
  const char *suffix;
  void (*onChange)() = nullptr;
};

struct DebugConfig {
  bool toTerminal = false;
  bool visuals = false;
};

struct GeneralConfig {
  float globalScale = 1.0f;
  bool audioEnabled = true;
  bool memesEnabled = true;
  bool canadaGooseMode = false;
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
  float maxForce = 350.0f;
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
  float steerSeekForce = 2.0f;
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
  float fetchEdgeMargin = 40.0f;
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
  int memePickupChance = 10;
  int fetchBaseChance = 2;
  int maxFetchBias = 100;
  int maxFetchGeese = 3;
  int memeFetchBiasMax = 30;
  int noteFetchBiasMax = 20;
  int attackMouseBiasMax = 25;
  int heistChancePercent = 10;
  int heistApproachMargin = 8;
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
};

struct ColorRGB {
  float r, g, b;
};

struct ColorRGBA {
  float r, g, b, a;
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
};

extern Config g_config;
extern double g_time;
extern std::vector<ConfigOption> g_configRegistry;

void Config_InitRegistry();
std::string Config_GetPath();
const ConfigOption *Config_FindOptionByKey(const std::string &key);
bool Config_GetValueByKey(const std::string &key, std::string *valueOut,
                          std::string *errorOut = nullptr);
bool Config_SetValueByKey(const std::string &key, const std::string &value,
                          std::string *errorOut = nullptr);
bool Config_SaveNow(std::string *errorOut = nullptr);

#endif // CONFIG_H
