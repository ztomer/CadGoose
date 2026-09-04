#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include <cstdlib>
#include "config.h"

namespace fs = std::filesystem;

TEST(ConfigPath, EnvVarOverridesDefault) {
    // Save/restore, never bare-unset: this suite runs with the CADGOOSE_CONFIG_DIR
    // sandbox installed by main(), and a test that leaves the var unset drops every
    // LATER test's Config_SaveAll through the <cwd>/config fallback — i.e. into the
    // repo's own tracked config/config.toml (the mcp_port 31073->31072 flip).
    const char* savedConfigDir = std::getenv("CADGOOSE_CONFIG_DIR");
    std::string testDir = "/tmp/cadgoose-test-config";
    fs::create_directories(testDir);
    setenv("CADGOOSE_CONFIG_DIR", testDir.c_str(), 1);
    fs::path dir = ConfigDirPath();
    EXPECT_EQ(dir.string(), testDir);
    if (savedConfigDir) {
        setenv("CADGOOSE_CONFIG_DIR", savedConfigDir, 1);
    } else {
        unsetenv("CADGOOSE_CONFIG_DIR");
    }
    fs::remove_all(testDir);
}

TEST(ConfigPath, ThemesDirSucceeds) {
    fs::path dir = Config_GetThemesDir();
    EXPECT_FALSE(dir.empty());
    EXPECT_NE(dir.string().find("themes"), std::string::npos);
}

TEST(ConfigPath, ConfigFindOptionByKeyKnown) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("global_scale");
    ASSERT_NE(opt, nullptr);
    EXPECT_STREQ(opt->key, "global_scale");
}

TEST(ConfigPath, ConfigFindOptionByKeyUnknown) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("nonexistent_key");
    EXPECT_EQ(opt, nullptr);
}

TEST(ConfigPath, ConfigFindOptionByKeyCaseInsensitive) {
    Config_Init();
    const ConfigOption* opt = Config_FindOptionByKey("GLOBAL_SCALE");
    ASSERT_NE(opt, nullptr);
    EXPECT_STREQ(opt->key, "global_scale");
}

TEST(ConfigPath, SetStringConfigValue) {
    Config_Init();
    std::string orig = g_config.behaviors.honcker.hotkey;
    bool ok = Config_SetValueByKey("honcker_hotkey", "cmd+shift+h");
    EXPECT_TRUE(ok);
    EXPECT_EQ(g_config.behaviors.honcker.hotkey, "cmd+shift+h");
    g_config.behaviors.honcker.hotkey = orig;
}

TEST(ConfigPath, HomeFallbackWhenNoConfigToml) {
    const char* savedConfigDir = std::getenv("CADGOOSE_CONFIG_DIR");
    fs::path savedCwd = fs::current_path();
    unsetenv("CADGOOSE_CONFIG_DIR");
    std::string tempDir = "/tmp/cadgoose-home-test";
    fs::create_directories(tempDir);
    fs::current_path(tempDir);
    fs::path result = ConfigDirPath();
    fs::current_path(savedCwd);
    if (savedConfigDir) {
        setenv("CADGOOSE_CONFIG_DIR", savedConfigDir, 1);
    } else {
        unsetenv("CADGOOSE_CONFIG_DIR");
    }
    fs::remove_all(tempDir);
    const char* home = std::getenv("HOME");
    ASSERT_NE(home, nullptr);
    fs::path expected = fs::path(home) / "Library" / "Application Support" / "CadGoose";
    EXPECT_EQ(result, expected);
}

TEST(ConfigPath, HomeFallbackWhenHomeUnset) {
    const char* savedConfigDir = std::getenv("CADGOOSE_CONFIG_DIR");
    const char* savedHome = std::getenv("HOME");
    fs::path savedCwd = fs::current_path();
    unsetenv("CADGOOSE_CONFIG_DIR");
    unsetenv("HOME");
    std::string tempDir = "/tmp/cadgoose-home-unset-test";
    fs::create_directories(tempDir);
    fs::current_path(tempDir);
    fs::path expected = fs::current_path();
    fs::path result = ConfigDirPath();
    fs::current_path(savedCwd);
    if (savedHome) setenv("HOME", savedHome, 1);
    if (savedConfigDir) setenv("CADGOOSE_CONFIG_DIR", savedConfigDir, 1);
    fs::remove_all(tempDir);
    EXPECT_EQ(result, expected);
}
