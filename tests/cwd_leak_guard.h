#ifndef CADGOOSE_TESTS_CWD_LEAK_GUARD_H
#define CADGOOSE_TESTS_CWD_LEAK_GUARD_H

// No test may write outside a temp dir it owns (ROADMAP G0.12). The suite runs
// with cwd = the repo root (ctest's WORKING_DIRECTORY) or build/ (the CI
// script), so any write through a relative or cwd-derived path lands in a
// working tree. This listener snapshots the cwd's top-level entries and the
// whole of <cwd>/config/ (ConfigDirPath()'s cwd fallback) around EVERY test,
// and names the test after which something appeared, vanished or changed.
// It is a gate, not a log: main() turns any finding into a nonzero exit.

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class CwdLeakGuard : public ::testing::EmptyTestEventListener {
public:
    // Entry name -> a fingerprint (kind, size, mtime). Pure over the tree, so
    // the self-test can drive it on a temp dir.
    using Snapshot = std::map<std::string, std::string>;

    static Snapshot Take(const std::filesystem::path& root);
    // Human-readable differences between two snapshots; empty when equal.
    static std::vector<std::string> Diff(const Snapshot& before, const Snapshot& after);

    void OnTestStart(const ::testing::TestInfo& info) override;
    void OnTestEnd(const ::testing::TestInfo& info) override;

    // Every finding so far, one line each ("Suite.Test: + name").
    const std::vector<std::string>& Findings() const { return m_findings; }

private:
    Snapshot m_before;
    std::vector<std::string> m_findings;
};

#endif
