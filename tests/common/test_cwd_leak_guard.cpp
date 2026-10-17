#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

#include "../cwd_leak_guard.h"

namespace fs = std::filesystem;

// The guard's pure half, driven on a temp dir this test owns (never the cwd:
// that is what the guard itself watches).
namespace {

struct GuardTempDir {
    fs::path path;
    GuardTempDir() {
        std::string tmpl = (fs::temp_directory_path() / "cadgoose-cwdguard-XXXXXX").string();
        path = mkdtemp(tmpl.data()) != nullptr ? fs::path(tmpl) : fs::path();
    }
    ~GuardTempDir() {
        std::error_code ec;
        if (!path.empty()) fs::remove_all(path, ec);
    }
    GuardTempDir(const GuardTempDir&) = delete;
    GuardTempDir& operator=(const GuardTempDir&) = delete;
};

}  // namespace

TEST(CwdLeakGuard, UnchangedTreeHasNoFindings) {
    GuardTempDir root;
    ASSERT_FALSE(root.path.empty());
    fs::create_directories(root.path / "config");
    { std::ofstream(root.path / "config" / "config.toml") << "a = 1\n"; }
    const auto snap = CwdLeakGuard::Take(root.path);
    EXPECT_TRUE(CwdLeakGuard::Diff(snap, CwdLeakGuard::Take(root.path)).empty());
}

TEST(CwdLeakGuard, NewTopLevelEntryIsReported) {
    // The G0.12 signature: a directory created through a relative path.
    GuardTempDir root;
    ASSERT_FALSE(root.path.empty());
    const auto before = CwdLeakGuard::Take(root.path);
    fs::create_directories(root.path / "e-home-unset-test" / "themes");
    const auto diff = CwdLeakGuard::Diff(before, CwdLeakGuard::Take(root.path));
    ASSERT_EQ(diff.size(), 1u);
    EXPECT_EQ(diff[0], "+ e-home-unset-test");
}

TEST(CwdLeakGuard, RewriteUnderConfigIsReported) {
    // The mcp_port flip signature: an existing tracked config file rewritten.
    GuardTempDir root;
    ASSERT_FALSE(root.path.empty());
    fs::create_directories(root.path / "config");
    { std::ofstream(root.path / "config" / "config.toml") << "mcp_port = 31073\n"; }
    const auto before = CwdLeakGuard::Take(root.path);
    { std::ofstream(root.path / "config" / "config.toml") << "mcp_port = 31072\nmore = 1\n"; }
    { std::ofstream(root.path / "config" / "new.toml") << "\n"; }
    const auto diff = CwdLeakGuard::Diff(before, CwdLeakGuard::Take(root.path));
    ASSERT_EQ(diff.size(), 2u);
    EXPECT_EQ(diff[0], "~ config/config.toml");
    EXPECT_EQ(diff[1], "+ config/new.toml");
}

TEST(CwdLeakGuard, RemovedEntryIsReported) {
    GuardTempDir root;
    ASSERT_FALSE(root.path.empty());
    { std::ofstream(root.path / "keep.txt") << "x\n"; }
    const auto before = CwdLeakGuard::Take(root.path);
    fs::remove(root.path / "keep.txt");
    const auto diff = CwdLeakGuard::Diff(before, CwdLeakGuard::Take(root.path));
    ASSERT_EQ(diff.size(), 1u);
    EXPECT_EQ(diff[0], "- keep.txt");
}
