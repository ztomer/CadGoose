// ThreadSanitizer proof for ROADMAP M1.2: Config_SaveAll (main thread) racing
// the MCP server thread's writers must be data-race free. This binary is built
// with -fsanitize=thread (target CadGooseTsanTests); TSan reports any
// unsynchronized access as an error and halt_on_error makes it a red exit.
#include <gtest/gtest.h>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include "config.h"
#include "config_string.h"
#include "hotkey_cache.h"

namespace fs = std::filesystem;

namespace {

// Long enough that every assignment reallocates (no small-string buffer), so
// an unsynchronized read sees a freed buffer, not just a torn short string.
const std::string kLongA(64, 'a');
const std::string kLongB(96, 'b');

class ConfigSaveRace : public ::testing::Test {
protected:
    void SetUp() override {
        Config_Init();
        char dirTemplate[] = "/tmp/cadgoose_tsan_XXXXXX";
        char* dirResult = mkdtemp(dirTemplate);
        ASSERT_NE(dirResult, nullptr);
        m_dir = dirResult;
        m_override.emplace(m_dir);
    }
    void TearDown() override {
        m_override.reset();
        std::error_code ec;
        fs::remove_all(m_dir, ec);
    }
    fs::path m_dir;
    std::optional<ScopedConfigDirOverride> m_override;
};

// Run `writer` on a second thread (the MCP server's role) while this thread
// saves, then return; TSan judges the interleaving.
template <typename Writer>
void RaceSaveAgainst(Writer writer) {
    std::atomic<bool> stop{false};
    std::thread mcp([&] {
        for (int i = 0; !stop.load(std::memory_order_relaxed); ++i) writer(i);
    });
    for (int i = 0; i < 200; ++i) Config_SaveAll();
    stop.store(true);
    mcp.join();
}

}  // namespace

TEST_F(ConfigSaveRace, SaveAgainstSetHotkey) {
    RaceSaveAgainst([](int i) {
        Hotkey_SetName(HotkeyId::Portal1, (i & 1) ? kLongA : kLongB);
    });
    std::string name = Hotkey_Name(HotkeyId::Portal1);
    EXPECT_TRUE(name == kLongA || name == kLongB);
}

// Any registry string writer (socket `set`, a reload), not only hotkeys.
TEST_F(ConfigSaveRace, SaveAgainstRegistryStringWrite) {
    const ConfigOption* opt = Config_FindOptionByKey("custom_model");
    ASSERT_NE(opt, nullptr);
    RaceSaveAgainst([opt](int i) { ConfigOption_SetString(*opt, (i & 1) ? kLongA : kLongB); });
}

TEST_F(ConfigSaveRace, GetAgainstSetHotkeyThroughRegistry) {
    std::atomic<bool> stop{false};
    std::thread mcp([&] {
        for (int i = 0; !stop.load(std::memory_order_relaxed); ++i)
            Hotkey_SetName(HotkeyId::HonckerHonk, (i & 1) ? kLongA : kLongB);
    });
    std::string value;
    for (int i = 0; i < 2000; ++i) Config_GetValueByKey("honcker_hotkey", &value, nullptr);
    stop.store(true);
    mcp.join();
    EXPECT_TRUE(value == kLongA || value == kLongB);
}
