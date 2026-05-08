#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <toml.hpp>
#include "config.h"

namespace fs = std::filesystem;

namespace config_helpers {
inline bool get_bool(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, bool& dest) {
    auto section_it = config.as_table().find(section);
    if (section_it == config.as_table().end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = section_tbl.find(key);
    if (key_it == section_tbl.end()) return false;
    dest = toml::get<bool>(key_it->second);
    return true;
}

inline bool get_int(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, int& dest) {
    auto section_it = config.as_table().find(section);
    if (section_it == config.as_table().end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = section_tbl.find(key);
    if (key_it == section_tbl.end()) return false;
    dest = toml::get<int>(key_it->second);
    return true;
}

inline bool get_float(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, float& dest) {
    auto section_it = config.as_table().find(section);
    if (section_it == config.as_table().end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = section_tbl.find(key);
    if (key_it == section_tbl.end()) return false;
    dest = toml::get<float>(key_it->second);
    return true;
}

inline bool get_string(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, std::string& dest) {
    auto section_it = config.as_table().find(section);
    if (section_it == config.as_table().end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = section_tbl.find(key);
    if (key_it == section_tbl.end()) return false;
    dest = toml::get<std::string>(key_it->second);
    return true;
}

inline bool get_color_rgb(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& subgroup, const std::string& colorName, float& r, float& g, float& b) {
    try {
        auto& section_tbl = toml::get<toml::table>(config.at(section));
        auto& subgroup_tbl = toml::get<toml::table>(section_tbl.at(subgroup));
        auto& color_tbl = toml::get<toml::table>(subgroup_tbl.at(colorName));
        r = toml::get<float>(color_tbl.at("r"));
        g = toml::get<float>(color_tbl.at("g"));
        b = toml::get<float>(color_tbl.at("b"));
        return true;
    } catch (...) {
        return false;
    }
}

inline bool get_color_rgba(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& subgroup, const std::string& colorName, float& r, float& g, float& b, float& a) {
    try {
        auto& section_tbl = toml::get<toml::table>(config.at(section));
        auto& subgroup_tbl = toml::get<toml::table>(section_tbl.at(subgroup));
        auto& color_tbl = toml::get<toml::table>(subgroup_tbl.at(colorName));
        r = toml::get<float>(color_tbl.at("r"));
        g = toml::get<float>(color_tbl.at("g"));
        b = toml::get<float>(color_tbl.at("b"));
        a = toml::get<float>(color_tbl.at("a"));
        return true;
    } catch (...) {
        return false;
    }
}
}

Config g_config;
double g_time = 0.0;
std::vector<ConfigOption> g_configRegistry;

static fs::path ConfigDirPath() {
    if (fs::exists(fs::current_path() / "config" / "config.toml")) {
        return fs::current_path() / "config";
    }

#if defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        if (*home) return fs::path(home) / "Library" / "Application Support" / "CadGoose";
    }
    return fs::current_path();
#else
    if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdgConfig) return fs::path(xdgConfig) / "desktop-goose";
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

    if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
        *outValue = true;
        return true;
    }
    if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
        *outValue = false;
        return true;
    }
    return false;
}

static std::string OptionValueToString(const ConfigOption& opt) {
    if (opt.type == CFG_BOOL) return *(bool*)opt.ptr ? "1" : "0";
    if (opt.type == CFG_INT) return std::to_string(*(int*)opt.ptr);
    if (opt.type == CFG_FLOAT) {
        std::ostringstream stream;
        stream << *(float*)opt.ptr;
        return stream.str();
    }
    return *(std::string*)opt.ptr;
}

static const ConfigOption* FindOptionByAnyKey(const std::string& key) {
    for (const auto& opt : g_configRegistry) {
        if (key == opt.key || key == opt.label) return &opt;
    }
    return nullptr;
}

const ConfigOption* Config_FindOptionByKey(const std::string& key) {
    for (const auto& opt : g_configRegistry) {
        if (key == opt.key) return &opt;
    }
    return nullptr;
}

static bool AssignValueToOption(const ConfigOption& opt, const std::string& rawValue, std::string* errorOut) {
    try {
        if (opt.type == CFG_BOOL) {
            bool parsed = false;
            if (!ParseBoolValue(Trim(rawValue), &parsed)) {
                if (errorOut) *errorOut = "Expected a boolean value for " + std::string(opt.key);
                return false;
            }
            *(bool*)opt.ptr = parsed;
            return true;
        }

        if (opt.type == CFG_INT) {
            *(int*)opt.ptr = std::stoi(Trim(rawValue));
            return true;
        }

        if (opt.type == CFG_FLOAT) {
            *(float*)opt.ptr = std::stof(Trim(rawValue));
            return true;
        }

        *(std::string*)opt.ptr = rawValue;
        return true;
    } catch (const std::exception&) {
        if (errorOut) *errorOut = "Invalid value for " + std::string(opt.key);
        return false;
    }
}

