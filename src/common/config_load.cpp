#include <filesystem>
#include <vector>
#include <algorithm>
#include <toml.hpp>
#include "config_helpers.h"

namespace fs = std::filesystem;

void Config_Load(const toml::basic_value<toml::type_config>& config) {
    for (const auto& opt : g_configRegistry) {
        if (!config_helpers::section_has_key(config, opt.section, opt.key)) continue;

        if (opt.type == CFG_BOOL) {
            bool val = false;
            if (config_helpers::get_bool(config, opt.section, opt.key, val))
                *(bool*)opt.ptr = val;
        } else if (opt.type == CFG_INT) {
            int val = 0;
            if (config_helpers::get_int(config, opt.section, opt.key, val)) {
                val = std::clamp(val, static_cast<int>(opt.min), static_cast<int>(opt.max));
                *(int*)opt.ptr = val;
            }
        } else if (opt.type == CFG_FLOAT) {
            float val = 0.0f;
            if (config_helpers::get_float(config, opt.section, opt.key, val)) {
                val = std::clamp(val, opt.min, opt.max);
                *(float*)opt.ptr = val;
            }
        } else if (opt.type == CFG_STRING) {
            std::string val;
            if (config_helpers::get_string(config, opt.section, opt.key, val)) {
                *(std::string*)opt.ptr = val;
            }
        }
    }

    config_helpers::get_color_rgb(config, "color", "goose", "goose",
        g_config.color.goose.r, g_config.color.goose.g, g_config.color.goose.b);
    config_helpers::get_color_rgb(config, "color", "beak", "beak",
        g_config.color.beak.r, g_config.color.beak.g, g_config.color.beak.b);
    config_helpers::get_color_rgb(config, "color", "eye", "eye",
        g_config.color.eye.r, g_config.color.eye.g, g_config.color.eye.b);
    config_helpers::get_color_rgba(config, "color", "eyeHighlight", "eyeHighlight",
        g_config.color.eyeHighlight.r, g_config.color.eyeHighlight.g,
        g_config.color.eyeHighlight.b, g_config.color.eyeHighlight.a);
    config_helpers::get_color_rgb(config, "color", "shadow", "shadow",
        g_config.color.shadow.r, g_config.color.shadow.g, g_config.color.shadow.b);
    config_helpers::get_color_rgb(config, "color", "footprint", "footprint",
        g_config.color.footprint.r, g_config.color.footprint.g, g_config.color.footprint.b);
    config_helpers::get_float(config, "color", "footprintAlphaMultiplier",
        g_config.color.footprintAlphaMultiplier);
    config_helpers::get_color_rgb(config, "color", "droppedItem", "droppedItem",
        g_config.color.droppedItem.r, g_config.color.droppedItem.g, g_config.color.droppedItem.b);
    config_helpers::get_color_rgb(config, "color", "canadaHead", "canadaHead",
        g_config.color.canadaHead.r, g_config.color.canadaHead.g, g_config.color.canadaHead.b);
    config_helpers::get_color_rgb(config, "color", "canadaNeck", "canadaNeck",
        g_config.color.canadaNeck.r, g_config.color.canadaNeck.g, g_config.color.canadaNeck.b);
    config_helpers::get_color_rgb(config, "color", "canadaBody", "canadaBody",
        g_config.color.canadaBody.r, g_config.color.canadaBody.g, g_config.color.canadaBody.b);
    config_helpers::get_color_rgb(config, "color", "canadaOutline", "canadaOutline",
        g_config.color.canadaOutline.r, g_config.color.canadaOutline.g, g_config.color.canadaOutline.b);
    config_helpers::get_color_rgb(config, "color", "canadaBeak", "canadaBeak",
        g_config.color.canadaBeak.r, g_config.color.canadaBeak.g, g_config.color.canadaBeak.b);
    config_helpers::get_color_rgb(config, "color", "canadaEye", "canadaEye",
        g_config.color.canadaEye.r, g_config.color.canadaEye.g, g_config.color.canadaEye.b);
    config_helpers::get_color_rgb(config, "color", "customBody", "customBody",
        g_config.color.customBody.r, g_config.color.customBody.g, g_config.color.customBody.b);
    config_helpers::get_color_rgb(config, "color", "customNeck", "customNeck",
        g_config.color.customNeck.r, g_config.color.customNeck.g, g_config.color.customNeck.b);
    config_helpers::get_color_rgb(config, "color", "customHead", "customHead",
        g_config.color.customHead.r, g_config.color.customHead.g, g_config.color.customHead.b);
    config_helpers::get_color_rgb(config, "color", "customBeak", "customBeak",
        g_config.color.customBeak.r, g_config.color.customBeak.g, g_config.color.customBeak.b);
    config_helpers::get_color_rgb(config, "color", "customEye", "customEye",
        g_config.color.customEye.r, g_config.color.customEye.g, g_config.color.customEye.b);
    config_helpers::get_color_rgb(config, "color", "customOutline", "customOutline",
        g_config.color.customOutline.r, g_config.color.customOutline.g, g_config.color.customOutline.b);
}

