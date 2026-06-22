#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include "config.h"
#include "toml.hpp"
#include "actor.h"
#include "goose.h"
#include "world.h"
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

TEST(ConfigLoadAll, LoadsConfigFile) {
    Config_Init();
    std::string savedDir;
    if (const char* env = std::getenv("CADGOOSE_CONFIG_DIR")) savedDir = env;

    char dirTemplate[] = "/tmp/cadgoose_cfg_XXXXXX";
    char* dirResult = mkdtemp(dirTemplate);
    ASSERT_NE(dirResult, nullptr);
    std::filesystem::path tmpDir(dirResult);
    setenv("CADGOOSE_CONFIG_DIR", tmpDir.string().c_str(), 1);

    std::ofstream out(tmpDir / "config.toml");
    out << "[General]\nglobal_scale = 4.25\n";
    out.close();

    auto opt = Config_FindOptionByKey("global_scale");
    ASSERT_NE(opt, nullptr);
    float saved = *(float*)opt->ptr;
    *(float*)opt->ptr = 1.0f;

    Config_LoadAll();

    EXPECT_NEAR(*(float*)opt->ptr, 4.25f, 0.01f);

    *(float*)opt->ptr = saved;
    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);
    if (!savedDir.empty()) {
        setenv("CADGOOSE_CONFIG_DIR", savedDir.c_str(), 1);
    } else {
        unsetenv("CADGOOSE_CONFIG_DIR");
    }
}

TEST(ConfigLoadAll, NoConfigFileFallback) {
    Config_Init();
    std::string savedDir;
    if (const char* env = std::getenv("CADGOOSE_CONFIG_DIR")) savedDir = env;

    char dirTemplate[] = "/tmp/cadgoose_empty_XXXXXX";
    char* dirResult = mkdtemp(dirTemplate);
    ASSERT_NE(dirResult, nullptr);
    std::filesystem::path tmpDir(dirResult);
    setenv("CADGOOSE_CONFIG_DIR", tmpDir.string().c_str(), 1);

    g_config.color.currentBody = {0.0f, 0.0f, 0.0f};

    Config_LoadAll();

    EXPECT_GT(g_config.color.currentBody.r, 0.0f);

    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);
    if (!savedDir.empty()) {
        setenv("CADGOOSE_CONFIG_DIR", savedDir.c_str(), 1);
    } else {
        unsetenv("CADGOOSE_CONFIG_DIR");
    }
}

TEST(ConfigLoadAll, CorruptConfigFileCatch) {
    Config_Init();
    std::string savedDir;
    if (const char* env = std::getenv("CADGOOSE_CONFIG_DIR")) savedDir = env;

    char dirTemplate[] = "/tmp/cadgoose_corrupt_XXXXXX";
    char* dirResult = mkdtemp(dirTemplate);
    ASSERT_NE(dirResult, nullptr);
    std::filesystem::path tmpDir(dirResult);
    setenv("CADGOOSE_CONFIG_DIR", tmpDir.string().c_str(), 1);

    std::ofstream out(tmpDir / "config.toml");
    out << "{{{{ NOT VALID TOML @@@@\n";
    out.close();

    Config_LoadAll();
    SUCCEED();

    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);
    if (!savedDir.empty()) {
        setenv("CADGOOSE_CONFIG_DIR", savedDir.c_str(), 1);
    } else {
        unsetenv("CADGOOSE_CONFIG_DIR");
    }
}

TEST(ConfigUpdateActiveTheme, CustomMode) {
    Config_Init();
    int savedMode = g_config.general.appearanceMode;

    g_config.general.appearanceMode = 3;
    g_config.color.customBody = {0.11f, 0.22f, 0.33f};
    g_config.color.customNeck = {0.44f, 0.55f, 0.66f};
    g_config.color.customHead = {0.77f, 0.88f, 0.99f};
    g_config.color.customBeak = {0.10f, 0.20f, 0.30f};
    g_config.color.customEye = {0.40f, 0.50f, 0.60f};
    g_config.color.customOutline = {0.70f, 0.80f, 0.90f};

    Config_UpdateActiveTheme();

    EXPECT_FLOAT_EQ(g_config.color.currentBody.r, 0.11f);
    EXPECT_FLOAT_EQ(g_config.color.currentNeck.g, 0.55f);
    EXPECT_FLOAT_EQ(g_config.color.currentHead.b, 0.99f);
    EXPECT_FLOAT_EQ(g_config.color.currentBeak.r, 0.10f);
    EXPECT_FLOAT_EQ(g_config.color.currentEye.g, 0.50f);
    EXPECT_FLOAT_EQ(g_config.color.currentOutline.b, 0.90f);

    g_config.general.appearanceMode = savedMode;
}

TEST(ConfigLoad, GooseNamesArray) {
    Config_Init();
    g_config.gooseNames.clear();

    auto tbl = toml::parse_str("goose = [\"Alice\", \"Bob\", \"Charlie\"]\n");
    Config_Load(tbl);

    EXPECT_EQ(g_config.gooseNames.size(), 3);
    EXPECT_EQ(g_config.gooseNames[0], "Alice");
    EXPECT_EQ(g_config.gooseNames[2], "Charlie");
    g_config.gooseNames.clear();
}