bool Config_SaveNow(std::string* errorOut) {
    std::error_code ec;
    fs::create_directories(ConfigDirPath(), ec);
    if (ec) {
        if (errorOut) *errorOut = "Unable to create config directory";
        return false;
    }

    std::ofstream ofs(Config_GetPath());
    if (!ofs.is_open()) {
        if (errorOut) *errorOut = "Unable to open config file for writing";
        return false;
    }

    toml::value config = toml::table{
        {"debug", toml::table{
            {"toTerminal", g_config.debug.toTerminal},
            {"visuals", g_config.debug.visuals}
        }},
        {"general", toml::table{
            {"globalScale", g_config.general.globalScale},
            {"audioEnabled", g_config.general.audioEnabled},
            {"memesEnabled", g_config.general.memesEnabled},
            {"canadaGooseMode", g_config.general.canadaGooseMode}
        }},
        {"screen", toml::table{
            {"defaultWidth", g_config.screen.defaultWidth},
            {"defaultHeight", g_config.screen.defaultHeight}
        }},
        {"asset", toml::table{
            {"memePlaceholderW", g_config.asset.memePlaceholderW},
            {"memePlaceholderH", g_config.asset.memePlaceholderH},
            {"textPlaceholderW", g_config.asset.textPlaceholderW},
            {"textPlaceholderH", g_config.asset.textPlaceholderH},
            {"notePlaceholderW", g_config.asset.notePlaceholderW},
            {"notePlaceholderH", g_config.asset.notePlaceholderH}
        }},
        {"movement", toml::table{
            {"baseWalkSpeed", g_config.movement.baseWalkSpeed},
            {"baseRunSpeed", g_config.movement.baseRunSpeed},
            {"maxForce", g_config.movement.maxForce},
            {"moveDistanceThreshold", g_config.movement.moveDistanceThreshold},
            {"friction", g_config.movement.friction},
            {"maxSpeed", g_config.movement.maxSpeed},
            {"runSpeedMultiplier", g_config.movement.runSpeedMultiplier},
            {"initDirectionMax", g_config.movement.initDirectionMax},
            {"speedLerpRate", g_config.movement.speedLerpRate},
            {"arrivalRadius", g_config.movement.arrivalRadius},
            {"runDistanceThreshold", g_config.movement.runDistanceThreshold},
            {"directionBlendRate", g_config.movement.directionBlendRate}
        }},
        {"physics", toml::table{
            {"screenMargin", g_config.physics.screenMargin},
            {"bounceFactorWall", g_config.physics.bounceFactorWall},
            {"bounceFactorCorner", g_config.physics.bounceFactorCorner},
            {"cornerMargin", g_config.physics.cornerMargin},
            {"dragMinDt", g_config.physics.dragMinDt},
            {"dragVelocityThreshold", g_config.physics.dragVelocityThreshold},
            {"dragRotationSpeed", g_config.physics.dragRotationSpeed},
            {"isoScaleX", g_config.physics.isoScaleX},
            {"isoScaleY", g_config.physics.isoScaleY},
            {"edgeAvoidMargin", g_config.physics.edgeAvoidMargin},
            {"edgeLookAheadSpeed", g_config.physics.edgeLookAheadSpeed},
            {"edgeLookAheadBase", g_config.physics.edgeLookAheadBase},
            {"edgeAvoidForce", g_config.physics.edgeAvoidForce},
            {"screenClampTight", g_config.physics.screenClampTight},
            {"screenClampExpanded", g_config.physics.screenClampExpanded},
            {"screenClampBounce", g_config.physics.screenClampBounce},
            {"steerSeekForce", g_config.physics.steerSeekForce},
            {"curveFadeDistance", g_config.physics.curveFadeDistance},
            {"curveTangentForce", g_config.physics.curveTangentForce},
            {"curveFadeMinVel", g_config.physics.curveFadeMinVel},
            {"directionRotateMinVel", g_config.physics.directionRotateMinVel},
            {"directionToCursorDist", g_config.physics.directionToCursorDist},
            {"snapDistance", g_config.physics.snapDistance},
            {"directionReverseMultiplier", g_config.physics.directionReverseMultiplier},
            {"minValidScale", g_config.physics.minValidScale}
        }},
        {"spawn", toml::table{
            {"marginX", g_config.spawn.marginX},
            {"marginY", g_config.spawn.marginY},
            {"targetReachedThresholdReturn", g_config.spawn.targetReachedThresholdReturn},
            {"targetReachedThresholdNormal", g_config.spawn.targetReachedThresholdNormal},
            {"targetReachedMinReturn", g_config.spawn.targetReachedMinReturn},
            {"targetReachedMinNormal", g_config.spawn.targetReachedMinNormal},
            {"randomTargetMarginX", g_config.spawn.randomTargetMarginX},
            {"randomTargetMarginY", g_config.spawn.randomTargetMarginY},
            {"itemDropMarginX", g_config.spawn.itemDropMarginX},
            {"itemDropMarginY", g_config.spawn.itemDropMarginY},
            {"itemPickupDistance", g_config.spawn.itemPickupDistance},
            {"catchThresholdBase", g_config.spawn.catchThresholdBase},
            {"catchThresholdMin", g_config.spawn.catchThresholdMin},
            {"wanderTargetMargin", g_config.spawn.wanderTargetMargin},
            {"wanderTargetOffset", g_config.spawn.wanderTargetOffset},
            {"fetchEdgeMargin", g_config.spawn.fetchEdgeMargin},
            {"maxFetchingGeese", g_config.spawn.maxFetchingGeese},
            {"separationMaxDistance", g_config.spawn.separationMaxDistance},
            {"separationForceMultiplier", g_config.spawn.separationForceMultiplier},
            {"separationMinDistance", g_config.spawn.separationMinDistance}
        }},
        {"rig", toml::table{
            {"underbodyY", g_config.rig.underbodyY},
            {"bodyY", g_config.rig.bodyY},
            {"neckBaseX", g_config.rig.neckBaseX},
            {"neckHeightIdle", g_config.rig.neckHeightIdle},
            {"neckHeightMoving", g_config.rig.neckHeightMoving},
            {"neckExtIdle", g_config.rig.neckExtIdle},
            {"neckExtMoving", g_config.rig.neckExtMoving},
            {"headBaseX", g_config.rig.headBaseX},
            {"headBaseY", g_config.rig.headBaseY},
            {"head1OffsetX", g_config.rig.head1OffsetX},
            {"head1OffsetY", g_config.rig.head1OffsetY},
            {"head1OffsetZ", g_config.rig.head1OffsetZ},
            {"head2OffsetX", g_config.rig.head2OffsetX},
            {"head2OffsetY", g_config.rig.head2OffsetY},
            {"beakBaseOffset", g_config.rig.beakBaseOffset},
            {"beakLen", g_config.rig.beakLen},
            {"beakWidth", g_config.rig.beakWidth},
            {"neckLerpRate", g_config.rig.neckLerpRate},
            {"runSpeedThreshold", g_config.rig.runSpeedThreshold},
            {"strideMax", g_config.rig.strideMax},
            {"footSpacing", g_config.rig.footSpacing},
            {"stepLiftHeight", g_config.rig.stepLiftHeight},
            {"footOffsetY", g_config.rig.footOffsetY},
            {"headForwardBias", g_config.rig.headForwardBias}
        }},
        {"cursor", toml::table{
            {"chaseEnabled", g_config.cursor.chaseEnabled},
            {"chaseChance", g_config.cursor.chaseChance},
            {"multiMonitorEnabled", g_config.cursor.multiMonitorEnabled}
        }},
        {"snatch", toml::table{
            {"radiusBase", g_config.snatch.radiusBase},
            {"radiusRange", g_config.snatch.radiusRange},
            {"angularSpeedBase", g_config.snatch.angularSpeedBase},
            {"speedMultiplier", g_config.snatch.speedMultiplier},
            {"offsetMax", g_config.snatch.offsetMax},
            {"duration", g_config.snatch.duration},
            {"pullDistance", g_config.snatch.pullDistance},
            {"lateralBiasLimit", g_config.snatch.lateralBiasLimit},
            {"forwardBiasScale", g_config.snatch.forwardBiasScale},
            {"forwardBiasMin", g_config.snatch.forwardBiasMin},
            {"forwardBiasMax", g_config.snatch.forwardBiasMax},
            {"tier3MaxStep", g_config.snatch.tier3MaxStep},
            {"tier3MinDelta", g_config.snatch.tier3MinDelta},
            {"angularSpeedRandomRange", g_config.snatch.angularSpeedRandomRange}
        }},
        {"mud", toml::table{
            {"enabled", g_config.mud.enabled},
            {"chance", g_config.mud.chance},
            {"lifetime", g_config.mud.lifetime}
        }},
        {"honk", toml::table{
            {"minGap", g_config.honk.minGap},
            {"idleMin", g_config.honk.idleMin},
            {"idleMax", g_config.honk.idleMax},
            {"genericCooldown", g_config.honk.genericCooldown},
            {"chaseCooldown", g_config.honk.chaseCooldown},
            {"fetchCooldown", g_config.honk.fetchCooldown},
            {"idleCheckAhead", g_config.honk.idleCheckAhead},
            {"idleChanceDivisor", g_config.honk.idleChanceDivisor},
            {"wanderHonkDivisor", g_config.honk.wanderHonkDivisor}
        }},
        {"step", toml::table{
            {"timeFetch", g_config.step.timeFetch},
            {"timeWander", g_config.step.timeWander},
            {"timeSnatch", g_config.step.timeSnatch},
            {"stepTriggerWalk", g_config.step.stepTriggerWalk},
            {"stepTriggerRun", g_config.step.stepTriggerRun},
            {"overshootWalk", g_config.step.overshootWalk},
            {"overshootRun", g_config.step.overshootRun},
            {"durationWalk", g_config.step.durationWalk},
            {"durationRun", g_config.step.durationRun},
            {"liftWalk", g_config.step.liftWalk},
            {"liftRun", g_config.step.liftRun},
            {"snapDistance", g_config.step.snapDistance},
            {"distFactorBase", g_config.step.distFactorBase},
            {"distFactorMin", g_config.step.distFactorMin},
            {"distFactorMax", g_config.step.distFactorMax},
            {"durationMin", g_config.step.durationMin},
            {"durationMax", g_config.step.durationMax},
            {"leftFootAngle", g_config.step.leftFootAngle},
            {"rightFootAngle", g_config.step.rightFootAngle},
            {"footSpacing", g_config.step.footSpacing},
            {"zeroVelocityThreshold", g_config.step.zeroVelocityThreshold},
            {"minDuration", g_config.step.minDuration}
        }},
        {"item", toml::table{
            {"pickupCooldown", g_config.item.pickupCooldown},
            {"itemLifetime", g_config.item.itemLifetime},
            {"memePickupChance", g_config.item.memePickupChance},
            {"fetchBaseChance", g_config.item.fetchBaseChance},
            {"maxFetchBias", g_config.item.maxFetchBias},
            {"maxFetchGeese", g_config.item.maxFetchGeese},
            {"memeFetchBiasMax", g_config.item.memeFetchBiasMax},
            {"noteFetchBiasMax", g_config.item.noteFetchBiasMax},
            {"attackMouseBiasMax", g_config.item.attackMouseBiasMax},
            {"heistChancePercent", g_config.item.heistChancePercent},
            {"heistApproachMargin", g_config.item.heistApproachMargin}
        }},
        {"render", toml::table{
            {"shadowOffsetX", g_config.render.shadowOffsetX},
            {"shadowOffsetY", g_config.render.shadowOffsetY},
            {"shadowWidth", g_config.render.shadowWidth},
            {"shadowHeight", g_config.render.shadowHeight},
            {"footSize", g_config.render.footSize},
            {"bodyWidth", g_config.render.bodyWidth},
            {"bodyHeight", g_config.render.bodyHeight},
            {"neckSize", g_config.render.neckSize},
            {"head1Size", g_config.render.head1Size},
            {"head2Size", g_config.render.head2Size},
            {"beakWidth", g_config.render.beakWidth},
            {"beakHeight", g_config.render.beakHeight},
            {"beakMaxWidth", g_config.render.beakMaxWidth},
            {"eyeSize", g_config.render.eyeSize},
            {"eyeOffsetXBack", g_config.render.eyeOffsetXBack},
            {"eyeOffsetXFront", g_config.render.eyeOffsetXFront},
            {"eyeOffsetY", g_config.render.eyeOffsetY},
            {"eyeFacingThreshold", g_config.render.eyeFacingThreshold},
            {"clickRadius", g_config.render.clickRadius},
            {"footprintWidth", g_config.render.footprintWidth},
            {"footprintHeight", g_config.render.footprintHeight},
            {"droppedItemSize", g_config.render.droppedItemSize},
            {"frameRate", g_config.render.frameRate},
            {"frameDt", g_config.render.frameDt},
            {"debugTickMod", g_config.render.debugTickMod},
            {"isoScaleX", g_config.render.isoScaleX},
            {"isoScaleY", g_config.render.isoScaleY},
            {"squashFactor", g_config.render.squashFactor},
            {"facingBackThreshold", g_config.render.facingBackThreshold},
            {"closeButtonSize", g_config.render.closeButtonSize},
            {"closeButtonMargin", g_config.render.closeButtonMargin},
            {"textNoteFontSize", g_config.render.textNoteFontSize},
            {"textNotePadding", g_config.render.textNotePadding}
        }},
        {"color", toml::table{
            {"goose", toml::table{
                {"r", g_config.color.goose.r},
                {"g", g_config.color.goose.g},
                {"b", g_config.color.goose.b}
            }},
            {"beak", toml::table{
                {"r", g_config.color.beak.r},
                {"g", g_config.color.beak.g},
                {"b", g_config.color.beak.b}
            }},
            {"eye", toml::table{
                {"r", g_config.color.eye.r},
                {"g", g_config.color.eye.g},
                {"b", g_config.color.eye.b}
            }},
            {"eyeHighlight", toml::table{
                {"r", g_config.color.eyeHighlight.r},
                {"g", g_config.color.eyeHighlight.g},
                {"b", g_config.color.eyeHighlight.b},
                {"a", g_config.color.eyeHighlight.a}
            }},
            {"shadow", toml::table{
                {"r", g_config.color.shadow.r},
                {"g", g_config.color.shadow.g},
                {"b", g_config.color.shadow.b}
            }},
            {"footprint", toml::table{
                {"r", g_config.color.footprint.r},
                {"g", g_config.color.footprint.g},
                {"b", g_config.color.footprint.b}
            }},
            {"footprintAlphaMultiplier", g_config.color.footprintAlphaMultiplier},
            {"droppedItem", toml::table{
                {"r", g_config.color.droppedItem.r},
                {"g", g_config.color.droppedItem.g},
                {"b", g_config.color.droppedItem.b}
            }},
            {"canadaHead", toml::table{
                {"r", g_config.color.canadaHead.r},
                {"g", g_config.color.canadaHead.g},
                {"b", g_config.color.canadaHead.b}
            }},
            {"canadaNeck", toml::table{
                {"r", g_config.color.canadaNeck.r},
                {"g", g_config.color.canadaNeck.g},
                {"b", g_config.color.canadaNeck.b}
            }},
            {"canadaBody", toml::table{
                {"r", g_config.color.canadaBody.r},
                {"g", g_config.color.canadaBody.g},
                {"b", g_config.color.canadaBody.b}
            }},
            {"canadaOutline", toml::table{
                {"r", g_config.color.canadaOutline.r},
                {"g", g_config.color.canadaOutline.g},
                {"b", g_config.color.canadaOutline.b}
            }},
            {"canadaBeak", toml::table{
                {"r", g_config.color.canadaBeak.r},
                {"g", g_config.color.canadaBeak.g},
                {"b", g_config.color.canadaBeak.b}
            }},
            {"canadaEye", toml::table{
                {"r", g_config.color.canadaEye.r},
                {"g", g_config.color.canadaEye.g},
                {"b", g_config.color.canadaEye.b}
            }}
        }}
    };

    ofs << config << "\n";
    return true;
}