void Config_LoadAll() {
    std::vector<fs::path> candidates = {
        fs::path(Config_GetPath()),
        fs::current_path() / "config.toml",
        fs::current_path() / "config.ini",
    };
    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            try {
                auto parsed = toml::parse(path.string());
                Config_Load(parsed);
                Config_UpdateActiveTheme();
                return;
            } catch (const std::exception&) {
                continue;
            }
        }
    }
    Config_UpdateActiveTheme(); // Fallback if no config file loaded
}

bool Config_LoadThemeColors(const std::string& themeName, ColorRGB& body, ColorRGB& neck, ColorRGB& head, ColorRGB& beak, ColorRGB& eye, ColorRGB& outline) {
    if (themeName.empty() || themeName == "Default") return false;
    
    // Convert to filename: "Default Light" -> "default_light.toml" or exact matches
    std::string filename = themeName;
    std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c){ return std::tolower(c); });
    std::replace(filename.begin(), filename.end(), ' ', '_');
    if (filename.find(".toml") == std::string::npos) filename += ".toml";
    
    fs::path themePath = Config_GetThemesDir() / filename;
    if (!fs::exists(themePath)) {
        // Fallback: search through all .toml files to match 'name' in [theme]
        bool found = false;
        for (const auto& entry : fs::directory_iterator(Config_GetThemesDir())) {
            if (entry.path().extension() == ".toml") {
                try {
                    auto data = toml::parse(entry.path().string());
                    if (toml::find_or<std::string>(data, "theme", "name", "") == themeName) {
                        themePath = entry.path();
                        found = true;
                        break;
                    }
                } catch (...) {}
            }
        }
        if (!found) return false;
    }
    
    try {
        auto data = toml::parse(themePath.string());
        if (!data.contains("colors")) return false;
        
        config_helpers::get_color_rgb_flat(data, "colors", "body", body.r, body.g, body.b);
        config_helpers::get_color_rgb_flat(data, "colors", "neck", neck.r, neck.g, neck.b);
        config_helpers::get_color_rgb_flat(data, "colors", "head", head.r, head.g, head.b);
        config_helpers::get_color_rgb_flat(data, "colors", "beak", beak.r, beak.g, beak.b);
        config_helpers::get_color_rgb_flat(data, "colors", "eye", eye.r, eye.g, eye.b);
        config_helpers::get_color_rgb_flat(data, "colors", "outline", outline.r, outline.g, outline.b);
        return true;
    } catch (...) {
        return false;
    }
}

extern bool Config_IsSystemDarkTheme();

void Config_UpdateActiveTheme() {
    int mode = g_config.general.appearanceMode;
    
    // 0=Light, 1=Dark, 2=System, 3=Custom
    if (mode == 2) {
        mode = Config_IsSystemDarkTheme() ? 1 : 0;
    }

    if (mode == 3) { // Custom
        g_config.color.currentBody = g_config.color.customBody;
        g_config.color.currentNeck = g_config.color.customNeck;
        g_config.color.currentHead = g_config.color.customHead;
        g_config.color.currentBeak = g_config.color.customBeak;
        g_config.color.currentEye = g_config.color.customEye;
        g_config.color.currentOutline = g_config.color.customOutline;
        return;
    }
    
    std::string roleTheme = (mode == 1) ? g_config.general.darkThemeRole : g_config.general.lightThemeRole;
    
    // Default fallback values if theme fails to load
    if (mode == 1) { // Dark (Canadian) fallback
        g_config.color.currentBody = g_config.color.canadaBody;
        g_config.color.currentNeck = g_config.color.canadaNeck;
        g_config.color.currentHead = g_config.color.canadaHead;
        g_config.color.currentBeak = g_config.color.canadaBeak;
        g_config.color.currentEye = g_config.color.canadaEye;
        g_config.color.currentOutline = g_config.color.canadaOutline;
    } else { // Light fallback
        g_config.color.currentBody = g_config.color.goose;
        g_config.color.currentNeck = g_config.color.goose;
        g_config.color.currentHead = g_config.color.goose;
        g_config.color.currentBeak = g_config.color.beak;
        g_config.color.currentEye = g_config.color.eye;
        g_config.color.currentOutline = {0.82f, 0.82f, 0.82f};
    }
    
    Config_LoadThemeColors(roleTheme, 
        g_config.color.currentBody, g_config.color.currentNeck, g_config.color.currentHead, 
        g_config.color.currentBeak, g_config.color.currentEye, g_config.color.currentOutline);
}