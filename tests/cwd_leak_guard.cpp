#include "cwd_leak_guard.h"

#include <chrono>
#include <system_error>

namespace fs = std::filesystem;

namespace {

std::string Fingerprint(const fs::directory_entry& e) {
    std::error_code ec;
    std::string fp = e.is_directory(ec) ? "d" : "f";
    if (!e.is_directory(ec)) {
        fp += ":" + std::to_string(e.file_size(ec));
    }
    const auto mtime = e.last_write_time(ec);
    fp += ":" + std::to_string(static_cast<long long>(mtime.time_since_epoch().count()));
    return fp;
}

}  // namespace

CwdLeakGuard::Snapshot CwdLeakGuard::Take(const fs::path& root) {
    Snapshot snap;
    std::error_code ec;
    // Top level only: a leak through a relative path creates an entry here.
    // Directory mtimes are deliberately left out at this level; build/ churns
    // as a matter of course, and a NEW name is the leak signature.
    for (const auto& e : fs::directory_iterator(root, ec)) {
        snap[e.path().filename().string()] = e.is_directory(ec) ? "d" : "f";
    }
    // <cwd>/config/ in full: ConfigDirPath() resolves there whenever the
    // CADGOOSE_CONFIG_DIR sandbox is lost, so a write into it is a leak even
    // when it creates no new top-level name.
    const fs::path config = root / "config";
    if (fs::is_directory(config, ec)) {
        for (auto it = fs::recursive_directory_iterator(config, ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            snap["config/" + it->path().lexically_relative(config).string()] = Fingerprint(*it);
        }
    }
    return snap;
}

std::vector<std::string> CwdLeakGuard::Diff(const Snapshot& before, const Snapshot& after) {
    std::vector<std::string> out;
    for (const auto& [name, fp] : after) {
        auto it = before.find(name);
        if (it == before.end()) {
            out.push_back("+ " + name);
        } else if (it->second != fp) {
            out.push_back("~ " + name);
        }
    }
    for (const auto& [name, fp] : before) {
        if (after.find(name) == after.end()) out.push_back("- " + name);
    }
    return out;
}

void CwdLeakGuard::OnTestStart(const ::testing::TestInfo&) {
    std::error_code ec;
    m_before = Take(fs::current_path(ec));
}

void CwdLeakGuard::OnTestEnd(const ::testing::TestInfo& info) {
    std::error_code ec;
    const Snapshot after = Take(fs::current_path(ec));
    for (const auto& line : Diff(m_before, after)) {
        m_findings.push_back(std::string(info.test_suite_name()) + "." + info.name() + ": " + line);
    }
}
