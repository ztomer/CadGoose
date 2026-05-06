#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "config.h"

namespace fs = std::filesystem;

Config g_config;
double g_time = 0.0;
std::vector<ConfigOption> g_configRegistry;

static fs::path ConfigDirPath() {
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
    return (ConfigDirPath() / "config.ini").string();
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

    std::ofstream file(Config_GetPath());
    if (!file.is_open()) {
        if (errorOut) *errorOut = "Unable to open config file for writing";
        return false;
    }

    file << "# ==========================================\n";
    file << "# CadGoose Configuration File\n";
    file << "# ==========================================\n\n";

    std::string currentSection = "";
    for (const auto& opt : g_configRegistry) {
        if (currentSection != opt.section) {
            currentSection = opt.section;
            file << "\n[" << currentSection << "]\n";
            if (currentSection == "Debug") {
                file << "# Debugging options for developers.\n";
            } else if (currentSection == "General") {
                file << "# General settings like audio, memes, and visual scales.\n";
            } else if (currentSection == "Movement") {
                file << "# Behavior tuning for goose movement speed, friction, and distances.\n";
            } else if (currentSection == "Physics") {
                file << "# Physics and boundary settings for the goose dragging objects.\n";
            } else if (currentSection == "Asset") {
                file << "# Asset paths and placeholder dimensions.\n";
            } else if (currentSection == "Render") {
                file << "# Rendering options, sizes, and opacities.\n";
            } else if (currentSection == "Color") {
                file << "# Color settings (RGB values from 0.0 to 1.0).\n";
            }
        }
        
        file << "# Label: " << opt.label << "\n";
        if (opt.type == CFG_INT || opt.type == CFG_FLOAT) {
            file << "# Valid range: [" << opt.min << ", " << opt.max << "]\n";
        } else if (opt.type == CFG_BOOL) {
            file << "# Valid range: [0 (false), 1 (true)]\n";
        }
        
        file << opt.key << "=" << OptionValueToString(opt) << "\n\n";
    }

    return true;
}