static void Config_Load() {
    std::vector<fs::path> candidatePaths = {
        fs::path(Config_GetPath()),
        fs::current_path() / "config.toml",
        fs::current_path() / "config.ini",
    };

    for (const auto& path : candidatePaths) {
        if (!fs::exists(path)) continue;
        try {
            auto config = toml::parse(path.string());

            config_helpers::get_bool(config, "debug", "toTerminal", g_config.debug.toTerminal);
            config_helpers::get_bool(config, "debug", "visuals", g_config.debug.visuals);
            config_helpers::get_float(config, "general", "globalScale", g_config.general.globalScale);
            config_helpers::get_bool(config, "general", "audioEnabled", g_config.general.audioEnabled);
            config_helpers::get_bool(config, "general", "memesEnabled", g_config.general.memesEnabled);
            config_helpers::get_bool(config, "general", "canadaGooseMode", g_config.general.canadaGooseMode);
            config_helpers::get_int(config, "screen", "defaultWidth", g_config.screen.defaultWidth);
            config_helpers::get_int(config, "screen", "defaultHeight", g_config.screen.defaultHeight);
            config_helpers::get_int(config, "asset", "memePlaceholderW", g_config.asset.memePlaceholderW);
            config_helpers::get_int(config, "asset", "memePlaceholderH", g_config.asset.memePlaceholderH);
            config_helpers::get_int(config, "asset", "textPlaceholderW", g_config.asset.textPlaceholderW);
            config_helpers::get_int(config, "asset", "textPlaceholderH", g_config.asset.textPlaceholderH);
            config_helpers::get_int(config, "asset", "notePlaceholderW", g_config.asset.notePlaceholderW);
            config_helpers::get_int(config, "asset", "notePlaceholderH", g_config.asset.notePlaceholderH);

            config_helpers::get_float(config, "movement", "baseWalkSpeed", g_config.movement.baseWalkSpeed);
            config_helpers::get_float(config, "movement", "baseRunSpeed", g_config.movement.baseRunSpeed);
            config_helpers::get_float(config, "movement", "maxForce", g_config.movement.maxForce);
            config_helpers::get_float(config, "movement", "moveDistanceThreshold", g_config.movement.moveDistanceThreshold);
            config_helpers::get_float(config, "movement", "friction", g_config.movement.friction);
            config_helpers::get_float(config, "movement", "maxSpeed", g_config.movement.maxSpeed);
            config_helpers::get_float(config, "movement", "runSpeedMultiplier", g_config.movement.runSpeedMultiplier);
            config_helpers::get_float(config, "movement", "initDirectionMax", g_config.movement.initDirectionMax);
            config_helpers::get_float(config, "movement", "speedLerpRate", g_config.movement.speedLerpRate);
            config_helpers::get_float(config, "movement", "arrivalRadius", g_config.movement.arrivalRadius);
            config_helpers::get_float(config, "movement", "runDistanceThreshold", g_config.movement.runDistanceThreshold);
            config_helpers::get_float(config, "movement", "directionBlendRate", g_config.movement.directionBlendRate);

            config_helpers::get_float(config, "physics", "screenMargin", g_config.physics.screenMargin);
            config_helpers::get_float(config, "physics", "bounceFactorWall", g_config.physics.bounceFactorWall);
            config_helpers::get_float(config, "physics", "bounceFactorCorner", g_config.physics.bounceFactorCorner);
            config_helpers::get_float(config, "physics", "cornerMargin", g_config.physics.cornerMargin);
            config_helpers::get_float(config, "physics", "dragMinDt", g_config.physics.dragMinDt);
            config_helpers::get_float(config, "physics", "dragVelocityThreshold", g_config.physics.dragVelocityThreshold);
            config_helpers::get_float(config, "physics", "dragRotationSpeed", g_config.physics.dragRotationSpeed);
            config_helpers::get_float(config, "physics", "isoScaleX", g_config.physics.isoScaleX);
            config_helpers::get_float(config, "physics", "isoScaleY", g_config.physics.isoScaleY);
            config_helpers::get_float(config, "physics", "edgeAvoidMargin", g_config.physics.edgeAvoidMargin);
            config_helpers::get_float(config, "physics", "edgeLookAheadSpeed", g_config.physics.edgeLookAheadSpeed);
            config_helpers::get_float(config, "physics", "edgeLookAheadBase", g_config.physics.edgeLookAheadBase);
            config_helpers::get_float(config, "physics", "edgeAvoidForce", g_config.physics.edgeAvoidForce);
            config_helpers::get_float(config, "physics", "screenClampTight", g_config.physics.screenClampTight);
            config_helpers::get_float(config, "physics", "screenClampExpanded", g_config.physics.screenClampExpanded);
            config_helpers::get_float(config, "physics", "screenClampBounce", g_config.physics.screenClampBounce);
            config_helpers::get_float(config, "physics", "steerSeekForce", g_config.physics.steerSeekForce);
            config_helpers::get_float(config, "physics", "curveFadeDistance", g_config.physics.curveFadeDistance);
            config_helpers::get_float(config, "physics", "curveTangentForce", g_config.physics.curveTangentForce);
            config_helpers::get_float(config, "physics", "curveFadeMinVel", g_config.physics.curveFadeMinVel);
            config_helpers::get_float(config, "physics", "directionRotateMinVel", g_config.physics.directionRotateMinVel);
            config_helpers::get_float(config, "physics", "directionToCursorDist", g_config.physics.directionToCursorDist);
            config_helpers::get_float(config, "physics", "snapDistance", g_config.physics.snapDistance);
            config_helpers::get_float(config, "physics", "directionReverseMultiplier", g_config.physics.directionReverseMultiplier);
            config_helpers::get_float(config, "physics", "minValidScale", g_config.physics.minValidScale);

            config_helpers::get_float(config, "spawn", "marginX", g_config.spawn.marginX);
            config_helpers::get_float(config, "spawn", "marginY", g_config.spawn.marginY);
            config_helpers::get_float(config, "spawn", "targetReachedThresholdReturn", g_config.spawn.targetReachedThresholdReturn);
            config_helpers::get_float(config, "spawn", "targetReachedThresholdNormal", g_config.spawn.targetReachedThresholdNormal);
            config_helpers::get_float(config, "spawn", "targetReachedMinReturn", g_config.spawn.targetReachedMinReturn);
            config_helpers::get_float(config, "spawn", "targetReachedMinNormal", g_config.spawn.targetReachedMinNormal);
            config_helpers::get_float(config, "spawn", "randomTargetMarginX", g_config.spawn.randomTargetMarginX);
            config_helpers::get_float(config, "spawn", "randomTargetMarginY", g_config.spawn.randomTargetMarginY);
            config_helpers::get_float(config, "spawn", "itemDropMarginX", g_config.spawn.itemDropMarginX);
            config_helpers::get_float(config, "spawn", "itemDropMarginY", g_config.spawn.itemDropMarginY);
            config_helpers::get_float(config, "spawn", "itemPickupDistance", g_config.spawn.itemPickupDistance);
            config_helpers::get_float(config, "spawn", "catchThresholdBase", g_config.spawn.catchThresholdBase);
            config_helpers::get_float(config, "spawn", "catchThresholdMin", g_config.spawn.catchThresholdMin);
            config_helpers::get_float(config, "spawn", "wanderTargetMargin", g_config.spawn.wanderTargetMargin);
            config_helpers::get_float(config, "spawn", "wanderTargetOffset", g_config.spawn.wanderTargetOffset);
            config_helpers::get_float(config, "spawn", "fetchEdgeMargin", g_config.spawn.fetchEdgeMargin);
            config_helpers::get_float(config, "spawn", "maxFetchingGeese", g_config.spawn.maxFetchingGeese);
            config_helpers::get_float(config, "spawn", "separationMaxDistance", g_config.spawn.separationMaxDistance);
            config_helpers::get_float(config, "spawn", "separationForceMultiplier", g_config.spawn.separationForceMultiplier);
            config_helpers::get_float(config, "spawn", "separationMinDistance", g_config.spawn.separationMinDistance);

            config_helpers::get_float(config, "rig", "underbodyY", g_config.rig.underbodyY);
            config_helpers::get_float(config, "rig", "bodyY", g_config.rig.bodyY);
            config_helpers::get_float(config, "rig", "neckBaseX", g_config.rig.neckBaseX);
            config_helpers::get_float(config, "rig", "neckHeightIdle", g_config.rig.neckHeightIdle);
            config_helpers::get_float(config, "rig", "neckHeightMoving", g_config.rig.neckHeightMoving);
            config_helpers::get_float(config, "rig", "neckExtIdle", g_config.rig.neckExtIdle);
            config_helpers::get_float(config, "rig", "neckExtMoving", g_config.rig.neckExtMoving);
            config_helpers::get_float(config, "rig", "headBaseX", g_config.rig.headBaseX);
            config_helpers::get_float(config, "rig", "headBaseY", g_config.rig.headBaseY);
            config_helpers::get_float(config, "rig", "head1OffsetX", g_config.rig.head1OffsetX);
            config_helpers::get_float(config, "rig", "head1OffsetY", g_config.rig.head1OffsetY);
            config_helpers::get_float(config, "rig", "head1OffsetZ", g_config.rig.head1OffsetZ);
            config_helpers::get_float(config, "rig", "head2OffsetX", g_config.rig.head2OffsetX);
            config_helpers::get_float(config, "rig", "head2OffsetY", g_config.rig.head2OffsetY);
            config_helpers::get_float(config, "rig", "beakBaseOffset", g_config.rig.beakBaseOffset);
            config_helpers::get_float(config, "rig", "beakLen", g_config.rig.beakLen);
            config_helpers::get_float(config, "rig", "beakWidth", g_config.rig.beakWidth);
            config_helpers::get_float(config, "rig", "neckLerpRate", g_config.rig.neckLerpRate);
            config_helpers::get_float(config, "rig", "runSpeedThreshold", g_config.rig.runSpeedThreshold);
            config_helpers::get_float(config, "rig", "strideMax", g_config.rig.strideMax);
            config_helpers::get_float(config, "rig", "footSpacing", g_config.rig.footSpacing);
            config_helpers::get_float(config, "rig", "stepLiftHeight", g_config.rig.stepLiftHeight);
            config_helpers::get_float(config, "rig", "footOffsetY", g_config.rig.footOffsetY);
            config_helpers::get_float(config, "rig", "headForwardBias", g_config.rig.headForwardBias);

            config_helpers::get_bool(config, "cursor", "chaseEnabled", g_config.cursor.chaseEnabled);
            config_helpers::get_int(config, "cursor", "chaseChance", g_config.cursor.chaseChance);
            config_helpers::get_bool(config, "cursor", "multiMonitorEnabled", g_config.cursor.multiMonitorEnabled);

            config_helpers::get_float(config, "snatch", "radiusBase", g_config.snatch.radiusBase);
            config_helpers::get_float(config, "snatch", "radiusRange", g_config.snatch.radiusRange);
            config_helpers::get_float(config, "snatch", "angularSpeedBase", g_config.snatch.angularSpeedBase);
            config_helpers::get_float(config, "snatch", "speedMultiplier", g_config.snatch.speedMultiplier);
            config_helpers::get_float(config, "snatch", "offsetMax", g_config.snatch.offsetMax);
            config_helpers::get_float(config, "snatch", "duration", g_config.snatch.duration);
            config_helpers::get_float(config, "snatch", "pullDistance", g_config.snatch.pullDistance);
            config_helpers::get_float(config, "snatch", "lateralBiasLimit", g_config.snatch.lateralBiasLimit);
            config_helpers::get_float(config, "snatch", "forwardBiasScale", g_config.snatch.forwardBiasScale);
            config_helpers::get_float(config, "snatch", "forwardBiasMin", g_config.snatch.forwardBiasMin);
            config_helpers::get_float(config, "snatch", "forwardBiasMax", g_config.snatch.forwardBiasMax);
            config_helpers::get_float(config, "snatch", "tier3MaxStep", g_config.snatch.tier3MaxStep);
            config_helpers::get_float(config, "snatch", "tier3MinDelta", g_config.snatch.tier3MinDelta);
            config_helpers::get_float(config, "snatch", "angularSpeedRandomRange", g_config.snatch.angularSpeedRandomRange);

            config_helpers::get_bool(config, "mud", "enabled", g_config.mud.enabled);
            config_helpers::get_int(config, "mud", "chance", g_config.mud.chance);
            config_helpers::get_float(config, "mud", "lifetime", g_config.mud.lifetime);

            config_helpers::get_float(config, "honk", "minGap", g_config.honk.minGap);
            config_helpers::get_float(config, "honk", "idleMin", g_config.honk.idleMin);
            config_helpers::get_float(config, "honk", "idleMax", g_config.honk.idleMax);
            config_helpers::get_float(config, "honk", "genericCooldown", g_config.honk.genericCooldown);
            config_helpers::get_float(config, "honk", "chaseCooldown", g_config.honk.chaseCooldown);
            config_helpers::get_float(config, "honk", "fetchCooldown", g_config.honk.fetchCooldown);
            config_helpers::get_float(config, "honk", "idleCheckAhead", g_config.honk.idleCheckAhead);
            config_helpers::get_int(config, "honk", "idleChanceDivisor", g_config.honk.idleChanceDivisor);
            config_helpers::get_int(config, "honk", "wanderHonkDivisor", g_config.honk.wanderHonkDivisor);

            config_helpers::get_float(config, "step", "timeFetch", g_config.step.timeFetch);
            config_helpers::get_float(config, "step", "timeWander", g_config.step.timeWander);
            config_helpers::get_float(config, "step", "timeSnatch", g_config.step.timeSnatch);
            config_helpers::get_float(config, "step", "stepTriggerWalk", g_config.step.stepTriggerWalk);
            config_helpers::get_float(config, "step", "stepTriggerRun", g_config.step.stepTriggerRun);
            config_helpers::get_float(config, "step", "overshootWalk", g_config.step.overshootWalk);
            config_helpers::get_float(config, "step", "overshootRun", g_config.step.overshootRun);
            config_helpers::get_float(config, "step", "durationWalk", g_config.step.durationWalk);
            config_helpers::get_float(config, "step", "durationRun", g_config.step.durationRun);
            config_helpers::get_float(config, "step", "liftWalk", g_config.step.liftWalk);
            config_helpers::get_float(config, "step", "liftRun", g_config.step.liftRun);
            config_helpers::get_float(config, "step", "snapDistance", g_config.step.snapDistance);
            config_helpers::get_float(config, "step", "distFactorBase", g_config.step.distFactorBase);
            config_helpers::get_float(config, "step", "distFactorMin", g_config.step.distFactorMin);
            config_helpers::get_float(config, "step", "distFactorMax", g_config.step.distFactorMax);
            config_helpers::get_float(config, "step", "durationMin", g_config.step.durationMin);
            config_helpers::get_float(config, "step", "durationMax", g_config.step.durationMax);
            config_helpers::get_float(config, "step", "leftFootAngle", g_config.step.leftFootAngle);
            config_helpers::get_float(config, "step", "rightFootAngle", g_config.step.rightFootAngle);
            config_helpers::get_float(config, "step", "footSpacing", g_config.step.footSpacing);
            config_helpers::get_float(config, "step", "zeroVelocityThreshold", g_config.step.zeroVelocityThreshold);
            config_helpers::get_float(config, "step", "minDuration", g_config.step.minDuration);

            config_helpers::get_float(config, "item", "pickupCooldown", g_config.item.pickupCooldown);
            config_helpers::get_float(config, "item", "itemLifetime", g_config.item.itemLifetime);
            config_helpers::get_int(config, "item", "memePickupChance", g_config.item.memePickupChance);
            config_helpers::get_int(config, "item", "fetchBaseChance", g_config.item.fetchBaseChance);
            config_helpers::get_int(config, "item", "maxFetchBias", g_config.item.maxFetchBias);
            config_helpers::get_int(config, "item", "maxFetchGeese", g_config.item.maxFetchGeese);
            config_helpers::get_int(config, "item", "memeFetchBiasMax", g_config.item.memeFetchBiasMax);
            config_helpers::get_int(config, "item", "noteFetchBiasMax", g_config.item.noteFetchBiasMax);
            config_helpers::get_int(config, "item", "attackMouseBiasMax", g_config.item.attackMouseBiasMax);
            config_helpers::get_int(config, "item", "heistChancePercent", g_config.item.heistChancePercent);
            config_helpers::get_int(config, "item", "heistApproachMargin", g_config.item.heistApproachMargin);

            config_helpers::get_float(config, "render", "shadowOffsetX", g_config.render.shadowOffsetX);
            config_helpers::get_float(config, "render", "shadowOffsetY", g_config.render.shadowOffsetY);
            config_helpers::get_float(config, "render", "shadowWidth", g_config.render.shadowWidth);
            config_helpers::get_float(config, "render", "shadowHeight", g_config.render.shadowHeight);
            config_helpers::get_float(config, "render", "footSize", g_config.render.footSize);
            config_helpers::get_float(config, "render", "bodyWidth", g_config.render.bodyWidth);
            config_helpers::get_float(config, "render", "bodyHeight", g_config.render.bodyHeight);
            config_helpers::get_float(config, "render", "neckSize", g_config.render.neckSize);
            config_helpers::get_float(config, "render", "head1Size", g_config.render.head1Size);
            config_helpers::get_float(config, "render", "head2Size", g_config.render.head2Size);
            config_helpers::get_float(config, "render", "beakWidth", g_config.render.beakWidth);
            config_helpers::get_float(config, "render", "beakHeight", g_config.render.beakHeight);
            config_helpers::get_float(config, "render", "beakMaxWidth", g_config.render.beakMaxWidth);
            config_helpers::get_float(config, "render", "eyeSize", g_config.render.eyeSize);
            config_helpers::get_float(config, "render", "eyeOffsetXBack", g_config.render.eyeOffsetXBack);
            config_helpers::get_float(config, "render", "eyeOffsetXFront", g_config.render.eyeOffsetXFront);
            config_helpers::get_float(config, "render", "eyeOffsetY", g_config.render.eyeOffsetY);
            config_helpers::get_float(config, "render", "eyeFacingThreshold", g_config.render.eyeFacingThreshold);
            config_helpers::get_float(config, "render", "clickRadius", g_config.render.clickRadius);
            config_helpers::get_float(config, "render", "footprintWidth", g_config.render.footprintWidth);
            config_helpers::get_float(config, "render", "footprintHeight", g_config.render.footprintHeight);
            config_helpers::get_float(config, "render", "droppedItemSize", g_config.render.droppedItemSize);
            config_helpers::get_float(config, "render", "frameRate", g_config.render.frameRate);
            config_helpers::get_float(config, "render", "frameDt", g_config.render.frameDt);
            config_helpers::get_int(config, "render", "debugTickMod", g_config.render.debugTickMod);
            config_helpers::get_float(config, "render", "isoScaleX", g_config.render.isoScaleX);
            config_helpers::get_float(config, "render", "isoScaleY", g_config.render.isoScaleY);
            config_helpers::get_float(config, "render", "squashFactor", g_config.render.squashFactor);
            config_helpers::get_float(config, "render", "facingBackThreshold", g_config.render.facingBackThreshold);
            config_helpers::get_float(config, "render", "closeButtonSize", g_config.render.closeButtonSize);
            config_helpers::get_float(config, "render", "closeButtonMargin", g_config.render.closeButtonMargin);
            config_helpers::get_float(config, "render", "textNoteFontSize", g_config.render.textNoteFontSize);
            config_helpers::get_float(config, "render", "textNotePadding", g_config.render.textNotePadding);

            config_helpers::get_color_rgb(config, "color", "goose", "goose", g_config.color.goose.r, g_config.color.goose.g, g_config.color.goose.b);
            config_helpers::get_color_rgb(config, "color", "beak", "beak", g_config.color.beak.r, g_config.color.beak.g, g_config.color.beak.b);
            config_helpers::get_color_rgb(config, "color", "eye", "eye", g_config.color.eye.r, g_config.color.eye.g, g_config.color.eye.b);
            config_helpers::get_color_rgba(config, "color", "eyeHighlight", "eyeHighlight", g_config.color.eyeHighlight.r, g_config.color.eyeHighlight.g, g_config.color.eyeHighlight.b, g_config.color.eyeHighlight.a);
            config_helpers::get_color_rgb(config, "color", "shadow", "shadow", g_config.color.shadow.r, g_config.color.shadow.g, g_config.color.shadow.b);
            config_helpers::get_color_rgb(config, "color", "footprint", "footprint", g_config.color.footprint.r, g_config.color.footprint.g, g_config.color.footprint.b);
            config_helpers::get_float(config, "color", "footprintAlphaMultiplier", g_config.color.footprintAlphaMultiplier);
            config_helpers::get_color_rgb(config, "color", "droppedItem", "droppedItem", g_config.color.droppedItem.r, g_config.color.droppedItem.g, g_config.color.droppedItem.b);

            config_helpers::get_color_rgb(config, "color", "canadaHead", "canadaHead", g_config.color.canadaHead.r, g_config.color.canadaHead.g, g_config.color.canadaHead.b);
            config_helpers::get_color_rgb(config, "color", "canadaNeck", "canadaNeck", g_config.color.canadaNeck.r, g_config.color.canadaNeck.g, g_config.color.canadaNeck.b);
            config_helpers::get_color_rgb(config, "color", "canadaBody", "canadaBody", g_config.color.canadaBody.r, g_config.color.canadaBody.g, g_config.color.canadaBody.b);
            config_helpers::get_color_rgb(config, "color", "canadaOutline", "canadaOutline", g_config.color.canadaOutline.r, g_config.color.canadaOutline.g, g_config.color.canadaOutline.b);
            config_helpers::get_color_rgb(config, "color", "canadaBeak", "canadaBeak", g_config.color.canadaBeak.r, g_config.color.canadaBeak.g, g_config.color.canadaBeak.b);
            config_helpers::get_color_rgb(config, "color", "canadaEye", "canadaEye", g_config.color.canadaEye.r, g_config.color.canadaEye.g, g_config.color.canadaEye.b);

            return;
        } catch (const std::exception&) {
            continue;
        }
    }
}

