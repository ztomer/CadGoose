// Config schema versioning and the one-shot migrations it gates (ROADMAP M1.1).
//
// The class under test: a persisted setting must hold across a load. Before
// M1.1, Config_LoadAll forced audio_enabled = true on every macOS load, so a
// user's `false` never survived a restart. The repair it stood in for (files
// written by the old `false` default) is now a versioned migration that runs
// once per file: the next save stamps the current schema version.
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "config.h"
#include "toml.hpp"

namespace fs = std::filesystem;

namespace {

// A private config dir for one test, restored and removed on every exit path.
class ConfigSchemaTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        char dirTemplate[] = "/tmp/cadgoose_schema_XXXXXX";
        char* dirResult = mkdtemp(dirTemplate);
        ASSERT_NE(dirResult, nullptr);
        m_dir = dirResult;
        m_override.emplace(m_dir);
        m_savedAudio = g_config.general.audioEnabled;
    }
    void TearDown() override {
        g_config.general.audioEnabled = m_savedAudio;
        m_override.reset();
        std::error_code ec;
        fs::remove_all(m_dir, ec);
    }
    void WriteConfig(const std::string& body) {
        std::ofstream out(m_dir / "config.toml");
        out << body;
    }
    std::string ReadConfig() {
        std::ifstream in(m_dir / "config.toml");
        std::stringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    fs::path m_dir;
    std::optional<ScopedConfigDirOverride> m_override;
    bool m_savedAudio = true;
};

}  // namespace

TEST_F(ConfigSchemaTest, AudioDisabledHoldsAcrossLoad) {
    WriteConfig("[meta]\nschema_version = " + std::to_string(kConfigSchemaVersion) +
                "\n[General]\naudio_enabled = false\n");
    g_config.general.audioEnabled = true;
    Config_LoadAll();
    EXPECT_FALSE(g_config.general.audioEnabled);
}

TEST_F(ConfigSchemaTest, AudioDisabledSurvivesSaveAndReload) {
    g_config.general.audioEnabled = false;
    Config_SaveAll();
    g_config.general.audioEnabled = true;
    Config_LoadAll();
    EXPECT_FALSE(g_config.general.audioEnabled);
}

TEST_F(ConfigSchemaTest, SaveStampsCurrentSchemaVersion) {
    Config_SaveAll();
    auto parsed = toml::parse(m_dir / "config.toml");
    EXPECT_EQ(Config_SchemaVersionOf(parsed), kConfigSchemaVersion);
}

TEST_F(ConfigSchemaTest, UnversionedFileIsVersionOne) {
    auto tbl = toml::parse_str("[General]\nglobal_scale = 1.0\n");
    EXPECT_EQ(Config_SchemaVersionOf(tbl), 1);
}

// The old poisoned default: a version-1 file carrying audio_enabled = false is
// repaired once, and the repair is persisted by the next save so it never runs
// again for that file.
TEST_F(ConfigSchemaTest, OldSchemaAudioFalseMigratesExactlyOnce) {
    WriteConfig("[General]\naudio_enabled = false\n");
    g_config.general.audioEnabled = false;
    Config_LoadAll();
    EXPECT_TRUE(g_config.general.audioEnabled) << "v1 file: migration repairs the old default";

    Config_SaveAll();  // stamps the current version
    g_config.general.audioEnabled = false;
    Config_SaveAll();  // the user now chooses false
    g_config.general.audioEnabled = true;
    Config_LoadAll();
    EXPECT_FALSE(g_config.general.audioEnabled) << "v2 file: the user's false holds";
}

TEST_F(ConfigSchemaTest, MigrationLeavesOtherKeysAlone) {
    auto tbl = toml::parse_str("[General]\naudio_enabled = false\naudio_muted = true\n");
    auto migrated = Config_MigrateSchema(tbl);
    EXPECT_EQ(Config_SchemaVersionOf(migrated), kConfigSchemaVersion);
    EXPECT_TRUE(toml::find<bool>(migrated, "General", "audio_enabled"));
    EXPECT_TRUE(toml::find<bool>(migrated, "General", "audio_muted"));
}

TEST_F(ConfigSchemaTest, MigrationOfCurrentVersionIsIdentity) {
    auto tbl = toml::parse_str("[meta]\nschema_version = " + std::to_string(kConfigSchemaVersion) +
                               "\n[General]\naudio_enabled = false\n");
    auto migrated = Config_MigrateSchema(tbl);
    EXPECT_FALSE(toml::find<bool>(migrated, "General", "audio_enabled"));
}
