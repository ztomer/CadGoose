#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include <fstream>
#include "config.h"
#include "toml.hpp"
#include "../../src/common/config_helpers.h"
#include "behavior.h"
#include "world.h"

extern int g_screenWidth;
extern int g_screenHeight;

TEST(BehaviorToggles, AllBehaviorTogglesDirectAccess) {
    Config_Init();

    EXPECT_FALSE(g_config.behaviors.fun.ball);
    EXPECT_FALSE(g_config.behaviors.fun.breadCrumbs);
    EXPECT_FALSE(g_config.behaviors.fun.hats);
    EXPECT_FALSE(g_config.behaviors.fun.rainbow);
    EXPECT_FALSE(g_config.behaviors.fun.acid);
    EXPECT_FALSE(g_config.behaviors.fun.anger);
    EXPECT_TRUE(g_config.behaviors.fun.autumnLeaves);
    EXPECT_TRUE(g_config.behaviors.fun.avoidance);
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
    EXPECT_FALSE(g_config.behaviors.fun.interactiveDrops);
    EXPECT_TRUE(g_config.behaviors.fun.avoidance);
    EXPECT_FALSE(g_config.behaviors.fun.boredom);
    EXPECT_TRUE(g_config.behaviors.fun.peeking);
    EXPECT_FALSE(g_config.behaviors.fun.affirmations);
    EXPECT_FALSE(g_config.behaviors.fun.interactiveDrops);
    EXPECT_FALSE(g_config.behaviors.fun.sonicMode);
    EXPECT_TRUE(g_config.behaviors.fun.toysEnabled);

    g_config.behaviors.fun.ball = true;
    EXPECT_TRUE(g_config.behaviors.fun.ball);
    g_config.behaviors.fun.ball = false;

    g_config.behaviors.systems.ai = true;
    EXPECT_TRUE(g_config.behaviors.systems.ai);
    g_config.behaviors.systems.ai = false;

    g_config.behaviors.fun.sonicMode = true;
    EXPECT_TRUE(g_config.behaviors.fun.sonicMode);
    g_config.behaviors.fun.sonicMode = false;

    g_config.behaviors.fun.toysEnabled = false;
    EXPECT_FALSE(g_config.behaviors.fun.toysEnabled);
    g_config.behaviors.fun.toysEnabled = true;
}

TEST(BehaviorToggles, AllRegisteredInRegistry) {
    Config_Init();
    const char* keys[] = {
        "ball_enabled", "breadcrumbs_enabled", "hats_enabled",
        "rainbow_enabled", "acid_enabled", "anger_enabled",
        "autumn_leaves_enabled", "avoidance_enabled", "boredom_enabled",
        "peeking_enabled", "affirmations_enabled", "interactive_drops_enabled",
        "sonic_mode_enabled", "toys_enabled",
        "honcker_enabled", "jail_enabled", "portals_enabled",
        "drag_enabled",
        "nametag_enabled", "presence_enabled", "config_gui_enabled",
        "health_enabled", "ai_enabled", "pomodoro_enabled"
    };
    for (const char* key : keys) {
        const ConfigOption* opt = Config_FindOptionByKey(key);
        ASSERT_NE(opt, nullptr) << "Missing registry entry: " << key;
        EXPECT_EQ(opt->type, CFG_BOOL) << "Wrong type for: " << key;
    }
}

TEST(BehaviorToggles, HotkeysRegisteredInRegistry) {
    Config_Init();
    const char* keys[] = {
        "honcker_hotkey", "jail_hotkey_o", "jail_hotkey_p",
        "portal_hotkey_1", "portal_hotkey_2", "portal_hotkey_0",
        "breadcrumbs_hotkey"
    };
    for (const char* key : keys) {
        const ConfigOption* opt = Config_FindOptionByKey(key);
        ASSERT_NE(opt, nullptr) << "Missing registry entry: " << key;
        EXPECT_EQ(opt->type, CFG_STRING) << "Wrong type for: " << key;
    }
}

