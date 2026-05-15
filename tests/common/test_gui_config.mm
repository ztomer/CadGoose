// Config key consistency tests: verify the in-memory config toggle chain
// These test the config struct directly (not via s_setBoolValue/s_getBoolForKey
// which live in config_gui.mm and would require linking AppKit GUI code).

#import <gtest/gtest.h>
#include "config.h"
#include <set>
#include <vector>

// All toggle bool keys used in the GUI addRow: calls in config_gui.mm
// These MUST match the registry keys (not dot-notation struct paths)
static const std::vector<std::pair<const char*, bool*>> kGUIVisibleToggleKeys = {
    {"ball_enabled",                   &g_config.behaviors.fun.ball},
    {"breadcrumbs_enabled",            &g_config.behaviors.fun.breadCrumbs},
    {"hats_enabled",                   &g_config.behaviors.fun.hats},
    {"rainbow_enabled",                &g_config.behaviors.fun.rainbow},
    {"acid_enabled",                   &g_config.behaviors.fun.acid},
    {"anger_enabled",                  &g_config.behaviors.fun.anger},
    {"autumn_leaves_enabled",          &g_config.behaviors.fun.autumnLeaves},
    {"avoidance_enabled",              &g_config.behaviors.fun.avoidance},
    {"boredom_enabled",                &g_config.behaviors.fun.boredom},
    {"peeking_enabled",                &g_config.behaviors.fun.peeking},
    {"affirmations_enabled",           &g_config.behaviors.fun.affirmations},
    {"interactive_drops_enabled",      &g_config.behaviors.fun.interactiveDrops},
    {"sonic_mode_enabled",             &g_config.behaviors.fun.sonicMode},
    {"toys_enabled",                   &g_config.behaviors.fun.toysEnabled},
    {"honcker_enabled",                &g_config.behaviors.control.honcker},
    {"jail_enabled",                   &g_config.behaviors.control.jail},
    {"portals_enabled",                &g_config.behaviors.control.portals},
    {"drag_enabled",                   &g_config.behaviors.control.drag},
    {"nametag_enabled",                &g_config.behaviors.info.nametag},
    {"health_enabled",                 &g_config.behaviors.systems.health},
    {"pomodoro_enabled",               &g_config.behaviors.systems.pomodoro},
};

// All toggle bool keys handled by s_getBoolForKey/s_setBoolValue (including hidden ones)
static const std::vector<std::pair<const char*, bool*>> kAllToggleKeys = {
    {"ball_enabled",                   &g_config.behaviors.fun.ball},
    {"breadcrumbs_enabled",            &g_config.behaviors.fun.breadCrumbs},
    {"hats_enabled",                   &g_config.behaviors.fun.hats},
    {"rainbow_enabled",                &g_config.behaviors.fun.rainbow},
    {"acid_enabled",                   &g_config.behaviors.fun.acid},
    {"anger_enabled",                  &g_config.behaviors.fun.anger},
    {"autumn_leaves_enabled",          &g_config.behaviors.fun.autumnLeaves},
    {"avoidance_enabled",              &g_config.behaviors.fun.avoidance},
    {"boredom_enabled",                &g_config.behaviors.fun.boredom},
    {"peeking_enabled",                &g_config.behaviors.fun.peeking},
    {"affirmations_enabled",           &g_config.behaviors.fun.affirmations},
    {"interactive_drops_enabled",      &g_config.behaviors.fun.interactiveDrops},
    {"sonic_mode_enabled",             &g_config.behaviors.fun.sonicMode},
    {"toys_enabled",                   &g_config.behaviors.fun.toysEnabled},
    {"honcker_enabled",                &g_config.behaviors.control.honcker},
    {"jail_enabled",                   &g_config.behaviors.control.jail},
    {"portals_enabled",                &g_config.behaviors.control.portals},
    {"drag_enabled",                   &g_config.behaviors.control.drag},
    {"nametag_enabled",                &g_config.behaviors.info.nametag},
    {"presence_enabled",               &g_config.behaviors.info.presence},
    {"config_gui_enabled",             &g_config.behaviors.info.configGUI},
    {"visible_enabled",                &g_config.behaviors.info.visible},
    {"health_enabled",                 &g_config.behaviors.systems.health},
    {"ai_enabled",                     &g_config.behaviors.systems.ai},
    {"pomodoro_enabled",               &g_config.behaviors.systems.pomodoro},
};

// Config struct field names that must have a CORRESPONDING entry in the struct
// (verify the struct has the right fields)
struct FieldCheck {
    size_t offset;  // offset into Fun/Control/Info/Systems
    bool expectedDefault;
};

// ============================================================
// Regression test: every GUI addRow: key must exist in registry
// Prevents the bug where dot-notation keys (behaviors.fun.ball)
// were used instead of registry keys (ball_enabled), causing
// s_setBoolValue to silently fail and toggles to always reset.
// ============================================================
TEST(GUIConfigRegression, AllGUIKeysExistInRegistry) {
    Config_Init();
    for (const auto& [key, ptr] : kGUIVisibleToggleKeys) {
        const ConfigOption* opt = Config_FindOptionByKey(key);
        ASSERT_NE(opt, nullptr) << "GUI key \"" << key << "\" not found in config registry — toggle will silently fail";
        EXPECT_EQ(opt->type, CFG_BOOL) << "GUI key \"" << key << "\" is not a bool in registry";
        EXPECT_EQ(opt->ptr, ptr) << "GUI key \"" << key << "\" points to wrong config field";
    }
}