static void Config_Load() {
    std::vector<fs::path> candidatePaths = {
        fs::path(Config_GetPath()),
        fs::current_path() / "config.ini",
    };

    for (const auto& path : candidatePaths) {
        std::ifstream file(path);
        if (!file.is_open()) continue;

        std::string line;
        while (std::getline(file, line)) {
            const size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            const std::string key = Trim(line.substr(0, eq));
            const std::string value = line.substr(eq + 1);
            const ConfigOption* opt = FindOptionByAnyKey(key);
            if (!opt) continue;

            AssignValueToOption(*opt, value, nullptr);
        }
        return;
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

    g_configRegistry.push_back({"Debug", "to_terminal", "To Terminal", CFG_BOOL, &g_config.debug.toTerminal, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Debug", "visuals", "Visuals", CFG_BOOL, &g_config.debug.visuals, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"General", "global_scale", "Global Scale", CFG_FLOAT, &g_config.general.globalScale, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"General", "audio_enabled", "Audio Enabled", CFG_BOOL, &g_config.general.audioEnabled, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"General", "memes_enabled", "Memes Enabled", CFG_BOOL, &g_config.general.memesEnabled, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"General", "canada_goose_mode", "Canada Goose Mode", CFG_BOOL, &g_config.general.canadaGooseMode, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Screen", "default_width", "Default Width", CFG_INT, &g_config.screen.defaultWidth, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Screen", "default_height", "Default Height", CFG_INT, &g_config.screen.defaultHeight, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Asset", "meme_placeholder_w", "Meme Placeholder W", CFG_INT, &g_config.asset.memePlaceholderW, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Asset", "meme_placeholder_h", "Meme Placeholder H", CFG_INT, &g_config.asset.memePlaceholderH, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Asset", "text_placeholder_w", "Text Placeholder W", CFG_INT, &g_config.asset.textPlaceholderW, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Asset", "text_placeholder_h", "Text Placeholder H", CFG_INT, &g_config.asset.textPlaceholderH, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Asset", "note_placeholder_w", "Note Placeholder W", CFG_INT, &g_config.asset.notePlaceholderW, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Asset", "note_placeholder_h", "Note Placeholder H", CFG_INT, &g_config.asset.notePlaceholderH, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "base_walk_speed", "Base Walk Speed", CFG_FLOAT, &g_config.movement.baseWalkSpeed, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "base_run_speed", "Base Run Speed", CFG_FLOAT, &g_config.movement.baseRunSpeed, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "max_force", "Max Force", CFG_FLOAT, &g_config.movement.maxForce, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "move_distance_threshold", "Move Distance Threshold", CFG_FLOAT, &g_config.movement.moveDistanceThreshold, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "friction", "Friction", CFG_FLOAT, &g_config.movement.friction, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "max_speed", "Max Speed", CFG_FLOAT, &g_config.movement.maxSpeed, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "run_speed_multiplier", "Run Speed Multiplier", CFG_FLOAT, &g_config.movement.runSpeedMultiplier, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "init_direction_max", "Init Direction Max", CFG_FLOAT, &g_config.movement.initDirectionMax, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "speed_lerp_rate", "Speed Lerp Rate", CFG_FLOAT, &g_config.movement.speedLerpRate, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "arrival_radius", "Arrival Radius", CFG_FLOAT, &g_config.movement.arrivalRadius, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "run_distance_threshold", "Run Distance Threshold", CFG_FLOAT, &g_config.movement.runDistanceThreshold, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Movement", "direction_blend_rate", "Direction Blend Rate", CFG_FLOAT, &g_config.movement.directionBlendRate, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "screen_margin", "Screen Margin", CFG_FLOAT, &g_config.physics.screenMargin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "bounce_factor_wall", "Bounce Factor Wall", CFG_FLOAT, &g_config.physics.bounceFactorWall, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "bounce_factor_corner", "Bounce Factor Corner", CFG_FLOAT, &g_config.physics.bounceFactorCorner, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "corner_margin", "Corner Margin", CFG_FLOAT, &g_config.physics.cornerMargin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "drag_min_dt", "Drag Min Dt", CFG_FLOAT, &g_config.physics.dragMinDt, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "drag_velocity_threshold", "Drag Velocity Threshold", CFG_FLOAT, &g_config.physics.dragVelocityThreshold, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "drag_rotation_speed", "Drag Rotation Speed", CFG_FLOAT, &g_config.physics.dragRotationSpeed, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "iso_scale_x", "Iso Scale X", CFG_FLOAT, &g_config.physics.isoScaleX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "iso_scale_y", "Iso Scale Y", CFG_FLOAT, &g_config.physics.isoScaleY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "edge_avoid_margin", "Edge Avoid Margin", CFG_FLOAT, &g_config.physics.edgeAvoidMargin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "edge_look_ahead_speed", "Edge Look Ahead Speed", CFG_FLOAT, &g_config.physics.edgeLookAheadSpeed, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "edge_look_ahead_base", "Edge Look Ahead Base", CFG_FLOAT, &g_config.physics.edgeLookAheadBase, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "edge_avoid_force", "Edge Avoid Force", CFG_FLOAT, &g_config.physics.edgeAvoidForce, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "screen_clamp_tight", "Screen Clamp Tight", CFG_FLOAT, &g_config.physics.screenClampTight, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "screen_clamp_expanded", "Screen Clamp Expanded", CFG_FLOAT, &g_config.physics.screenClampExpanded, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "screen_clamp_bounce", "Screen Clamp Bounce", CFG_FLOAT, &g_config.physics.screenClampBounce, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "steer_seek_force", "Steer Seek Force", CFG_FLOAT, &g_config.physics.steerSeekForce, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "curve_fade_distance", "Curve Fade Distance", CFG_FLOAT, &g_config.physics.curveFadeDistance, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "curve_tangent_force", "Curve Tangent Force", CFG_FLOAT, &g_config.physics.curveTangentForce, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "curve_fade_min_vel", "Curve Fade Min Vel", CFG_FLOAT, &g_config.physics.curveFadeMinVel, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "direction_rotate_min_vel", "Direction Rotate Min Vel", CFG_FLOAT, &g_config.physics.directionRotateMinVel, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "direction_to_cursor_dist", "Direction To Cursor Dist", CFG_FLOAT, &g_config.physics.directionToCursorDist, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "snap_distance", "Snap Distance", CFG_FLOAT, &g_config.physics.snapDistance, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "direction_reverse_multiplier", "Direction Reverse Multiplier", CFG_FLOAT, &g_config.physics.directionReverseMultiplier, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Physics", "min_valid_scale", "Min Valid Scale", CFG_FLOAT, &g_config.physics.minValidScale, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "margin_x", "Margin X", CFG_FLOAT, &g_config.spawn.marginX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "margin_y", "Margin Y", CFG_FLOAT, &g_config.spawn.marginY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "target_reached_threshold_return", "Target Reached Threshold Return", CFG_FLOAT, &g_config.spawn.targetReachedThresholdReturn, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "target_reached_threshold_normal", "Target Reached Threshold Normal", CFG_FLOAT, &g_config.spawn.targetReachedThresholdNormal, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "target_reached_min_return", "Target Reached Min Return", CFG_FLOAT, &g_config.spawn.targetReachedMinReturn, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "target_reached_min_normal", "Target Reached Min Normal", CFG_FLOAT, &g_config.spawn.targetReachedMinNormal, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "random_target_margin_x", "Random Target Margin X", CFG_FLOAT, &g_config.spawn.randomTargetMarginX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "random_target_margin_y", "Random Target Margin Y", CFG_FLOAT, &g_config.spawn.randomTargetMarginY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "item_drop_margin_x", "Item Drop Margin X", CFG_FLOAT, &g_config.spawn.itemDropMarginX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "item_drop_margin_y", "Item Drop Margin Y", CFG_FLOAT, &g_config.spawn.itemDropMarginY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "item_pickup_distance", "Item Pickup Distance", CFG_FLOAT, &g_config.spawn.itemPickupDistance, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "catch_threshold_base", "Catch Threshold Base", CFG_FLOAT, &g_config.spawn.catchThresholdBase, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "catch_threshold_min", "Catch Threshold Min", CFG_FLOAT, &g_config.spawn.catchThresholdMin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "wander_target_margin", "Wander Target Margin", CFG_FLOAT, &g_config.spawn.wanderTargetMargin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "wander_target_offset", "Wander Target Offset", CFG_FLOAT, &g_config.spawn.wanderTargetOffset, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "fetch_edge_margin", "Fetch Edge Margin", CFG_FLOAT, &g_config.spawn.fetchEdgeMargin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "max_fetching_geese", "Max Fetching Geese", CFG_FLOAT, &g_config.spawn.maxFetchingGeese, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "separation_max_distance", "Separation Max Distance", CFG_FLOAT, &g_config.spawn.separationMaxDistance, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "separation_force_multiplier", "Separation Force Multiplier", CFG_FLOAT, &g_config.spawn.separationForceMultiplier, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Spawn", "separation_min_distance", "Separation Min Distance", CFG_FLOAT, &g_config.spawn.separationMinDistance, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "underbody_y", "Underbody Y", CFG_FLOAT, &g_config.rig.underbodyY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "body_y", "Body Y", CFG_FLOAT, &g_config.rig.bodyY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "neck_base_x", "Neck Base X", CFG_FLOAT, &g_config.rig.neckBaseX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "neck_height_idle", "Neck Height Idle", CFG_FLOAT, &g_config.rig.neckHeightIdle, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "neck_height_moving", "Neck Height Moving", CFG_FLOAT, &g_config.rig.neckHeightMoving, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "neck_ext_idle", "Neck Ext Idle", CFG_FLOAT, &g_config.rig.neckExtIdle, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "neck_ext_moving", "Neck Ext Moving", CFG_FLOAT, &g_config.rig.neckExtMoving, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head_base_x", "Head Base X", CFG_FLOAT, &g_config.rig.headBaseX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head_base_y", "Head Base Y", CFG_FLOAT, &g_config.rig.headBaseY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head1_offset_x", "Head1Offset X", CFG_FLOAT, &g_config.rig.head1OffsetX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head1_offset_y", "Head1Offset Y", CFG_FLOAT, &g_config.rig.head1OffsetY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head1_offset_z", "Head1Offset Z", CFG_FLOAT, &g_config.rig.head1OffsetZ, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head2_offset_x", "Head2Offset X", CFG_FLOAT, &g_config.rig.head2OffsetX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head2_offset_y", "Head2Offset Y", CFG_FLOAT, &g_config.rig.head2OffsetY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "beak_base_offset", "Beak Base Offset", CFG_FLOAT, &g_config.rig.beakBaseOffset, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "beak_len", "Beak Len", CFG_FLOAT, &g_config.rig.beakLen, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "beak_width", "Beak Width", CFG_FLOAT, &g_config.rig.beakWidth, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "neck_lerp_rate", "Neck Lerp Rate", CFG_FLOAT, &g_config.rig.neckLerpRate, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "run_speed_threshold", "Run Speed Threshold", CFG_FLOAT, &g_config.rig.runSpeedThreshold, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "stride_max", "Stride Max", CFG_FLOAT, &g_config.rig.strideMax, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "foot_spacing", "Foot Spacing", CFG_FLOAT, &g_config.rig.footSpacing, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "step_lift_height", "Step Lift Height", CFG_FLOAT, &g_config.rig.stepLiftHeight, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "foot_offset_y", "Foot Offset Y", CFG_FLOAT, &g_config.rig.footOffsetY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Rig", "head_forward_bias", "Head Forward Bias", CFG_FLOAT, &g_config.rig.headForwardBias, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Cursor", "chase_enabled", "Chase Enabled", CFG_BOOL, &g_config.cursor.chaseEnabled, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Cursor", "chase_chance", "Chase Chance", CFG_INT, &g_config.cursor.chaseChance, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Cursor", "multi_monitor_enabled", "Multi Monitor Enabled", CFG_BOOL, &g_config.cursor.multiMonitorEnabled, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "radius_base", "Radius Base", CFG_FLOAT, &g_config.snatch.radiusBase, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "radius_range", "Radius Range", CFG_FLOAT, &g_config.snatch.radiusRange, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "angular_speed_base", "Angular Speed Base", CFG_FLOAT, &g_config.snatch.angularSpeedBase, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "speed_multiplier", "Speed Multiplier", CFG_FLOAT, &g_config.snatch.speedMultiplier, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "offset_max", "Offset Max", CFG_FLOAT, &g_config.snatch.offsetMax, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "duration", "Duration", CFG_FLOAT, &g_config.snatch.duration, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "pull_distance", "Pull Distance", CFG_FLOAT, &g_config.snatch.pullDistance, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "lateral_bias_limit", "Lateral Bias Limit", CFG_FLOAT, &g_config.snatch.lateralBiasLimit, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "forward_bias_scale", "Forward Bias Scale", CFG_FLOAT, &g_config.snatch.forwardBiasScale, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "forward_bias_min", "Forward Bias Min", CFG_FLOAT, &g_config.snatch.forwardBiasMin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "forward_bias_max", "Forward Bias Max", CFG_FLOAT, &g_config.snatch.forwardBiasMax, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "tier3_max_step", "Tier3Max Step", CFG_FLOAT, &g_config.snatch.tier3MaxStep, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "tier3_min_delta", "Tier3Min Delta", CFG_FLOAT, &g_config.snatch.tier3MinDelta, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Snatch", "angular_speed_random_range", "Angular Speed Random Range", CFG_FLOAT, &g_config.snatch.angularSpeedRandomRange, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Mud", "enabled", "Enabled", CFG_BOOL, &g_config.mud.enabled, 0, 1, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Mud", "chance", "Chance", CFG_INT, &g_config.mud.chance, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Mud", "lifetime", "Lifetime", CFG_FLOAT, &g_config.mud.lifetime, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "min_gap", "Min Gap", CFG_FLOAT, &g_config.honk.minGap, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "idle_min", "Idle Min", CFG_FLOAT, &g_config.honk.idleMin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "idle_max", "Idle Max", CFG_FLOAT, &g_config.honk.idleMax, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "generic_cooldown", "Generic Cooldown", CFG_FLOAT, &g_config.honk.genericCooldown, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "chase_cooldown", "Chase Cooldown", CFG_FLOAT, &g_config.honk.chaseCooldown, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "fetch_cooldown", "Fetch Cooldown", CFG_FLOAT, &g_config.honk.fetchCooldown, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "idle_check_ahead", "Idle Check Ahead", CFG_FLOAT, &g_config.honk.idleCheckAhead, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "idle_chance_divisor", "Idle Chance Divisor", CFG_INT, &g_config.honk.idleChanceDivisor, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Honk", "wander_honk_divisor", "Wander Honk Divisor", CFG_INT, &g_config.honk.wanderHonkDivisor, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "time_fetch", "Time Fetch", CFG_FLOAT, &g_config.step.timeFetch, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "time_wander", "Time Wander", CFG_FLOAT, &g_config.step.timeWander, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "time_snatch", "Time Snatch", CFG_FLOAT, &g_config.step.timeSnatch, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "step_trigger_walk", "Step Trigger Walk", CFG_FLOAT, &g_config.step.stepTriggerWalk, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "step_trigger_run", "Step Trigger Run", CFG_FLOAT, &g_config.step.stepTriggerRun, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "overshoot_walk", "Overshoot Walk", CFG_FLOAT, &g_config.step.overshootWalk, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "overshoot_run", "Overshoot Run", CFG_FLOAT, &g_config.step.overshootRun, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "duration_walk", "Duration Walk", CFG_FLOAT, &g_config.step.durationWalk, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "duration_run", "Duration Run", CFG_FLOAT, &g_config.step.durationRun, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "lift_walk", "Lift Walk", CFG_FLOAT, &g_config.step.liftWalk, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "lift_run", "Lift Run", CFG_FLOAT, &g_config.step.liftRun, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "snap_distance", "Snap Distance", CFG_FLOAT, &g_config.step.snapDistance, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "dist_factor_base", "Dist Factor Base", CFG_FLOAT, &g_config.step.distFactorBase, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "dist_factor_min", "Dist Factor Min", CFG_FLOAT, &g_config.step.distFactorMin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "dist_factor_max", "Dist Factor Max", CFG_FLOAT, &g_config.step.distFactorMax, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "duration_min", "Duration Min", CFG_FLOAT, &g_config.step.durationMin, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "duration_max", "Duration Max", CFG_FLOAT, &g_config.step.durationMax, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "left_foot_angle", "Left Foot Angle", CFG_FLOAT, &g_config.step.leftFootAngle, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "right_foot_angle", "Right Foot Angle", CFG_FLOAT, &g_config.step.rightFootAngle, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "foot_spacing", "Foot Spacing", CFG_FLOAT, &g_config.step.footSpacing, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "zero_velocity_threshold", "Zero Velocity Threshold", CFG_FLOAT, &g_config.step.zeroVelocityThreshold, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Step", "min_duration", "Min Duration", CFG_FLOAT, &g_config.step.minDuration, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "pickup_cooldown", "Pickup Cooldown", CFG_FLOAT, &g_config.item.pickupCooldown, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "item_lifetime", "Item Lifetime", CFG_FLOAT, &g_config.item.itemLifetime, 0.0f, 3600.0f, 1.0f, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "meme_pickup_chance", "Meme Pickup Chance", CFG_INT, &g_config.item.memePickupChance, 0, 100, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "fetch_base_chance", "Fetch Base Chance", CFG_INT, &g_config.item.fetchBaseChance, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "max_fetch_bias", "Max Fetch Bias", CFG_INT, &g_config.item.maxFetchBias, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "max_fetch_geese", "Max Fetch Geese", CFG_INT, &g_config.item.maxFetchGeese, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "meme_fetch_bias_max", "Meme Fetch Bias Max", CFG_INT, &g_config.item.memeFetchBiasMax, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "note_fetch_bias_max", "Note Fetch Bias Max", CFG_INT, &g_config.item.noteFetchBiasMax, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "attack_mouse_bias_max", "Attack Mouse Bias Max", CFG_INT, &g_config.item.attackMouseBiasMax, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "heist_chance_percent", "Heist Chance Percent", CFG_INT, &g_config.item.heistChancePercent, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Item", "heist_approach_margin", "Heist Approach Margin", CFG_INT, &g_config.item.heistApproachMargin, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "shadow_offset_x", "Shadow Offset X", CFG_FLOAT, &g_config.render.shadowOffsetX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "shadow_offset_y", "Shadow Offset Y", CFG_FLOAT, &g_config.render.shadowOffsetY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "shadow_width", "Shadow Width", CFG_FLOAT, &g_config.render.shadowWidth, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "shadow_height", "Shadow Height", CFG_FLOAT, &g_config.render.shadowHeight, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "foot_size", "Foot Size", CFG_FLOAT, &g_config.render.footSize, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "body_width", "Body Width", CFG_FLOAT, &g_config.render.bodyWidth, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "body_height", "Body Height", CFG_FLOAT, &g_config.render.bodyHeight, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "neck_size", "Neck Size", CFG_FLOAT, &g_config.render.neckSize, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "head1_size", "Head1Size", CFG_FLOAT, &g_config.render.head1Size, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "head2_size", "Head2Size", CFG_FLOAT, &g_config.render.head2Size, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "beak_width", "Beak Width", CFG_FLOAT, &g_config.render.beakWidth, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "beak_height", "Beak Height", CFG_FLOAT, &g_config.render.beakHeight, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "beak_max_width", "Beak Max Width", CFG_FLOAT, &g_config.render.beakMaxWidth, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "eye_size", "Eye Size", CFG_FLOAT, &g_config.render.eyeSize, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "eye_offset_x_back", "Eye Offset Xback", CFG_FLOAT, &g_config.render.eyeOffsetXBack, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "eye_offset_x_front", "Eye Offset Xfront", CFG_FLOAT, &g_config.render.eyeOffsetXFront, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "eye_offset_y", "Eye Offset Y", CFG_FLOAT, &g_config.render.eyeOffsetY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "eye_facing_threshold", "Eye Facing Threshold", CFG_FLOAT, &g_config.render.eyeFacingThreshold, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "click_radius", "Click Radius", CFG_FLOAT, &g_config.render.clickRadius, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "footprint_width", "Footprint Width", CFG_FLOAT, &g_config.render.footprintWidth, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "footprint_height", "Footprint Height", CFG_FLOAT, &g_config.render.footprintHeight, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "dropped_item_size", "Dropped Item Size", CFG_FLOAT, &g_config.render.droppedItemSize, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "frame_rate", "Frame Rate", CFG_FLOAT, &g_config.render.frameRate, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "frame_dt", "Frame Dt", CFG_FLOAT, &g_config.render.frameDt, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "debug_tick_mod", "Debug Tick Mod", CFG_INT, &g_config.render.debugTickMod, 0, 1000, 1, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "iso_scale_x", "Iso Scale X", CFG_FLOAT, &g_config.render.isoScaleX, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "iso_scale_y", "Iso Scale Y", CFG_FLOAT, &g_config.render.isoScaleY, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "squash_factor", "Squash Factor", CFG_FLOAT, &g_config.render.squashFactor, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});
    g_configRegistry.push_back({"Render", "facing_back_threshold", "Facing Back Threshold", CFG_FLOAT, &g_config.render.facingBackThreshold, 0.0f, 1000.0f, 0.1f, "", OnConfigChange});

    Config_Load();
    Config_SaveNow(nullptr);
}