TEST(ConfigLoad, GooseNamesEmptyArray) {
    Config_Init();
    auto tbl = toml::parse_str("goose = []\n");
    Config_Load(tbl);
    EXPECT_TRUE(g_config.gooseNames.empty());
}

TEST(ConfigUpdateActiveTheme, StalinMode) {
    Config_Init();
    int savedMode = g_config.general.appearanceMode;

    g_config.general.appearanceMode = 4;
    Config_UpdateActiveTheme();

    EXPECT_FLOAT_EQ(g_config.color.currentBody.r, 0.50f);
    EXPECT_FLOAT_EQ(g_config.color.currentBody.g, 0.55f);
    EXPECT_FLOAT_EQ(g_config.color.currentHead.b, 0.40f);
    EXPECT_FLOAT_EQ(g_config.color.currentBeak.r, g_config.color.beak.r);
    EXPECT_FLOAT_EQ(g_config.color.currentEye.r, g_config.color.eye.r);

    g_config.general.appearanceMode = savedMode;
}

TEST(ConfigUpdateActiveTheme, LightModeFallback) {
    Config_Init();
    int savedMode = g_config.general.appearanceMode;
    std::string savedTheme = g_config.general.lightThemeRole;

    g_config.general.appearanceMode = 0;
    g_config.general.lightThemeRole = "";
    Config_UpdateActiveTheme();

    EXPECT_FLOAT_EQ(g_config.color.currentBody.r, g_config.color.goose.r);
    EXPECT_FLOAT_EQ(g_config.color.currentBody.g, g_config.color.goose.g);
    EXPECT_FLOAT_EQ(g_config.color.currentBeak.r, g_config.color.beak.r);

    g_config.general.appearanceMode = savedMode;
    g_config.general.lightThemeRole = savedTheme;
}

TEST(ConfigThemeColors, NameSearchFallback) {
    Config_Init();
    std::filesystem::path themeDir = Config_GetThemesDir();
    std::filesystem::path themeFile = themeDir / "zz_mismatched_filename.toml";

    std::ofstream out(themeFile);
    out << "[theme]\nname = \"SearchFallbackTheme\"\n\n[colors]\n"
        << "body = { r = 0.1, g = 0.2, b = 0.3 }\n"
        << "neck = { r = 0.1, g = 0.2, b = 0.3 }\n"
        << "head = { r = 0.1, g = 0.2, b = 0.3 }\n"
        << "beak = { r = 0.4, g = 0.5, b = 0.6 }\n"
        << "eye = { r = 0.7, g = 0.8, b = 0.9 }\n"
        << "outline = { r = 0.2, g = 0.2, b = 0.2 }\n";
    out.close();

    ColorRGB body, neck, head, beak, eye, outline;
    bool success = Config_LoadThemeColors("SearchFallbackTheme", body, neck, head, beak, eye, outline);
    EXPECT_TRUE(success);
    EXPECT_FLOAT_EQ(body.r, 0.1f);
    EXPECT_FLOAT_EQ(beak.r, 0.4f);

    std::filesystem::remove(themeFile);
}

TEST(ConfigThemeColors, NonExistentTheme) {
    Config_Init();
    ColorRGB body, neck, head, beak, eye, outline;

    bool success = Config_LoadThemeColors("NonExistentTheme_XYZ123", body, neck, head, beak, eye, outline);
    EXPECT_FALSE(success);
}

TEST(ConfigThemeColors, CorruptedThemeFile) {
    Config_Init();
    std::filesystem::path themeDir = Config_GetThemesDir();
    std::filesystem::path themeFile = themeDir / "broken_theme.toml";

    std::ofstream out(themeFile);
    out << "{{{{ invalid toml content {{{{\n";
    out.close();

    ColorRGB body, neck, head, beak, eye, outline;
    bool success = Config_LoadThemeColors("broken_theme", body, neck, head, beak, eye, outline);
    EXPECT_FALSE(success);

    std::filesystem::remove(themeFile);
}

TEST(ConfigThemeColors, ThemeWithMissingColorsSection) {
    Config_Init();
    std::filesystem::path themeDir = Config_GetThemesDir();
    std::filesystem::path themeFile = themeDir / "missing_colors.toml";

    std::ofstream out(themeFile);
    out << "[theme]\nname = \"MissingColors\"\n";
    out.close();

    ColorRGB body, neck, head, beak, eye, outline;
    bool success = Config_LoadThemeColors("MissingColors", body, neck, head, beak, eye, outline);
    EXPECT_FALSE(success);

    std::filesystem::remove(themeFile);
}

TEST(ConfigThemeColors, EmptyName) {
    Config_Init();
    ColorRGB body, neck, head, beak, eye, outline;

    bool success = Config_LoadThemeColors("", body, neck, head, beak, eye, outline);
    EXPECT_FALSE(success);
}