TEST(BehaviorToggles, RegistryGetSetBool) {
    Config_Init();
    std::string error;

    bool result = Config_SetValueByKey("ball_enabled", "1", &error);
    EXPECT_TRUE(result) << error;
    EXPECT_TRUE(g_config.behaviors.fun.ball);

    result = Config_SetValueByKey("ball_enabled", "0", &error);
    EXPECT_TRUE(result) << error;
    EXPECT_FALSE(g_config.behaviors.fun.ball);

    std::string value;
    result = Config_GetValueByKey("ball_enabled", &value, &error);
    EXPECT_TRUE(result) << error;
    EXPECT_EQ(value, "0");
}

TEST(BehaviorToggles, LoadFromToml) {
    Config_Init();
    auto savedBall = g_config.behaviors.fun.ball;
    auto savedRainbow = g_config.behaviors.fun.rainbow;
    auto savedAutumn = g_config.behaviors.fun.autumnLeaves;
    auto savedHoncker = g_config.behaviors.control.honcker;
    auto savedPomo = g_config.behaviors.systems.pomodoro;

    auto tbl = toml::parse_str(
        "[Behavior]\n"
        "ball_enabled = true\n"
        "rainbow_enabled = false\n"
        "autumn_leaves_enabled = false\n"
        "honcker_enabled = true\n"
        "pomodoro_enabled = true\n"
    );
    Config_Load(tbl);

    EXPECT_TRUE(g_config.behaviors.fun.ball);
    EXPECT_FALSE(g_config.behaviors.fun.rainbow);
    EXPECT_FALSE(g_config.behaviors.fun.autumnLeaves);
    EXPECT_TRUE(g_config.behaviors.control.honcker);
    EXPECT_TRUE(g_config.behaviors.systems.pomodoro);

    g_config.behaviors.fun.ball = savedBall;
    g_config.behaviors.fun.rainbow = savedRainbow;
    g_config.behaviors.fun.autumnLeaves = savedAutumn;
    g_config.behaviors.control.honcker = savedHoncker;
    g_config.behaviors.systems.pomodoro = savedPomo;
}

TEST(BehaviorToggles, SaveRoundTrip) {
    Config_Init();
    auto savedBall = g_config.behaviors.fun.ball;
    auto savedAutumn = g_config.behaviors.fun.autumnLeaves;
    auto savedHoncker = g_config.behaviors.control.honcker;

    Config_SetValueByKey("ball_enabled", "1");
    Config_SetValueByKey("autumn_leaves_enabled", "0");
    Config_SetValueByKey("honcker_enabled", "1");
    Config_SaveAll();

    g_config.behaviors.fun.ball = false;
    g_config.behaviors.fun.autumnLeaves = true;
    g_config.behaviors.control.honcker = false;

    auto tbl = toml::parse(Config_GetPath());
    Config_Load(tbl);

    EXPECT_TRUE(g_config.behaviors.fun.ball);
    EXPECT_FALSE(g_config.behaviors.fun.autumnLeaves);
    EXPECT_TRUE(g_config.behaviors.control.honcker);

    g_config.behaviors.fun.ball = savedBall;
    g_config.behaviors.fun.autumnLeaves = savedAutumn;
    g_config.behaviors.control.honcker = savedHoncker;
    Config_SaveAll();
}

TEST(ConfigThemes, LoadThemeColors) {
    Config_Init();
    std::filesystem::path themeDir = Config_GetThemesDir();
    std::filesystem::path themeFile = themeDir / "testtheme.toml";

    std::ofstream out(themeFile);
    out << "[theme]\nname = \"TestTheme\"\n\n[colors]\n"
        << "body = { r = 0.5, g = 0.5, b = 0.5 }\n"
        << "neck = { r = 0.5, g = 0.5, b = 0.5 }\n"
        << "head = { r = 0.5, g = 0.5, b = 0.5 }\n"
        << "beak = { r = 0.1, g = 0.2, b = 0.3 }\n"
        << "eye = { r = 0.1, g = 0.1, b = 0.1 }\n"
        << "outline = { r = 0.2, g = 0.2, b = 0.2 }\n";
    out.close();

    ColorRGB body, neck, head, beak, eye, outline;
    bool success = Config_LoadThemeColors("TestTheme", body, neck, head, beak, eye, outline);
    EXPECT_TRUE(success);
    EXPECT_FLOAT_EQ(body.r, 0.5f);
    EXPECT_FLOAT_EQ(beak.g, 0.2f);

    std::filesystem::remove(themeFile);
}

