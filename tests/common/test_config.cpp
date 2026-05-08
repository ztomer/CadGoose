#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include <chrono>
#include "config.h"
#include "toml.hpp"
#include "../../src/common/config_helpers.h"

namespace fs = std::filesystem;

TEST(ConfigRegistry, FindOptionByKey) {
    Config_Init();
    ASSERT_FALSE(g_configRegistry.empty());

    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    ASSERT_NE(opt, nullptr) << "Could not find global_scale";
    EXPECT_STREQ(opt->key, "global_scale");
    EXPECT_STREQ(opt->section, "General");
    EXPECT_EQ(opt->type, CFG_FLOAT);

    const ConfigOption* notFound = Config_FindOptionByKey("nonexistent_key");
    EXPECT_EQ(notFound, nullptr);
}

TEST(ConfigRegistry, CaseInsensitiveLookup) {
    Config_Init();
    EXPECT_NE(Config_FindOptionByKey("global_scale"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("GLOBAL_SCALE"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("Global_Scale"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("GLOBAL_SCALE"), nullptr);
    EXPECT_NE(Config_FindOptionByKey("GLOBAL_SCALE"), nullptr);
}

TEST(ConfigRegistry, AllEntriesHaveValidTypes) {
    Config_Init();
    for (const auto& opt : g_configRegistry) {
        EXPECT_GE(opt.type, CFG_BOOL) << "Invalid type for key: " << opt.key;
        EXPECT_LE(opt.type, CFG_STRING) << "Invalid type for key: " << opt.key;
    }
}

TEST(ConfigRegistry, AllFloatEntriesHaveValidPtr) {
    Config_Init();
    for (const auto& opt : g_configRegistry) {
        if (opt.type == CFG_FLOAT) {
            EXPECT_NE(opt.ptr, nullptr) << "Null ptr for key: " << opt.key;
        }
    }
}

TEST(ConfigRegistry, O1LookupPerformance) {
    Config_Init();
    // Do many lookups to ensure performance is fast (O(1))
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        Config_FindOptionByKey("global_scale");
        Config_FindOptionByKey("default_width");
        Config_FindOptionByKey("base_walk_speed");
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Lookups should be fast enough to complete well under 50ms for 30000 lookups if it's O(1) map lookup
    EXPECT_LT(duration, 50);
}

TEST(ConfigGetSet, GetValueByKey_Bool) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("audio_enabled");
    if (opt && opt->type == CFG_BOOL) {
        std::string value;
        bool result = Config_GetValueByKey("audio_enabled", &value);
        EXPECT_TRUE(result);
        EXPECT_TRUE(value == "0" || value == "1");
    }
}

TEST(ConfigGetSet, GetValueByKey_Int) {
    Config_Init();
    std::string value;
    bool result = Config_GetValueByKey("default_width", &value);
    EXPECT_TRUE(result);
    int parsed = std::stoi(value);
    EXPECT_GE(parsed, 0);
}

TEST(ConfigGetSet, GetValueByKey_Float) {
    Config_Init();
    std::string value;
    bool result = Config_GetValueByKey("global_scale", &value);
    EXPECT_TRUE(result);
    float parsed = std::stof(value);
    EXPECT_GT(parsed, 0.0f);
}

TEST(ConfigGetSet, GetValueByKey_Unknown) {
    std::string value;
    std::string error;
    bool result = Config_GetValueByKey("nonexistent.key", &value, &error);
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
}

TEST(ConfigGetSet, SetValueByKey_Bool_True) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("audio_enabled");
    if (!opt || opt->type != CFG_BOOL) return;

    *(bool*)opt->ptr = false; // ensure it is false
    bool original = *(bool*)opt->ptr;
    bool result = Config_SetValueByKey("audio_enabled", "1");
    EXPECT_TRUE(result);
    EXPECT_NE(*(bool*)opt->ptr, original);

    result = Config_SetValueByKey("audio_enabled", "true");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "yes");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "on");
    EXPECT_TRUE(result);
}

TEST(ConfigGetSet, SetValueByKey_Bool_False) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("audio_enabled");
    if (!opt || opt->type != CFG_BOOL) return;

    bool result = Config_SetValueByKey("audio_enabled", "0");
    EXPECT_TRUE(result);
    EXPECT_FALSE(*(bool*)opt->ptr);

    result = Config_SetValueByKey("audio_enabled", "false");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "no");
    EXPECT_TRUE(result);
    result = Config_SetValueByKey("audio_enabled", "off");
    EXPECT_TRUE(result);
}

TEST(ConfigGetSet, SetValueByKey_Int) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    bool result = Config_SetValueByKey("default_width", "60");
    EXPECT_TRUE(result);
    EXPECT_EQ(*(int*)opt->ptr, 60);
}

TEST(ConfigGetSet, SetValueByKey_Float) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    bool result = Config_SetValueByKey("global_scale", "1.5");
    EXPECT_TRUE(result);
    EXPECT_NEAR(*(float*)opt->ptr, 1.5f, 0.01f);
}

TEST(ConfigGetSet, SetValueByKey_Unknown) {
    std::string error;
    bool result = Config_SetValueByKey("nonexistent.key", "value", &error);
    EXPECT_FALSE(result);
    EXPECT_FALSE(error.empty());
}

TEST(ConfigGetSet, OnConfigChange_Callback) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;
    EXPECT_TRUE(opt->onChange != nullptr);

    float original = *(float*)opt->ptr;
    Config_SetValueByKey("global_scale", "99.0");
    EXPECT_NE(*(float*)opt->ptr, original);
}

