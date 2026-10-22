#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include "config.h"
#include "app_identity.h"

namespace fs = std::filesystem;

// The settings directory of the installed app is named after its bundle id
// (ROADMAP D1.1). Before the move it was the shared
// ~/Library/Application Support/CadGoose, which the macOS successor fork also
// reads and writes: two apps, one config.toml, two schemas.
//
// No test here may setenv/unsetenv/chdir: the process environment and cwd are
// shared with every thread in the binary (ROADMAP G0.12). The resolution is
// tested through ResolveConfigDir over explicit sources, and the migration
// through explicit paths.
namespace {

// A temp dir this test owns, removed on every exit path.
struct OwnedTempDir {
    fs::path path;
    OwnedTempDir() {
        std::string tmpl = (fs::temp_directory_path() / "cadgoose-migrate-XXXXXX").string();
        path = mkdtemp(tmpl.data()) != nullptr ? fs::path(tmpl) : fs::path();
    }
    ~OwnedTempDir() {
        std::error_code ec;
        if (!path.empty()) fs::remove_all(path, ec);
    }
    OwnedTempDir(const OwnedTempDir&) = delete;
    OwnedTempDir& operator=(const OwnedTempDir&) = delete;
};

void WriteFile(const fs::path& file, const std::string& content) {
    fs::create_directories(file.parent_path());
    std::ofstream out(file);
    out << content;
}

std::string ReadFile(const fs::path& file) {
    std::ifstream in(file);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

}  // namespace

TEST(ConfigMigration, ThisAppsDirectoryIsNamedAfterItsBundleId) {
    ConfigDirSources src;
    src.home = "/Users/nobody";
    src.cwd = "/nonexistent-cwd";
#if defined(__APPLE__)
    EXPECT_EQ(ResolveConfigDir(src),
              fs::path("/Users/nobody") / "Library" / "Application Support" / CADGOOSE_BUNDLE_ID);
#else
    EXPECT_EQ(ResolveConfigDir(src), fs::path("/Users/nobody") / ".config" / "desktop-goose");
#endif
}

TEST(ConfigMigration, EachBundleIdGetsItsOwnDirectory) {
    // The isolation is a property of the id, not of a hardcoded path: two apps
    // with two ids cannot land in one directory.
    ConfigDirSources src;
    src.home = "/Users/nobody";
    src.cwd = "/nonexistent-cwd";
    const fs::path mine = ResolveConfigDir(src, CADGOOSE_BUNDLE_ID);
    const fs::path theirs = ResolveConfigDir(src, "com.example.OtherGoose");
#if defined(__APPLE__)
    EXPECT_NE(mine, theirs);
    EXPECT_EQ(mine.parent_path(),
              fs::path("/Users/nobody") / "Library" / "Application Support");
    EXPECT_EQ(theirs.parent_path(), mine.parent_path());
    EXPECT_EQ(theirs.filename(), "com.example.OtherGoose");
#else
    // The Linux path is not bundle-id based; keeping the assertion here in the
    // same shape would make this file fail if it were ever built there.
    EXPECT_EQ(mine, theirs);
#endif
}

TEST(ConfigMigration, TheSharedDirectoryIsNotThisAppsDirectory) {
    // The shared directory is the one this app used to use, and the one the
    // macOS successor fork still uses. This app's own directory is not it.
    ConfigDirSources src;
    src.home = "/Users/nobody";
    src.cwd = "/nonexistent-cwd";
    const auto shared = LegacyConfigDirPath(src);
#if defined(__APPLE__)
    ASSERT_TRUE(shared.has_value());
    EXPECT_EQ(*shared,
              fs::path("/Users/nobody") / "Library" / "Application Support" / "CadGoose");
    EXPECT_NE(ResolveConfigDir(src), *shared);
#else
    EXPECT_FALSE(shared.has_value());
#endif
}

TEST(ConfigMigration, OldDirectoryPresentNewAbsentIsCopied) {
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path legacy = root.path / "CadGoose";
    const fs::path fresh = root.path / "com.desktoppad.CadGoose";
    WriteFile(legacy / "config.toml", "goose = [\"Shagoose\"]\n");
    WriteFile(legacy / "themes" / "gangland.toml", "[theme]\nname = \"gangland\"\n");

    EXPECT_EQ(MigrateLegacyConfigDir(legacy, fresh), ConfigMigrationOutcome::kMigrated);

    // The settings are in this app's own directory now...
    EXPECT_TRUE(fs::exists(fresh / "config.toml"));
    EXPECT_EQ(ReadFile(fresh / "config.toml"), "goose = [\"Shagoose\"]\n");
    EXPECT_TRUE(fs::exists(fresh / "themes" / "gangland.toml"));
    // ...and the shared directory is still there: the app that reads it needs it.
    EXPECT_TRUE(fs::exists(legacy / "config.toml"));
    EXPECT_TRUE(fs::exists(legacy / "themes" / "gangland.toml"));
}

TEST(ConfigMigration, BothPresentTouchesNothing) {
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path legacy = root.path / "CadGoose";
    const fs::path mine = root.path / "com.desktoppad.CadGoose";
    WriteFile(legacy / "config.toml", "shared\n");
    WriteFile(mine / "config.toml", "mine\n");

    EXPECT_EQ(MigrateLegacyConfigDir(legacy, mine), ConfigMigrationOutcome::kNothingToMigrate);

    // This app's own settings are untouched, and the shared ones are too.
    EXPECT_EQ(ReadFile(mine / "config.toml"), "mine\n");
    EXPECT_EQ(ReadFile(legacy / "config.toml"), "shared\n");
    // Nothing was copied in: the shared themes/ did not arrive.
    EXPECT_FALSE(fs::exists(mine / "themes"));
}

TEST(ConfigMigration, NeitherPresentDoesNothing) {
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path legacy = root.path / "CadGoose";
    const fs::path mine = root.path / "com.desktoppad.CadGoose";

    EXPECT_EQ(MigrateLegacyConfigDir(legacy, mine), ConfigMigrationOutcome::kNothingToMigrate);
    EXPECT_FALSE(fs::exists(mine));
    EXPECT_FALSE(fs::exists(legacy));
}

TEST(ConfigMigration, RunningTwiceMigratesOnce) {
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path legacy = root.path / "CadGoose";
    const fs::path mine = root.path / "com.desktoppad.CadGoose";
    WriteFile(legacy / "config.toml", "shared\n");

    EXPECT_EQ(MigrateLegacyConfigDir(legacy, mine), ConfigMigrationOutcome::kMigrated);
    WriteFile(legacy / "config.toml", "shared again\n");
    EXPECT_EQ(MigrateLegacyConfigDir(legacy, mine), ConfigMigrationOutcome::kNothingToMigrate);
    // The second run copied nothing: the first run's copy stands.
    EXPECT_EQ(ReadFile(mine / "config.toml"), "shared\n");
}

TEST(ConfigMigration, RuntimeLogsAreNotCopied) {
    // The logs are this run's output, not settings: the shared ones stay in the
    // shared directory.
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path legacy = root.path / "CadGoose";
    const fs::path mine = root.path / "com.desktoppad.CadGoose";
    WriteFile(legacy / "logs" / "session-20260101_000000.log", "noise\n");
    WriteFile(legacy / "config.toml", "shared\n");

    EXPECT_EQ(MigrateLegacyConfigDir(legacy, mine), ConfigMigrationOutcome::kMigrated);
    EXPECT_FALSE(fs::exists(mine / "logs"));
    EXPECT_TRUE(fs::exists(legacy / "logs" / "session-20260101_000000.log"));
}

TEST(ConfigMigration, ModelWeightsAreLinkedNotCopied) {
    // The weights are 633 MB on the owner's machine (measured 2026-10-22) and
    // both apps read them, so they are linked, not copied.
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path legacy = root.path / "CadGoose";
    const fs::path mine = root.path / "com.desktoppad.CadGoose";
    WriteFile(legacy / "Models" / "model.mlmodelc" / "weights.bin", "big\n");
    WriteFile(legacy / "config.toml", "shared\n");

    EXPECT_EQ(MigrateLegacyConfigDir(legacy, mine), ConfigMigrationOutcome::kMigrated);
    EXPECT_TRUE(fs::is_symlink(mine / "Models"));
    EXPECT_EQ(fs::read_symlink(mine / "Models"), legacy / "Models");
    // The weights are where they were: nothing was copied.
    EXPECT_TRUE(fs::exists(legacy / "Models" / "model.mlmodelc" / "weights.bin"));
}

// The decision the running app makes at startup: the shared directory is
// migrated only when the app is the installed app reading its own directory.
// The shared directory is a macOS thing (the Linux path is not bundle-id
// based), so these are macOS cases.
#if defined(__APPLE__)
TEST(ConfigMigration, TheInstalledAppMigratesTheSharedDirectory) {
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path support = root.path / "Library" / "Application Support";
    WriteFile(support / "CadGoose" / "config.toml", "shared\n");

    ConfigDirSources src;
    src.home = root.path.string();
    src.cwd = root.path / "cwd";  // empty: not a development tree

    MigrateLegacySettings(src);

    EXPECT_TRUE(fs::exists(support / "com.desktoppad.CadGoose" / "config.toml"));
    EXPECT_EQ(ReadFile(support / "com.desktoppad.CadGoose" / "config.toml"), "shared\n");
}

TEST(ConfigMigration, AnAppPointedElsewhereMigratesNothing) {
    // The test suite points the app at its own sandbox: it must never touch
    // the user's real shared directory.
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path support = root.path / "Library" / "Application Support";
    WriteFile(support / "CadGoose" / "config.toml", "shared\n");

    ConfigDirSources src;
    src.home = root.path.string();
    src.cwd = root.path / "cwd";
    src.configDirEnv = (root.path / "elsewhere").string();

    MigrateLegacySettings(src);

    EXPECT_FALSE(fs::exists(support / "com.desktoppad.CadGoose"));
    EXPECT_FALSE(fs::exists(root.path / "elsewhere"));
}

TEST(ConfigMigration, TheDevelopmentTreeMigratesNothing) {
    // Started from a checkout, the app reads ./config: the user's shared
    // directory is none of its business.
    OwnedTempDir root;
    ASSERT_FALSE(root.path.empty());
    const fs::path support = root.path / "Library" / "Application Support";
    WriteFile(support / "CadGoose" / "config.toml", "shared\n");
    WriteFile(root.path / "cwd" / "config" / "config.toml", "dev\n");

    ConfigDirSources src;
    src.home = root.path.string();
    src.cwd = root.path / "cwd";

    MigrateLegacySettings(src);

    EXPECT_FALSE(fs::exists(support / "com.desktoppad.CadGoose"));
}
#endif  // __APPLE__
