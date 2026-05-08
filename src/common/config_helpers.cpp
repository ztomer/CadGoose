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