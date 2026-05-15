// Config key consistency tests: verify the in-memory config toggle chain
// These test the config struct directly (not via s_setBoolValue/s_getBoolForKey
// which live in config_gui.mm and would require linking AppKit GUI code).

#import <gtest/gtest.h>
#include "config.h"

// All toggle bool keys used in the GUI (matched to addRow: calls in config_gui.mm)
// These are the "visible" toggle keys — the ones the user sees in the preferences window.
static const std::vector<std::pair<const char*, bool*>> kGUIVisibleToggleKeys = {
    {"behaviors.fun.ball",             &g_config.behaviors.fun.ball},
    {"behaviors.fun.breadCrumbs",      &g_config.behaviors.fun.breadCrumbs},
    {"behaviors.fun.hats",             &g_config.behaviors.fun.hats},
    {"behaviors.fun.rainbow",          &g_config.behaviors.fun.rainbow},
    {"behaviors.fun.acid",              &g_config.behaviors.fun.acid},
    {"behaviors.fun.anger",            &g_config.behaviors.fun.anger},
    {"behaviors.fun.autumnLeaves",     &g_config.behaviors.fun.autumnLeaves},
    {"behaviors.control.honcker",      &g_config.behaviors.control.honcker},
    {"behaviors.control.jail",         &g_config.behaviors.control.jail},
    {"behaviors.control.portals",      &g_config.behaviors.control.portals},
    {"behaviors.control.drag",         &g_config.behaviors.control.drag},
    {"behaviors.info.nametag",         &g_config.behaviors.info.nametag},
    {"behaviors.systems.health",       &g_config.behaviors.systems.health},
    {"behaviors.systems.pomodoro",     &g_config.behaviors.systems.pomodoro},
};

// All toggle bool keys handled by s_getBoolForKey/s_setBoolValue (including hidden ones)
static const std::vector<std::pair<const char*, bool*>> kAllToggleKeys = {
    {"behaviors.fun.ball",             &g_config.behaviors.fun.ball},
    {"behaviors.fun.breadCrumbs",      &g_config.behaviors.fun.breadCrumbs},
    {"behaviors.fun.hats",             &g_config.behaviors.fun.hats},
    {"behaviors.fun.rainbow",          &g_config.behaviors.fun.rainbow},
    {"behaviors.fun.acid",              &g_config.behaviors.fun.acid},
    {"behaviors.fun.anger",            &g_config.behaviors.fun.anger},
    {"behaviors.fun.autumnLeaves",     &g_config.behaviors.fun.autumnLeaves},
    {"behaviors.control.honcker",      &g_config.behaviors.control.honcker},
    {"behaviors.control.jail",         &g_config.behaviors.control.jail},
    {"behaviors.control.portals",      &g_config.behaviors.control.portals},
    {"behaviors.control.drag",         &g_config.behaviors.control.drag},
    {"behaviors.info.nametag",         &g_config.behaviors.info.nametag},
    {"behaviors.info.presence",        &g_config.behaviors.info.presence},
    {"behaviors.info.configGUI",       &g_config.behaviors.info.configGUI},
    {"behaviors.systems.health",       &g_config.behaviors.systems.health},
    {"behaviors.systems.ai",           &g_config.behaviors.systems.ai},
    {"behaviors.systems.pomodoro",     &g_config.behaviors.systems.pomodoro},
};

// Config struct field names that must have a CORRESPONDING entry in the struct
// (verify the struct has the right fields)
struct FieldCheck {
    size_t offset;  // offset into Fun/Control/Info/Systems
    bool expectedDefault;
};

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
            EXPECT_NE(std::string(kGUIVisibleToggleKeys[i].first), kGUIVisibleToggleKeys[j].first)
                << "Duplicate GUI key: " << kGUIVisibleToggleKeys[i].first;
        }
    }
}

TEST(GUIConfigTest, ToggleCount) {
    EXPECT_EQ(kGUIVisibleToggleKeys.size(), 14)
        << "Should be exactly 14 GUI-visible toggle keys";
    EXPECT_EQ(kAllToggleKeys.size(), 17)
        << "Should be exactly 17 total toggle keys (14 visible + 3 hidden: presence, configGUI, ai)";
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
    EXPECT_FALSE(g_config.behaviors.control.honcker);
    EXPECT_FALSE(g_config.behaviors.control.jail);
    EXPECT_FALSE(g_config.behaviors.control.portals);
    EXPECT_FALSE(g_config.behaviors.control.drag);
    EXPECT_FALSE(g_config.behaviors.info.nametag);
    EXPECT_FALSE(g_config.behaviors.info.presence);
    EXPECT_FALSE(g_config.behaviors.info.configGUI);
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
