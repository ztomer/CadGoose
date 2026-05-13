#ifndef CONFIG_HELPERS_H
#define CONFIG_HELPERS_H

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <toml.hpp>
#include "config.h"

namespace config_helpers {

template <typename Table>
inline auto find_case_insensitive(Table& tbl, const std::string& key) {
    std::string key_lower = key;
    std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), ::tolower);
    for (auto it = tbl.begin(); it != tbl.end(); ++it) {
        std::string k = it->first;
        std::transform(k.begin(), k.end(), k.begin(), ::tolower);
        if (k == key_lower) return it;
    }
    return tbl.end();
}

inline bool get_bool(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, bool& dest) {
    auto& tbl = config.as_table();
    auto section_it = find_case_insensitive(tbl, section);
    if (section_it == tbl.end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = find_case_insensitive(section_tbl, key);
    if (key_it == section_tbl.end()) return false;
    dest = toml::get<bool>(key_it->second);
    return true;
}

inline bool get_int(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, int& dest) {
    auto& tbl = config.as_table();
    auto section_it = find_case_insensitive(tbl, section);
    if (section_it == tbl.end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = find_case_insensitive(section_tbl, key);
    if (key_it == section_tbl.end()) return false;
    dest = toml::get<int>(key_it->second);
    return true;
}

inline bool get_float(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, float& dest) {
    auto& tbl = config.as_table();
    auto section_it = find_case_insensitive(tbl, section);
    if (section_it == tbl.end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = find_case_insensitive(section_tbl, key);
    if (key_it == section_tbl.end()) return false;
    try {
        if (key_it->second.is_integer()) dest = static_cast<float>(toml::get<int>(key_it->second));
        else dest = toml::get<float>(key_it->second);
        return true;
    } catch (...) {
        return false;
    }
}

inline bool get_string(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key, std::string& dest) {
    auto& tbl = config.as_table();
    auto section_it = find_case_insensitive(tbl, section);
    if (section_it == tbl.end()) return false;
    auto& section_tbl = section_it->second.as_table();
    auto key_it = find_case_insensitive(section_tbl, key);
    if (key_it == section_tbl.end()) return false;
    try {
        dest = toml::get<std::string>(key_it->second);
        return true;
    } catch (...) {
        return false;
    }
}

inline bool get_color_rgb(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& subgroup, const std::string& colorName, float& r, float& g, float& b) {
    try {
        auto& section_tbl = toml::get<toml::table>(config.at(section));
        auto& subgroup_tbl = toml::get<toml::table>(section_tbl.at(subgroup));
        auto& color_tbl = toml::get<toml::table>(subgroup_tbl.at(colorName));
        if (color_tbl.at("r").is_integer()) r = static_cast<float>(toml::get<int>(color_tbl.at("r"))); else r = toml::get<float>(color_tbl.at("r"));
        if (color_tbl.at("g").is_integer()) g = static_cast<float>(toml::get<int>(color_tbl.at("g"))); else g = toml::get<float>(color_tbl.at("g"));
        if (color_tbl.at("b").is_integer()) b = static_cast<float>(toml::get<int>(color_tbl.at("b"))); else b = toml::get<float>(color_tbl.at("b"));
        return true;
    } catch (...) {
        return false;
    }
}

inline bool get_color_rgb_flat(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& colorName, float& r, float& g, float& b) {
    try {
        auto& section_tbl = toml::get<toml::table>(config.at(section));
        auto& color_tbl = toml::get<toml::table>(section_tbl.at(colorName));
        if (color_tbl.at("r").is_integer()) r = static_cast<float>(toml::get<int>(color_tbl.at("r"))); else r = toml::get<float>(color_tbl.at("r"));
        if (color_tbl.at("g").is_integer()) g = static_cast<float>(toml::get<int>(color_tbl.at("g"))); else g = toml::get<float>(color_tbl.at("g"));
        if (color_tbl.at("b").is_integer()) b = static_cast<float>(toml::get<int>(color_tbl.at("b"))); else b = toml::get<float>(color_tbl.at("b"));
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

inline bool section_has_key(const toml::basic_value<toml::type_config>& config, const std::string& section, const std::string& key) {
    auto& tbl = config.as_table();
    auto section_it = find_case_insensitive(tbl, section);
    if (section_it == tbl.end()) return false;
    auto& section_tbl = section_it->second.as_table();
    return find_case_insensitive(section_tbl, key) != section_tbl.end();
}

template<typename T>
void set_by_type(ConfigType type, void* ptr, T val) {
    if (type == CFG_BOOL) *(bool*)ptr = val;
    else if (type == CFG_INT) *(int*)ptr = val;
    else if (type == CFG_FLOAT) *(float*)ptr = val;
    else *(std::string*)ptr = val;
}

template<typename T>
T get_by_type(ConfigType type, void* ptr) {
    if (type == CFG_BOOL) return *(bool*)ptr;
    if (type == CFG_INT) return *(int*)ptr;
    if (type == CFG_FLOAT) return *(float*)ptr;
    return *(std::string*)ptr;
}
}

#endif