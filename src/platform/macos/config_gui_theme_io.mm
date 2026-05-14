// config_gui_theme_io.mm
// Theme file read/write helpers
#import "config_gui_helpers.h"
#include "config.h"
#include <toml.hpp>
#include <fstream>
#include <string>

static NSString* ThemeDescriptionForFile(const std::string& path) {
    try {
        auto data = toml::parse(path);
        auto& theme = toml::find(data, "theme");
        return @(toml::find<std::string>(theme, "description").c_str());
    } catch (...) { return @""; }
}

bool LoadThemeFromFile(const std::string& path) {
    try {
        auto data = toml::parse(path);
        auto& colors = toml::find(data, "colors");
        g_config.color.customBody.r    = toml::find<float>(colors, "body", "r");
        g_config.color.customBody.g    = toml::find<float>(colors, "body", "g");
        g_config.color.customBody.b    = toml::find<float>(colors, "body", "b");
        g_config.color.customNeck.r    = toml::find<float>(colors, "neck", "r");
        g_config.color.customNeck.g    = toml::find<float>(colors, "neck", "g");
        g_config.color.customNeck.b    = toml::find<float>(colors, "neck", "b");
        g_config.color.customHead.r    = toml::find<float>(colors, "head", "r");
        g_config.color.customHead.g    = toml::find<float>(colors, "head", "g");
        g_config.color.customHead.b    = toml::find<float>(colors, "head", "b");
        g_config.color.customBeak.r    = toml::find<float>(colors, "beak", "r");
        g_config.color.customBeak.g    = toml::find<float>(colors, "beak", "g");
        g_config.color.customBeak.b    = toml::find<float>(colors, "beak", "b");
        g_config.color.customEye.r     = toml::find<float>(colors, "eye", "r");
        g_config.color.customEye.g     = toml::find<float>(colors, "eye", "g");
        g_config.color.customEye.b     = toml::find<float>(colors, "eye", "b");
        g_config.color.customOutline.r = toml::find<float>(colors, "outline", "r");
        g_config.color.customOutline.g = toml::find<float>(colors, "outline", "g");
        g_config.color.customOutline.b = toml::find<float>(colors, "outline", "b");
        return true;
    } catch (...) { return false; }
}

bool SaveThemeToFile(const std::string& path, const std::string& desc) {
    try {
        toml::table theme;
        theme["theme"]["description"] = desc;

        auto setColor = [&](const std::string& name, const ColorRGB& c) {
            theme["colors"][name] = toml::table{{"r", c.r}, {"g", c.g}, {"b", c.b}};
        };
        setColor("body",    g_config.color.customBody);
        setColor("neck",    g_config.color.customNeck);
        setColor("head",    g_config.color.customHead);
        setColor("beak",    g_config.color.customBeak);
        setColor("eye",     g_config.color.customEye);
        setColor("outline", g_config.color.customOutline);

        std::ofstream ofs(path);
        if (!ofs.is_open()) return false;
        ofs << toml::value(theme) << "\n";
        return true;
    } catch (...) { return false; }
}
