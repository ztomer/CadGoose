// ROADMAP M1.5: the app start path must read back what OnConfigChange saved.
// Linux called Config_Init() and never Config_LoadAll(), so every Linux
// setting was written to config.toml and lost at the next start. This suite
// runs on BOTH platforms (CadGooseTests on macOS, CadGooseLinuxTests on
// Linux): it saves a value, puts the default back as a fresh process would
// have it, runs the start path, and reads the saved value back.
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include "config.h"

namespace fs = std::filesystem;

namespace {

class ConfigStartup : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        char dirTemplate[] = "/tmp/cadgoose_startup_XXXXXX";
        char* dirResult = mkdtemp(dirTemplate);
        ASSERT_NE(dirResult, nullptr);
        m_dir = dirResult;
        m_override.emplace(m_dir);
        m_savedScale = g_config.general.globalScale;
        m_savedBall = g_config.behaviors.fun.ball;
    }
    void TearDown() override {
        g_config.general.globalScale = m_savedScale;
        g_config.behaviors.fun.ball = m_savedBall;
        m_override.reset();
        std::error_code ec;
        fs::remove_all(m_dir, ec);
    }
    fs::path m_dir;
    std::optional<ScopedConfigDirOverride> m_override;
    float m_savedScale = 0.0F;
    bool m_savedBall = false;
};

}  // namespace

TEST_F(ConfigStartup, StartPathReadsBackWhatWasSaved) {
    const float savedScale = m_savedScale + 0.75F;
    const bool savedBall = !m_savedBall;
    g_config.general.globalScale = savedScale;
    g_config.behaviors.fun.ball = savedBall;
    Config_SaveAll();
    ASSERT_TRUE(fs::exists(Config_GetPath())) << "save wrote nothing to " << Config_GetPath();

    // A fresh process: the defaults, not the values this one last held.
    g_config.general.globalScale = m_savedScale;
    g_config.behaviors.fun.ball = m_savedBall;

    Config_Startup();

    EXPECT_NEAR(g_config.general.globalScale, savedScale, 0.01F)
        << "the start path never read config.toml";
    EXPECT_EQ(g_config.behaviors.fun.ball, savedBall)
        << "the start path never read config.toml";
}