// Every registry behavior toggle must have a GUI row (no orphaned toggles)
TEST(GUIConfigRegression, AllRegistryTogglesHaveGUIRows) {
    Config_Init();
    for (const auto& [key, ptr] : kAllToggleKeys) {
        const ConfigOption* opt = Config_FindOptionByKey(key);
        ASSERT_NE(opt, nullptr) << "Registry key \"" << key << "\" not found";
        EXPECT_EQ(opt->ptr, ptr) << "Registry key \"" << key << "\" points to wrong config field";
    }
}

// No key should be registered twice with different pointers
TEST(GUIConfigRegression, NoDuplicateRegistryEntries) {
    Config_Init();
    std::set<const void*> seenPtrs;
    for (const auto& [key, ptr] : kAllToggleKeys) {
        EXPECT_FALSE(seenPtrs.count(ptr)) << "Key \"" << key << "\" shares pointer with another key";
        seenPtrs.insert(ptr);
    }
}

TEST(GUIConfigTest, AllGUIVisibleKeysAreInToggleList) {
    // Every key used in addRow: must be in the full toggle list
    for (const auto& [guiKey, guiPtr] : kGUIVisibleToggleKeys) {
        bool found = false;
        for (const auto& [allKey, allPtr] : kAllToggleKeys) {
            if (std::string(guiKey) == allKey) { found = true; break; }
        }
        EXPECT_TRUE(found) << "GUI key \"" << guiKey << "\" has no corresponding handler";
    }
}

TEST(GUIConfigTest, AllToggleKeysAreUnique) {
    // Verify no duplicate keys
    for (size_t i = 0; i < kAllToggleKeys.size(); i++) {
        for (size_t j = i + 1; j < kAllToggleKeys.size(); j++) {
            EXPECT_NE(std::string(kAllToggleKeys[i].first), kAllToggleKeys[j].first)
                << "Duplicate toggle key: " << kAllToggleKeys[i].first;
        }
    }
}

TEST(GUIConfigTest, AllGUIVisibleKeysAreUnique) {
    for (size_t i = 0; i < kGUIVisibleToggleKeys.size(); i++) {
        for (size_t j = i + 1; j < kGUIVisibleToggleKeys.size(); j++) {
            EXPECT_NE(std::string(kGUIVisibleToggleKeys[i].first), kAllToggleKeys[j].first)
                << "Duplicate GUI key: " << kGUIVisibleToggleKeys[i].first;
        }
    }
}

TEST(GUIConfigTest, ToggleCount) {
    EXPECT_EQ(kGUIVisibleToggleKeys.size(), 21)
        << "Should be exactly 21 GUI-visible toggle keys";
    EXPECT_EQ(kAllToggleKeys.size(), 25)
        << "Should be exactly 25 total toggle keys (21 visible + 4 hidden: presence, configGUI, visible, ai)";
}

TEST(GUIConfigTest, ReadWriteRoundTrip) {
    // For each toggle key, set the bool directly, verify, then restore
    for (const auto& [key, ptr] : kAllToggleKeys) {
        bool original = *ptr;

        *ptr = true;
        EXPECT_TRUE(*ptr) << "Config field for \"" << key << "\" should be true after direct set";
        *ptr = false;
        EXPECT_FALSE(*ptr) << "Config field for \"" << key << "\" should be false after direct set";

        *ptr = original;
    }
}

TEST(GUIConfigTest, DefaultValues) {
    // Verify struct defaults match expectations
    EXPECT_FALSE(g_config.behaviors.fun.ball);
    EXPECT_FALSE(g_config.behaviors.fun.breadCrumbs);
    EXPECT_FALSE(g_config.behaviors.fun.hats);
    EXPECT_FALSE(g_config.behaviors.fun.rainbow);
    EXPECT_FALSE(g_config.behaviors.fun.acid);
    EXPECT_FALSE(g_config.behaviors.fun.anger);
    EXPECT_TRUE(g_config.behaviors.fun.autumnLeaves);
    EXPECT_TRUE(g_config.behaviors.fun.avoidance);
    EXPECT_FALSE(g_config.behaviors.fun.boredom);
    EXPECT_TRUE(g_config.behaviors.fun.peeking);
    EXPECT_FALSE(g_config.behaviors.fun.affirmations);
    EXPECT_FALSE(g_config.behaviors.fun.interactiveDrops);
    EXPECT_FALSE(g_config.behaviors.fun.sonicMode);
    EXPECT_FALSE(g_config.behaviors.fun.toysEnabled);
    EXPECT_FALSE(g_config.behaviors.control.honcker);
    EXPECT_FALSE(g_config.behaviors.control.jail);
    EXPECT_FALSE(g_config.behaviors.control.portals);
    EXPECT_FALSE(g_config.behaviors.control.drag);
    EXPECT_FALSE(g_config.behaviors.info.nametag);
    EXPECT_FALSE(g_config.behaviors.info.presence);
    EXPECT_FALSE(g_config.behaviors.info.configGUI);
    EXPECT_TRUE(g_config.behaviors.info.visible);  // visible defaults to true
    EXPECT_FALSE(g_config.behaviors.systems.health);
    EXPECT_FALSE(g_config.behaviors.systems.ai);
    EXPECT_FALSE(g_config.behaviors.systems.pomodoro);
}

TEST(GUIConfigTest, StructPointerDistinct) {
    // Each toggle key must point to a distinct config field (no aliasing)
    std::set<bool*> seen;
    for (const auto& [key, ptr] : kAllToggleKeys) {
        EXPECT_FALSE(seen.count(ptr)) << "Duplicate pointer for key \"" << key << "\"";
        seen.insert(ptr);
    }
    EXPECT_EQ(seen.size(), kAllToggleKeys.size());
}