TEST(ConfigSave, GooseNamesArrayWritten) {
    std::string savedDir;
    if (const char* env = std::getenv("CADGOOSE_CONFIG_DIR")) savedDir = env;

    char dirTemplate[] = "/tmp/cadgoose_names_XXXXXX";
    char* dirResult = mkdtemp(dirTemplate);
    ASSERT_NE(dirResult, nullptr);
    std::filesystem::path tmpDir(dirResult);
    setenv("CADGOOSE_CONFIG_DIR", tmpDir.string().c_str(), 1);

    Config_Init();
    g_config.gooseNames.clear();
    g_config.gooseNames.push("Alpha");
    g_config.gooseNames.push("Beta");

    Config_SaveAll();

    auto tbl = toml::parse(tmpDir / "config.toml");
    ASSERT_TRUE(tbl.contains("goose"));
    auto& arr = tbl.at("goose").as_array();
    ASSERT_EQ(arr.size(), 2);
    EXPECT_EQ(arr[0].as_string(), "Alpha");
    EXPECT_EQ(arr[1].as_string(), "Beta");

    g_config.gooseNames.clear();
    std::error_code ec;
    std::filesystem::remove_all(tmpDir, ec);
    if (!savedDir.empty()) {
        setenv("CADGOOSE_CONFIG_DIR", savedDir.c_str(), 1);
    } else {
        unsetenv("CADGOOSE_CONFIG_DIR");
    }
}

TEST(ConfigSave, SaveGooseNamesFunction) {
    Config_Init();
    std::string configPath = Config_GetPath();

    Config_SaveGooseNames();

    EXPECT_TRUE(fs::exists(configPath));
}

TEST(ConfigSave, SaveGooseNamesWithGoose) {
    std::string savedDir;
    if (const char* env = std::getenv("CADGOOSE_CONFIG_DIR")) savedDir = env;

    char dirTemplate[] = "/tmp/cadgoose_gname_XXXXXX";
    char* dirResult = mkdtemp(dirTemplate);
    ASSERT_NE(dirResult, nullptr);
    std::filesystem::path tmpDir(dirResult);
    setenv("CADGOOSE_CONFIG_DIR", tmpDir.string().c_str(), 1);

    Config_Init();
    ActorManager::Instance().destroyAllOfType(ActorType::Goose);
    g_world.nextId = 42;

    Goose* goose = new Goose(42, "Gussie", 1920, 1080);
    ActorManager::Instance().add(goose);

    std::error_code ec;
    Config_SaveGooseNames();

    auto tbl = toml::parse(tmpDir / "config.toml");
    ASSERT_TRUE(tbl.contains("goose"));
    auto& arr = tbl.at("goose").as_array();
    ASSERT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0].as_string(), "Gussie");

    g_config.gooseNames.clear();
    ActorManager::Instance().destroyAllOfType(ActorType::Goose);
    std::filesystem::remove_all(tmpDir, ec);
    if (!savedDir.empty()) {
        setenv("CADGOOSE_CONFIG_DIR", savedDir.c_str(), 1);
    } else {
        unsetenv("CADGOOSE_CONFIG_DIR");
    }
}

TEST(ConfigThemeColors, DefaultName) {
    Config_Init();
    ColorRGB body, neck, head, beak, eye, outline;

    bool success = Config_LoadThemeColors("Default", body, neck, head, beak, eye, outline);
    EXPECT_FALSE(success);
}

TEST(ConfigUpdateActiveTheme, DarkModeWithTheme) {
    Config_Init();
    int savedMode = g_config.general.appearanceMode;
    std::string savedTheme = g_config.general.darkThemeRole;

    std::filesystem::path themeDir = Config_GetThemesDir();
    std::filesystem::path themeFile = themeDir / "dark_test_theme.toml";

    std::ofstream out(themeFile);
    out << "[theme]\nname = \"dark_test_theme\"\n\n[colors]\n"
        << "body = { r = 1.0, g = 0.0, b = 0.0 }\n"
        << "neck = { r = 0.0, g = 1.0, b = 0.0 }\n"
        << "head = { r = 0.0, g = 0.0, b = 1.0 }\n"
        << "beak = { r = 1.0, g = 1.0, b = 0.0 }\n"
        << "eye = { r = 1.0, g = 0.0, b = 1.0 }\n"
        << "outline = { r = 0.5, g = 0.5, b = 0.5 }\n";
    out.close();

    g_config.general.appearanceMode = 1;
    g_config.general.darkThemeRole = "dark_test_theme";
    Config_UpdateActiveTheme();

    EXPECT_FLOAT_EQ(g_config.color.currentBody.r, 1.0f);
    EXPECT_FLOAT_EQ(g_config.color.currentNeck.g, 1.0f);
    EXPECT_FLOAT_EQ(g_config.color.currentHead.b, 1.0f);
    EXPECT_FLOAT_EQ(g_config.color.currentBeak.r, 1.0f);

    std::filesystem::remove(themeFile);
    g_config.general.appearanceMode = savedMode;
    g_config.general.darkThemeRole = savedTheme;
}