TEST(ConfigThemes, UpdateActiveTheme) {
    Config_Init();
    g_config.general.appearanceMode = 3;
    g_config.color.customBody = { 1.0f, 0.0f, 0.0f };

    Config_UpdateActiveTheme();

    EXPECT_FLOAT_EQ(g_config.color.currentBody.r, 1.0f);
    EXPECT_FLOAT_EQ(g_config.color.currentBody.g, 0.0f);
}

TEST(BehaviorToggles, SonicModeBehaviorRegistered) {
    Config_Init();
    Behavior* sonic = BehaviorRegistry::Instance().Get("sonic");
    ASSERT_NE(sonic, nullptr);
    EXPECT_STREQ(sonic->id, "sonic");
    EXPECT_EQ(sonic->configPtr, &g_config.behaviors.fun.sonicMode);
}

TEST(BehaviorToggles, ToysBehaviorRegistered) {
    Config_Init();
    Behavior* toys = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(toys, nullptr);
    EXPECT_STREQ(toys->id, "toys");
    EXPECT_EQ(toys->configPtr, &g_config.behaviors.fun.toysEnabled);
}

TEST(BehaviorToggles, SonicStateManagement) {
    Config_Init();
    BehaviorStateManager::Instance().ClearAll();

    int testGooseId = 999;
    auto* state = BehaviorStateManager::Instance().GetOrCreate<SonicState>(testGooseId, "sonic");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->trails.empty());
    EXPECT_DOUBLE_EQ(state->lastTrailTime, 0);
    EXPECT_DOUBLE_EQ(state->lastHonkTime, 0);

    state->trails.push_back(SonicTrail{{100, 200}, 1.0});
    EXPECT_EQ(state->trails.size(), 1u);

    state->Reset();
    EXPECT_TRUE(state->trails.empty());

    BehaviorStateManager::Instance().ClearAll();
}

TEST(BehaviorToggles, ToysStateManagement) {
    Config_Init();
    BehaviorStateManager::Instance().ClearAll();

    int testGooseId = 998;
    auto* state = BehaviorStateManager::Instance().GetOrCreate<ToysState>(testGooseId, "toys");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->activeCount, 0);
    EXPECT_DOUBLE_EQ(state->lastSpawnTime, 0);

    state->toys[0].active = true;
    state->toys[0].pos = {150, 250};
    state->toys[0].type = Toy::Type::Stick;
    state->activeCount = 1;

    EXPECT_TRUE(state->toys[0].active);
    EXPECT_FLOAT_EQ(state->toys[0].pos.x, 150);

    state->Reset();
    EXPECT_FALSE(state->toys[0].active);
    EXPECT_EQ(state->activeCount, 0);

    BehaviorStateManager::Instance().ClearAll();
}

TEST(BehaviorToggles, SonicModeConfigToggle) {
    Config_Init();
    EXPECT_FALSE(g_config.behaviors.fun.sonicMode);

    g_config.behaviors.fun.sonicMode = true;
    EXPECT_TRUE(g_config.behaviors.fun.sonicMode);

    g_config.behaviors.fun.sonicMode = false;
    EXPECT_FALSE(g_config.behaviors.fun.sonicMode);
}

TEST(BehaviorToggles, ToysEnabledConfigToggle) {
    Config_Init();
    EXPECT_TRUE(g_config.behaviors.fun.toysEnabled);

    g_config.behaviors.fun.toysEnabled = false;
    EXPECT_FALSE(g_config.behaviors.fun.toysEnabled);

    g_config.behaviors.fun.toysEnabled = true;
    EXPECT_TRUE(g_config.behaviors.fun.toysEnabled);
}

TEST(BehaviorToggles, SonicModeLoadFromToml) {
    Config_Init();

    auto tbl = toml::parse_str(
        "[Behavior]\n"
        "sonic_mode_enabled = true\n"
    );
    Config_Load(tbl);

    EXPECT_TRUE(g_config.behaviors.fun.sonicMode);

    g_config.behaviors.fun.sonicMode = false;
}

TEST(BehaviorToggles, ToysEnabledLoadFromToml) {
    Config_Init();

    auto tbl = toml::parse_str(
        "[Behavior]\n"
        "toys_enabled = true\n"
    );
    Config_Load(tbl);

    EXPECT_TRUE(g_config.behaviors.fun.toysEnabled);

    g_config.behaviors.fun.toysEnabled = false;
}