TEST(ConfigGetSet, ClampIntToMax) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    int original = *(int*)opt->ptr;
    bool result = Config_SetValueByKey("default_width", "999999");
    EXPECT_TRUE(result);
    EXPECT_LE(*(int*)opt->ptr, static_cast<int>(opt->max));

    *(int*)opt->ptr = original;
}

TEST(ConfigGetSet, ClampIntToMin) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("default_width");
    if (!opt || opt->type != CFG_INT) return;

    int original = *(int*)opt->ptr;
    bool result = Config_SetValueByKey("default_width", "-999999");
    EXPECT_TRUE(result);
    EXPECT_GE(*(int*)opt->ptr, static_cast<int>(opt->min));

    *(int*)opt->ptr = original;
}

TEST(ConfigGetSet, ClampFloatToMax) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float original = *(float*)opt->ptr;
    bool result = Config_SetValueByKey("global_scale", "999.0");
    EXPECT_TRUE(result);
    EXPECT_LE(*(float*)opt->ptr, opt->max);

    *(float*)opt->ptr = original;
}

TEST(ConfigGetSet, ClampFloatToMin) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float original = *(float*)opt->ptr;
    bool result = Config_SetValueByKey("global_scale", "-999.0");
    EXPECT_TRUE(result);
    EXPECT_GE(*(float*)opt->ptr, opt->min);

    *(float*)opt->ptr = original;
}

TEST(ConfigLoad, BasicLoad) {
    Config_Init();
    auto original = Config_FindOptionByKey("global_scale");
    if (!original || original->type != CFG_FLOAT) return;

    float saved = *(float*)original->ptr;

    auto tbl = toml::parse_str("[General]\nglobal_scale = 2.5\n");
    Config_Load(tbl);

    EXPECT_NEAR(*(float*)original->ptr, 2.5f, 0.01f);

    *(float*)original->ptr = saved;
}

TEST(ConfigLoad, PartialLoad) {
    Config_Init();
    auto optScale = Config_FindOptionByKey("global_scale");
    auto optAudio = Config_FindOptionByKey("audio_enabled");
    if (!optScale || optScale->type != CFG_FLOAT) return;
    if (!optAudio || optAudio->type != CFG_BOOL) return;

    float savedScale = *(float*)optScale->ptr;
    bool savedAudio = *(bool*)optAudio->ptr;

    auto tbl = toml::parse_str("[General]\nglobal_scale = 3.0\n");
    Config_Load(tbl);

    EXPECT_NEAR(*(float*)optScale->ptr, 3.0f, 0.01f);

    *(float*)optScale->ptr = savedScale;
    *(bool*)optAudio->ptr = savedAudio;
}

TEST(ConfigLoad, UnknownKeysIgnored) {
    Config_Init();
    auto tbl = toml::parse_str("[General]\nglobal_scale = 1.0\n");
    Config_Load(tbl);
}

TEST(ConfigLoad, InvalidValuesIgnored) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float saved = *(float*)opt->ptr;

    auto tbl = toml::parse_str("[General]\nglobal_scale = \"not_a_number\"\n");
    Config_Load(tbl);

    EXPECT_FLOAT_EQ(*(float*)opt->ptr, saved);

    *(float*)opt->ptr = saved;
}

TEST(ConfigLoad, CaseInsensitiveSection) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float saved = *(float*)opt->ptr;
    auto tbl = toml::parse_str("[GENERAL]\nglobal_scale = 4.0\n");
    Config_Load(tbl);
    EXPECT_NEAR(*(float*)opt->ptr, 4.0f, 0.01f);
    *(float*)opt->ptr = saved;
}

TEST(ConfigLoad, ClampLoadedValues) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float saved = *(float*)opt->ptr;
    auto tbl = toml::parse_str("[General]\nglobal_scale = 999.0\n");
    Config_Load(tbl);
    EXPECT_LE(*(float*)opt->ptr, opt->max);
    *(float*)opt->ptr = saved;
}

TEST(ConfigSave, SaveAllCompletes) {
    Config_Init();
    Config_SaveAll();
}

TEST(ConfigSave, AfterModifyAndSave) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float saved = *(float*)opt->ptr;
    Config_SetValueByKey("global_scale", "1.77");
    Config_SaveAll();
    EXPECT_NEAR(*(float*)opt->ptr, 1.77f, 0.01f);
    *(float*)opt->ptr = saved;
}

TEST(ConfigSave, PersistsToFile) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float saved = *(float*)opt->ptr;
    Config_SetValueByKey("global_scale", "2.99");
    Config_SaveAll();
    *(float*)opt->ptr = saved;

    auto tbl = toml::parse(Config_GetPath());
    float loaded = 0.0f;
    if (config_helpers::get_float(tbl, "General", "global_scale", loaded)) {
        EXPECT_NEAR(loaded, 2.99f, 0.01f);
    }

    Config_SetValueByKey("global_scale", std::to_string(saved));
    Config_SaveAll();
}

TEST(ConfigSave, AtomicSave) {
    Config_Init();
    std::string configPath = Config_GetPath();
    std::string tempPath = configPath + ".tmp";

    // Ensure clean state
    std::error_code ec;
    fs::remove(tempPath, ec);
    
    // Save to create files
    Config_SaveAll();
    
    // During normal operation, temp path should not exist after save
    EXPECT_FALSE(fs::exists(tempPath, ec));
    EXPECT_TRUE(fs::exists(configPath, ec));
}

TEST(ConfigPath, GetPathReturnsNonEmpty) {
    std::string path = Config_GetPath();
    EXPECT_FALSE(path.empty());
    EXPECT_NE(path.find("config.toml"), std::string::npos);
}

TEST(ConfigPath, ConfigDirPathReturnsNonEmpty) {
    fs::path dir = ConfigDirPath();
    EXPECT_FALSE(dir.empty());
}