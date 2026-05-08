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
                return;
            } catch (const std::exception&) {
                continue;
            }
        }
    }
}