#include <fstream>
#include <toml.hpp>
#include "config_helpers.h"
#include "world.h"

namespace fs = std::filesystem;

void Config_SaveGooseNames() {
    g_config.gooseNames.clear();
    for (const auto& g : g_world.geese) {
        g_config.gooseNames.push(g.name);
    }
    Config_SaveAll();
}

void Config_SaveAll() {
    std::error_code ec;
    fs::create_directories(ConfigDirPath(), ec);
    if (ec) return;

    std::string configPath = Config_GetPath();
    std::string tempPath = configPath + ".tmp";

    std::ofstream ofs(tempPath);
    if (!ofs.is_open()) return;

    toml::table config;

    for (const auto& opt : g_configRegistry) {
        std::string section(opt.section);
        std::string key(opt.key);
        
        if (opt.type == CFG_BOOL) {
            config[section][key] = *(bool*)opt.ptr ? true : false;
        } else if (opt.type == CFG_INT) {
            config[section][key] = *(int*)opt.ptr;
        } else if (opt.type == CFG_FLOAT) {
            config[section][key] = *(float*)opt.ptr;
        } else {
            config[section][key] = std::string(*(std::string*)opt.ptr);
        }
    }

    config["color"]["goose"] = toml::table{{"r", g_config.color.goose.r}, {"g", g_config.color.goose.g}, {"b", g_config.color.goose.b}};
    config["color"]["beak"] = toml::table{{"r", g_config.color.beak.r}, {"g", g_config.color.beak.g}, {"b", g_config.color.beak.b}};
    config["color"]["eye"] = toml::table{{"r", g_config.color.eye.r}, {"g", g_config.color.eye.g}, {"b", g_config.color.eye.b}};
    config["color"]["eyeHighlight"] = toml::table{{"r", g_config.color.eyeHighlight.r}, {"g", g_config.color.eyeHighlight.g}, {"b", g_config.color.eyeHighlight.b}, {"a", g_config.color.eyeHighlight.a}};
    config["color"]["shadow"] = toml::table{{"r", g_config.color.shadow.r}, {"g", g_config.color.shadow.g}, {"b", g_config.color.shadow.b}};
    config["color"]["footprint"] = toml::table{{"r", g_config.color.footprint.r}, {"g", g_config.color.footprint.g}, {"b", g_config.color.footprint.b}};
    config["color"]["footprintAlphaMultiplier"] = g_config.color.footprintAlphaMultiplier;
    config["color"]["droppedItem"] = toml::table{{"r", g_config.color.droppedItem.r}, {"g", g_config.color.droppedItem.g}, {"b", g_config.color.droppedItem.b}};
    config["color"]["canadaHead"] = toml::table{{"r", g_config.color.canadaHead.r}, {"g", g_config.color.canadaHead.g}, {"b", g_config.color.canadaHead.b}};
    config["color"]["canadaNeck"] = toml::table{{"r", g_config.color.canadaNeck.r}, {"g", g_config.color.canadaNeck.g}, {"b", g_config.color.canadaNeck.b}};
    config["color"]["canadaBody"] = toml::table{{"r", g_config.color.canadaBody.r}, {"g", g_config.color.canadaBody.g}, {"b", g_config.color.canadaBody.b}};
    config["color"]["canadaOutline"] = toml::table{{"r", g_config.color.canadaOutline.r}, {"g", g_config.color.canadaOutline.g}, {"b", g_config.color.canadaOutline.b}};
    config["color"]["canadaBeak"] = toml::table{{"r", g_config.color.canadaBeak.r}, {"g", g_config.color.canadaBeak.g}, {"b", g_config.color.canadaBeak.b}};
    config["color"]["canadaEye"] = toml::table{{"r", g_config.color.canadaEye.r}, {"g", g_config.color.canadaEye.g}, {"b", g_config.color.canadaEye.b}};
    config["color"]["customBody"] = toml::table{{"r", g_config.color.customBody.r}, {"g", g_config.color.customBody.g}, {"b", g_config.color.customBody.b}};
    config["color"]["customNeck"] = toml::table{{"r", g_config.color.customNeck.r}, {"g", g_config.color.customNeck.g}, {"b", g_config.color.customNeck.b}};
    config["color"]["customHead"] = toml::table{{"r", g_config.color.customHead.r}, {"g", g_config.color.customHead.g}, {"b", g_config.color.customHead.b}};
    config["color"]["customBeak"] = toml::table{{"r", g_config.color.customBeak.r}, {"g", g_config.color.customBeak.g}, {"b", g_config.color.customBeak.b}};
    config["color"]["customEye"] = toml::table{{"r", g_config.color.customEye.r}, {"g", g_config.color.customEye.g}, {"b", g_config.color.customEye.b}};
    config["color"]["customOutline"] = toml::table{{"r", g_config.color.customOutline.r}, {"g", g_config.color.customOutline.g}, {"b", g_config.color.customOutline.b}};

    if (!g_config.gooseNames.empty()) {
        toml::array namesArr;
        for (const auto& name : g_config.gooseNames) {
            namesArr.push_back(name);
        }
        config["goose"] = namesArr;
    }

    ofs << toml::value(config) << "\n";
    ofs.close();

    fs::rename(tempPath, configPath, ec);
}