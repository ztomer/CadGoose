#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include "config.h"

namespace fs = std::filesystem;

// ConfigDirPath() is tested through ResolveConfigDir over explicit sources.
// No test here may setenv/unsetenv/chdir: the process environment and cwd are
// shared with every thread in the binary, and the old versions of these tests
// mutated both mid-suite (ROADMAP G0.12; tools/check_test_process_state.py
// keeps it that way).
namespace {

// A temp dir this test owns, removed on every exit path.
struct OwnedTempDir {
    fs::path path;
    OwnedTempDir() {
        std::string tmpl = (fs::temp_directory_path() / "cadgoose-cfgdir-XXXXXX").string();
        path = mkdtemp(tmpl.data()) != nullptr ? fs::path(tmpl) : fs::path();
    }
    ~OwnedTempDir() {
        std::error_code ec;
        if (!path.empty()) fs::remove_all(path, ec);
    }
    OwnedTempDir(const OwnedTempDir&) = delete;
    OwnedTempDir& operator=(const OwnedTempDir&) = delete;
};

}  // namespace

TEST(ConfigPath, EnvVarOverridesDefault) {
    ConfigDirSources src;
    src.configDirEnv = "/somewhere/cadgoose-test-config";
    src.home = "/Users/nobody";
    src.cwd = "/nonexistent-cwd";
    EXPECT_EQ(ResolveConfigDir(src), fs::path("/somewhere/cadgoose-test-config"));
}

TEST(ConfigPath, EmptyEnvValuesAreUnset) {
    // An empty CADGOOSE_CONFIG_DIR used to resolve to the relative path "",
    // i.e. wherever the process happened to be.
    const fs::path cwd = "/nonexistent-cwd";
    ConfigDirSources src = ConfigDirSources_FromEnv(
        [](const char* name) -> const char* {
            if (std::string(name) == "CADGOOSE_CONFIG_DIR") return "";
            if (std::string(name) == "HOME") return "/Users/nobody";
            return nullptr;
        },
        cwd);
    EXPECT_FALSE(src.configDirEnv.has_value());
    ASSERT_TRUE(src.home.has_value());
    EXPECT_EQ(*src.home, "/Users/nobody");
    EXPECT_TRUE(ResolveConfigDir(src).is_absolute());
}

TEST(ConfigPath, CwdConfigTomlWinsOverHome) {
    OwnedTempDir cwd;
    ASSERT_FALSE(cwd.path.empty());
    fs::create_directories(cwd.path / "config");
    { std::ofstream(cwd.path / "config" / "config.toml") << "\n"; }
    ConfigDirSources src;
    src.home = "/Users/nobody";
    src.cwd = cwd.path;
    EXPECT_EQ(ResolveConfigDir(src), cwd.path / "config");
}

TEST(ConfigPath, ProcessSourcesMatchTheSuiteSandbox) {
    // main() installs CADGOOSE_CONFIG_DIR before any thread starts; the
    // impure wrapper must read it back unchanged.
    ConfigDirSources src = ConfigDirSources_FromProcess();
    ASSERT_TRUE(src.configDirEnv.has_value());
    EXPECT_EQ(ConfigDirPath(), fs::path(*src.configDirEnv));
}

TEST(ConfigPath, OverrideWinsAndRestores) {
    const fs::path before = ConfigDirPath();
    {
        ScopedConfigDirOverride guard("/tmp/cadgoose-override-probe");
        EXPECT_EQ(ConfigDirPath(), fs::path("/tmp/cadgoose-override-probe"));
        {
            ScopedConfigDirOverride inner("/tmp/cadgoose-override-inner");
            EXPECT_EQ(ConfigDirPath(), fs::path("/tmp/cadgoose-override-inner"));
        }
        EXPECT_EQ(ConfigDirPath(), fs::path("/tmp/cadgoose-override-probe"));
    }
    EXPECT_EQ(ConfigDirPath(), before);
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
    OwnedTempDir cwd;  // empty: no config/config.toml
    ASSERT_FALSE(cwd.path.empty());
    ConfigDirSources src;
    src.home = "/Users/nobody";
    src.cwd = cwd.path;
#if defined(__APPLE__)
    EXPECT_EQ(ResolveConfigDir(src),
              fs::path("/Users/nobody") / "Library" / "Application Support" / "CadGoose");
#else
    EXPECT_EQ(ResolveConfigDir(src), fs::path("/Users/nobody") / ".config" / "desktop-goose");
#endif
}

TEST(ConfigPath, HomeFallbackWhenHomeUnset) {
    OwnedTempDir cwd;
    ASSERT_FALSE(cwd.path.empty());
    ConfigDirSources src;  // no env at all
    src.cwd = cwd.path;
    EXPECT_EQ(ResolveConfigDir(src), cwd.path);
}
