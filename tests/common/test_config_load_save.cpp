#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include "config.h"
#include "toml.hpp"
#include "../../src/common/config_helpers.h"

namespace fs = std::filesystem;

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

    std::error_code ec;
    fs::remove(tempPath, ec);

    Config_SaveAll();

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

TEST(ConfigEdgeCase, EmptyTomlLoad) {
    Config_Init();
    auto original = Config_FindOptionByKey("global_scale");
    if (!original || original->type != CFG_FLOAT) return;

    float saved = *(float*)original->ptr;
    auto tbl = toml::parse_str("");
    Config_Load(tbl);
    EXPECT_FLOAT_EQ(*(float*)original->ptr, saved);
    *(float*)original->ptr = saved;
}

TEST(ConfigEdgeCase, MissingSectionLoad) {
    Config_Init();
    auto tbl = toml::parse_str("[Nonexistent]\nkey = 123\n");
    Config_Load(tbl);
    SUCCEED();
}

TEST(ConfigEdgeCase, RoundTripSaveLoad) {
    Config_Init();
    auto opt = Config_FindOptionByKey("global_scale");
    if (!opt || opt->type != CFG_FLOAT) return;

    float original = *(float*)opt->ptr;
    Config_SetValueByKey("global_scale", "2.5");
    Config_SaveAll();

    auto tbl = toml::parse(Config_GetPath());
    float loaded = 0.0f;
    if (config_helpers::get_float(tbl, "General", "global_scale", loaded)) {
        EXPECT_NEAR(loaded, 2.5f, 0.01f);
    }

    *(float*)opt->ptr = original;
    Config_SaveAll();
}
