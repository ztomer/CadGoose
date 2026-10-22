// config_migration.cpp — the one-time move out of the shared settings directory.
//
// Until this app moved, it read and wrote ~/Library/Application Support/CadGoose
// — the same directory the macOS successor fork uses. Two apps in one directory
// share one config.toml and one themes/, and the two schemas have diverged
// (ROADMAP D1.1). This app's own directory is now named after its bundle id,
// and the shared directory's settings are copied over once.
//
// The copy, not the move: the app that still reads the shared directory needs
// it. "This app's own directory already exists" is the marker that says the
// migration is done, and nothing is touched when it is there.
#include "config.h"
#include "app_identity.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

// The entries of the shared directory that are not settings.
//
//   logs/    runtime output (session and crash logs). This app writes its own.
//   Models/  the downloaded model weights: 633 MB on the owner's machine
//            (measured 2026-10-22). They are read-mostly state that both apps
//            use, so they are LINKED into this app's directory, not copied
//            into it: a copy would double 633 MB on the first launch.
constexpr const char* kLogsDirName = "logs";
constexpr const char* kModelsDirName = "Models";

}  // namespace

ConfigMigrationOutcome MigrateLegacyConfigDir(const fs::path& legacyDir,
                                              const fs::path& newDir) {
    std::error_code ec;
    if (legacyDir.empty() || newDir.empty() || legacyDir == newDir) {
        return ConfigMigrationOutcome::kNothingToMigrate;
    }
    // Both present: this app's own directory is already there, so the
    // migration is done, and the shared directory is the other app's.
    if (fs::exists(newDir, ec)) return ConfigMigrationOutcome::kNothingToMigrate;
    if (!fs::exists(legacyDir, ec)) return ConfigMigrationOutcome::kNothingToMigrate;

    // The copy lands in a staging directory and is renamed into place: a
    // directory rename is atomic, so a second process racing this one either
    // wins (and this copy is discarded) or finds the directory already there.
    fs::path staging = newDir;
    staging += ".migrating-" + std::to_string(static_cast<long long>(getpid()));
    fs::remove_all(staging, ec);
    ec.clear();
    fs::create_directories(staging, ec);
    if (ec) return ConfigMigrationOutcome::kFailed;

    for (const auto& entry : fs::directory_iterator(legacyDir, ec)) {
        const std::string name = entry.path().filename().string();
        if (name == kLogsDirName) continue;
        if (name == kModelsDirName && entry.is_directory()) {
            fs::create_directory_symlink(entry.path(), staging / name, ec);
            if (ec) {
                fs::remove_all(staging, ec);
                return ConfigMigrationOutcome::kFailed;
            }
            continue;
        }
        fs::copy(entry.path(), staging / name, fs::copy_options::recursive, ec);
        if (ec) {
            fs::remove_all(staging, ec);
            return ConfigMigrationOutcome::kFailed;
        }
    }
    if (ec) {
        fs::remove_all(staging, ec);
        return ConfigMigrationOutcome::kFailed;
    }

    ec.clear();
    fs::rename(staging, newDir, ec);
    if (ec) {
        // The directory is not empty: another process got there first and its
        // copy is the one that stands.
        const bool someoneElseWon = fs::exists(newDir, ec);
        fs::remove_all(staging, ec);
        return someoneElseWon ? ConfigMigrationOutcome::kNothingToMigrate
                               : ConfigMigrationOutcome::kFailed;
    }
    return ConfigMigrationOutcome::kMigrated;
}

void MigrateLegacySettings(const ConfigDirSources& src) {
    // A directory the app was pointed at on purpose is not the installed app's
    // business (the test suite points at its own sandbox; a user can point at
    // another directory).
    if (src.configDirEnv) return;
    const auto installed = InstalledConfigDirPath(src, CADGOOSE_BUNDLE_ID);
    if (!installed) return;  // no home directory, or a platform without one
    const auto legacy = LegacyConfigDirPath(src);
    if (!legacy) return;
    // The development tree (./config) is the developer's, not the user's: the
    // app must not migrate the user's settings because it was started from a
    // checkout.
    if (ResolveConfigDir(src, CADGOOSE_BUNDLE_ID) != *installed) return;
    MigrateLegacyConfigDir(*legacy, *installed);
}

void MigrateLegacySettings() {
    MigrateLegacySettings(ConfigDirSources_FromProcess());
}