static void OnConfigChange() {
    Config_SaveNow(nullptr);
}

bool Config_GetValueByKey(const std::string& key, std::string* valueOut, std::string* errorOut) {
    const ConfigOption* opt = Config_FindOptionByKey(key);
    if (!opt) {
        if (errorOut) *errorOut = "Unknown config key: " + key;
        return false;
    }

    if (valueOut) *valueOut = OptionValueToString(*opt);
    return true;
}

bool Config_SetValueByKey(const std::string& key, const std::string& value, std::string* errorOut) {
    const ConfigOption* opt = Config_FindOptionByKey(key);
    if (!opt) {
        if (errorOut) *errorOut = "Unknown config key: " + key;
        return false;
    }

    if (!AssignValueToOption(*opt, value, errorOut)) return false;
    if (opt->onChange) opt->onChange();
    return true;
}

void Config_InitRegistry() {
    g_configRegistry.clear();

    g_configRegistry.push_back(CONFIG_BOOL("Debug", "to_terminal", "To Terminal",
        &g_config.debug.toTerminal, OnConfigChange));
    g_configRegistry.push_back(CONFIG_BOOL("Debug", "visuals", "Visuals",
        &g_config.debug.visuals, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("General", "global_scale", "Global Scale",
        &g_config.general.globalScale, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_BOOL("General", "audio_enabled", "Audio Enabled",
        &g_config.general.audioEnabled, OnConfigChange));
    g_configRegistry.push_back(CONFIG_BOOL("General", "memes_enabled", "Memes Enabled",
        &g_config.general.memesEnabled, OnConfigChange));
    g_configRegistry.push_back(CONFIG_BOOL("General", "canada_goose_mode", "Canada Goose Mode",
        &g_config.general.canadaGooseMode, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Screen", "default_width", "Default Width",
        &g_config.screen.defaultWidth, 0, 10000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Screen", "default_height", "Default Height",
        &g_config.screen.defaultHeight, 0, 10000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Asset", "meme_placeholder_w", "Meme Placeholder W",
        &g_config.asset.memePlaceholderW, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Asset", "meme_placeholder_h", "Meme Placeholder H",
        &g_config.asset.memePlaceholderH, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Asset", "text_placeholder_w", "Text Placeholder W",
        &g_config.asset.textPlaceholderW, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Asset", "text_placeholder_h", "Text Placeholder H",
        &g_config.asset.textPlaceholderH, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Asset", "note_placeholder_w", "Note Placeholder W",
        &g_config.asset.notePlaceholderW, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Asset", "note_placeholder_h", "Note Placeholder H",
        &g_config.asset.notePlaceholderH, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "base_walk_speed", "Base Walk Speed",
        &g_config.movement.baseWalkSpeed, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "base_run_speed", "Base Run Speed",
        &g_config.movement.baseRunSpeed, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "max_force", "Max Force",
        &g_config.movement.maxForce, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "move_distance_threshold", "Move Distance Threshold",
        &g_config.movement.moveDistanceThreshold, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "friction", "Friction",
        &g_config.movement.friction, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "max_speed", "Max Speed",
        &g_config.movement.maxSpeed, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "run_speed_multiplier", "Run Speed Multiplier",
        &g_config.movement.runSpeedMultiplier, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "init_direction_max", "Init Direction Max",
        &g_config.movement.initDirectionMax, 0.0f, 360.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "speed_lerp_rate", "Speed Lerp Rate",
        &g_config.movement.speedLerpRate, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "arrival_radius", "Arrival Radius",
        &g_config.movement.arrivalRadius, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "run_distance_threshold", "Run Distance Threshold",
        &g_config.movement.runDistanceThreshold, 0.0f, 2000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Movement", "direction_blend_rate", "Direction Blend Rate",
        &g_config.movement.directionBlendRate, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "screen_margin", "Screen Margin",
        &g_config.physics.screenMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "bounce_factor_wall", "Bounce Factor Wall",
        &g_config.physics.bounceFactorWall, 0.0f, 2.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "bounce_factor_corner", "Bounce Factor Corner",
        &g_config.physics.bounceFactorCorner, 0.0f, 2.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "corner_margin", "Corner Margin",
        &g_config.physics.cornerMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "drag_min_dt", "Drag Min Dt",
        &g_config.physics.dragMinDt, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "drag_velocity_threshold", "Drag Velocity Threshold",
        &g_config.physics.dragVelocityThreshold, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "drag_rotation_speed", "Drag Rotation Speed",
        &g_config.physics.dragRotationSpeed, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "iso_scale_x", "Iso Scale X",
        &g_config.physics.isoScaleX, 0.1f, 2.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "iso_scale_y", "Iso Scale Y",
        &g_config.physics.isoScaleY, 0.1f, 2.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "edge_avoid_margin", "Edge Avoid Margin",
        &g_config.physics.edgeAvoidMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "edge_look_ahead_speed", "Edge Look Ahead Speed",
        &g_config.physics.edgeLookAheadSpeed, 0.0f, 100.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "edge_look_ahead_base", "Edge Look Ahead Base",
        &g_config.physics.edgeLookAheadBase, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "edge_avoid_force", "Edge Avoid Force",
        &g_config.physics.edgeAvoidForce, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "screen_clamp_tight", "Screen Clamp Tight",
        &g_config.physics.screenClampTight, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "screen_clamp_expanded", "Screen Clamp Expanded",
        &g_config.physics.screenClampExpanded, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "screen_clamp_bounce", "Screen Clamp Bounce",
        &g_config.physics.screenClampBounce, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "steer_seek_force", "Steer Seek Force",
        &g_config.physics.steerSeekForce, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "curve_fade_distance", "Curve Fade Distance",
        &g_config.physics.curveFadeDistance, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "curve_tangent_force", "Curve Tangent Force",
        &g_config.physics.curveTangentForce, 0.0f, 100.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "curve_fade_min_vel", "Curve Fade Min Vel",
        &g_config.physics.curveFadeMinVel, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "direction_rotate_min_vel", "Direction Rotate Min Vel",
        &g_config.physics.directionRotateMinVel, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "direction_to_cursor_dist", "Direction To Cursor Dist",
        &g_config.physics.directionToCursorDist, 0.0f, 2000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "snap_distance", "Snap Distance",
        &g_config.physics.snapDistance, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "direction_reverse_multiplier", "Direction Reverse Multiplier",
        &g_config.physics.directionReverseMultiplier, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Physics", "min_valid_scale", "Min Valid Scale",
        &g_config.physics.minValidScale, 0.01f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "margin_x", "Margin X",
        &g_config.spawn.marginX, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "margin_y", "Margin Y",
        &g_config.spawn.marginY, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "target_reached_threshold_return", "Target Reached Threshold Return",
        &g_config.spawn.targetReachedThresholdReturn, 1.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "target_reached_threshold_normal", "Target Reached Threshold Normal",
        &g_config.spawn.targetReachedThresholdNormal, 1.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "target_reached_min_return", "Target Reached Min Return",
        &g_config.spawn.targetReachedMinReturn, 1.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "target_reached_min_normal", "Target Reached Min Normal",
        &g_config.spawn.targetReachedMinNormal, 1.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "random_target_margin_x", "Random Target Margin X",
        &g_config.spawn.randomTargetMarginX, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "random_target_margin_y", "Random Target Margin Y",
        &g_config.spawn.randomTargetMarginY, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "item_drop_margin_x", "Item Drop Margin X",
        &g_config.spawn.itemDropMarginX, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "item_drop_margin_y", "Item Drop Margin Y",
        &g_config.spawn.itemDropMarginY, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "item_pickup_distance", "Item Pickup Distance",
        &g_config.spawn.itemPickupDistance, 1.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "catch_threshold_base", "Catch Threshold Base",
        &g_config.spawn.catchThresholdBase, 1.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "catch_threshold_min", "Catch Threshold Min",
        &g_config.spawn.catchThresholdMin, 1.0f, 50.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "wander_target_margin", "Wander Target Margin",
        &g_config.spawn.wanderTargetMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "wander_target_offset", "Wander Target Offset",
        &g_config.spawn.wanderTargetOffset, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "fetch_edge_margin", "Fetch Edge Margin",
        &g_config.spawn.fetchEdgeMargin, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Spawn", "max_fetching_geese", "Max Fetching Geese",
        &g_config.spawn.maxFetchingGeese, 0, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "separation_max_distance", "Separation Max Distance",
        &g_config.spawn.separationMaxDistance, 1.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "separation_force_multiplier", "Separation Force Multiplier",
        &g_config.spawn.separationForceMultiplier, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Spawn", "separation_min_distance", "Separation Min Distance",
        &g_config.spawn.separationMinDistance, 1.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "underbody_y", "Underbody Y",
        &g_config.rig.underbodyY, -100.0f, 100.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "body_y", "Body Y",
        &g_config.rig.bodyY, -100.0f, 100.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "neck_base_x", "Neck Base X",
        &g_config.rig.neckBaseX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "neck_height_idle", "Neck Height Idle",
        &g_config.rig.neckHeightIdle, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "neck_height_moving", "Neck Height Moving",
        &g_config.rig.neckHeightMoving, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "neck_ext_idle", "Neck Ext Idle",
        &g_config.rig.neckExtIdle, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "neck_ext_moving", "Neck Ext Moving",
        &g_config.rig.neckExtMoving, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head_base_x", "Head Base X",
        &g_config.rig.headBaseX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head_base_y", "Head Base Y",
        &g_config.rig.headBaseY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head1_offset_x", "Head1Offset X",
        &g_config.rig.head1OffsetX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head1_offset_y", "Head1Offset Y",
        &g_config.rig.head1OffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head1_offset_z", "Head1Offset Z",
        &g_config.rig.head1OffsetZ, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head2_offset_x", "Head2Offset X",
        &g_config.rig.head2OffsetX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head2_offset_y", "Head2Offset Y",
        &g_config.rig.head2OffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "beak_base_offset", "Beak Base Offset",
        &g_config.rig.beakBaseOffset, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "beak_len", "Beak Len",
        &g_config.rig.beakLen, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "beak_width", "Beak Width",
        &g_config.rig.beakWidth, 0.0f, 30.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "neck_lerp_rate", "Neck Lerp Rate",
        &g_config.rig.neckLerpRate, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "run_speed_threshold", "Run Speed Threshold",
        &g_config.rig.runSpeedThreshold, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "stride_max", "Stride Max",
        &g_config.rig.strideMax, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "foot_spacing", "Foot Spacing",
        &g_config.rig.footSpacing, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "step_lift_height", "Step Lift Height",
        &g_config.rig.stepLiftHeight, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "foot_offset_y", "Foot Offset Y",
        &g_config.rig.footOffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Rig", "head_forward_bias", "Head Forward Bias",
        &g_config.rig.headForwardBias, -1.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_BOOL("Cursor", "chase_enabled", "Chase Enabled",
        &g_config.cursor.chaseEnabled, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Cursor", "chase_chance", "Chase Chance",
        &g_config.cursor.chaseChance, 0, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_BOOL("Cursor", "multi_monitor_enabled", "Multi Monitor Enabled",
        &g_config.cursor.multiMonitorEnabled, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "radius_base", "Radius Base",
        &g_config.snatch.radiusBase, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "radius_range", "Radius Range",
        &g_config.snatch.radiusRange, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "angular_speed_base", "Angular Speed Base",
        &g_config.snatch.angularSpeedBase, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "speed_multiplier", "Speed Multiplier",
        &g_config.snatch.speedMultiplier, 0.0f, 5.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "offset_max", "Offset Max",
        &g_config.snatch.offsetMax, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "duration", "Duration",
        &g_config.snatch.duration, 0.1f, 60.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "pull_distance", "Pull Distance",
        &g_config.snatch.pullDistance, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "lateral_bias_limit", "Lateral Bias Limit",
        &g_config.snatch.lateralBiasLimit, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "forward_bias_scale", "Forward Bias Scale",
        &g_config.snatch.forwardBiasScale, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "forward_bias_min", "Forward Bias Min",
        &g_config.snatch.forwardBiasMin, -100.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "forward_bias_max", "Forward Bias Max",
        &g_config.snatch.forwardBiasMax, -100.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "tier3_max_step", "Tier3Max Step",
        &g_config.snatch.tier3MaxStep, 0.0f, 50.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "tier3_min_delta", "Tier3Min Delta",
        &g_config.snatch.tier3MinDelta, 0.0f, 50.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Snatch", "angular_speed_random_range", "Angular Speed Random Range",
        &g_config.snatch.angularSpeedRandomRange, 0.0f, 5.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_BOOL("Mud", "enabled", "Enabled",
        &g_config.mud.enabled, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Mud", "chance", "Chance",
        &g_config.mud.chance, 0, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Mud", "lifetime", "Lifetime",
        &g_config.mud.lifetime, 0.0f, 60.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Honk", "min_gap", "Min Gap",
        &g_config.honk.minGap, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Honk", "idle_min", "Idle Min",
        &g_config.honk.idleMin, 0.0f, 60.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Honk", "idle_max", "Idle Max",
        &g_config.honk.idleMax, 0.0f, 120.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Honk", "generic_cooldown", "Generic Cooldown",
        &g_config.honk.genericCooldown, 0.0f, 30.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Honk", "chase_cooldown", "Chase Cooldown",
        &g_config.honk.chaseCooldown, 0.0f, 30.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Honk", "fetch_cooldown", "Fetch Cooldown",
        &g_config.honk.fetchCooldown, 0.0f, 30.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Honk", "idle_check_ahead", "Idle Check Ahead",
        &g_config.honk.idleCheckAhead, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Honk", "idle_chance_divisor", "Idle Chance Divisor",
        &g_config.honk.idleChanceDivisor, 1, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Honk", "wander_honk_divisor", "Wander Honk Divisor",
        &g_config.honk.wanderHonkDivisor, 1, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "time_fetch", "Time Fetch",
        &g_config.step.timeFetch, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "time_wander", "Time Wander",
        &g_config.step.timeWander, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "time_snatch", "Time Snatch",
        &g_config.step.timeSnatch, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "step_trigger_walk", "Step Trigger Walk",
        &g_config.step.stepTriggerWalk, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "step_trigger_run", "Step Trigger Run",
        &g_config.step.stepTriggerRun, 0.0f, 500.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "overshoot_walk", "Overshoot Walk",
        &g_config.step.overshootWalk, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "overshoot_run", "Overshoot Run",
        &g_config.step.overshootRun, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "duration_walk", "Duration Walk",
        &g_config.step.durationWalk, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "duration_run", "Duration Run",
        &g_config.step.durationRun, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "lift_walk", "Lift Walk",
        &g_config.step.liftWalk, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "lift_run", "Lift Run",
        &g_config.step.liftRun, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "snap_distance", "Snap Distance",
        &g_config.step.snapDistance, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "dist_factor_base", "Dist Factor Base",
        &g_config.step.distFactorBase, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "dist_factor_min", "Dist Factor Min",
        &g_config.step.distFactorMin, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "dist_factor_max", "Dist Factor Max",
        &g_config.step.distFactorMax, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "duration_min", "Duration Min",
        &g_config.step.durationMin, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "duration_max", "Duration Max",
        &g_config.step.durationMax, 0.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "left_foot_angle", "Left Foot Angle",
        &g_config.step.leftFootAngle, -180.0f, 180.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "right_foot_angle", "Right Foot Angle",
        &g_config.step.rightFootAngle, -180.0f, 180.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "foot_spacing", "Foot Spacing",
        &g_config.step.footSpacing, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "zero_velocity_threshold", "Zero Velocity Threshold",
        &g_config.step.zeroVelocityThreshold, 0.0f, 100.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Step", "min_duration", "Min Duration",
        &g_config.step.minDuration, 0.0f, 0.5f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Item", "pickup_cooldown", "Pickup Cooldown",
        &g_config.item.pickupCooldown, 0.0f, 10.0f, 0.1f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Item", "item_lifetime", "Item Lifetime",
        &g_config.item.itemLifetime, 0.0f, 3600.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "meme_pickup_chance", "Meme Pickup Chance",
        &g_config.item.memePickupChance, 0, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "fetch_base_chance", "Fetch Base Chance",
        &g_config.item.fetchBaseChance, 0, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "max_fetch_bias", "Max Fetch Bias",
        &g_config.item.maxFetchBias, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "max_fetch_geese", "Max Fetch Geese",
        &g_config.item.maxFetchGeese, 0, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "meme_fetch_bias_max", "Meme Fetch Bias Max",
        &g_config.item.memeFetchBiasMax, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "note_fetch_bias_max", "Note Fetch Bias Max",
        &g_config.item.noteFetchBiasMax, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "attack_mouse_bias_max", "Attack Mouse Bias Max",
        &g_config.item.attackMouseBiasMax, 0, 1000, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "heist_chance_percent", "Heist Chance Percent",
        &g_config.item.heistChancePercent, 0, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Item", "heist_approach_margin", "Heist Approach Margin",
        &g_config.item.heistApproachMargin, 0, 500, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "shadow_offset_x", "Shadow Offset X",
        &g_config.render.shadowOffsetX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "shadow_offset_y", "Shadow Offset Y",
        &g_config.render.shadowOffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "shadow_width", "Shadow Width",
        &g_config.render.shadowWidth, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "shadow_height", "Shadow Height",
        &g_config.render.shadowHeight, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "foot_size", "Foot Size",
        &g_config.render.footSize, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "body_width", "Body Width",
        &g_config.render.bodyWidth, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "body_height", "Body Height",
        &g_config.render.bodyHeight, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "neck_size", "Neck Size",
        &g_config.render.neckSize, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "head1_size", "Head1Size",
        &g_config.render.head1Size, 0.0f, 100.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "head2_size", "Head2Size",
        &g_config.render.head2Size, 0.0f, 100.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "beak_width", "Beak Width",
        &g_config.render.beakWidth, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "beak_height", "Beak Height",
        &g_config.render.beakHeight, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "beak_max_width", "Beak Max Width",
        &g_config.render.beakMaxWidth, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "eye_size", "Eye Size",
        &g_config.render.eyeSize, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "eye_offset_x_back", "Eye Offset Xback",
        &g_config.render.eyeOffsetXBack, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "eye_offset_x_front", "Eye Offset Xfront",
        &g_config.render.eyeOffsetXFront, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "eye_offset_y", "Eye Offset Y",
        &g_config.render.eyeOffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "eye_facing_threshold", "Eye Facing Threshold",
        &g_config.render.eyeFacingThreshold, -1.0f, 1.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "click_radius", "Click Radius",
        &g_config.render.clickRadius, 0.0f, 100.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "footprint_width", "Footprint Width",
        &g_config.render.footprintWidth, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "footprint_height", "Footprint Height",
        &g_config.render.footprintHeight, 0.0f, 50.0f, 0.5f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "dropped_item_size", "Dropped Item Size",
        &g_config.render.droppedItemSize, 0.0f, 200.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "frame_rate", "Frame Rate",
        &g_config.render.frameRate, 1.0f, 120.0f, 1.0f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "frame_dt", "Frame Dt",
        &g_config.render.frameDt, 0.001f, 0.1f, 0.001f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_INT("Render", "debug_tick_mod", "Debug Tick Mod",
        &g_config.render.debugTickMod, 1, 100, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "iso_scale_x", "Iso Scale X",
        &g_config.render.isoScaleX, 0.1f, 2.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "iso_scale_y", "Iso Scale Y",
        &g_config.render.isoScaleY, 0.1f, 2.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "squash_factor", "Squash Factor",
        &g_config.render.squashFactor, 0.1f, 2.0f, 0.01f, OnConfigChange));
    g_configRegistry.push_back(CONFIG_FLOAT("Render", "facing_back_threshold", "Facing Back Threshold",
        &g_config.render.facingBackThreshold, -1.0f, 1.0f, 0.01f, OnConfigChange));

    Config_Load();
    Config_SaveNow(nullptr);
}
