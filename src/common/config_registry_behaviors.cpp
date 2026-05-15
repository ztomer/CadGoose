// config_registry_behaviors.cpp
// Behavior toggle and hotkey registration
#include "config.h"

void RegisterBehaviors(std::vector<ConfigOption>& r) {
    // Behavior toggle bools
    r.push_back(CONFIG_BOOL("Behavior", "ball_enabled", "Ball Enabled",
        &g_config.behaviors.fun.ball, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "breadcrumbs_enabled", "Breadcrumbs Enabled",
        &g_config.behaviors.fun.breadCrumbs, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "hats_enabled", "Hats Enabled",
        &g_config.behaviors.fun.hats, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "rainbow_enabled", "Rainbow Enabled",
        &g_config.behaviors.fun.rainbow, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "acid_enabled", "Acid Enabled",
        &g_config.behaviors.fun.acid, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "anger_enabled", "Anger Enabled",
        &g_config.behaviors.fun.anger, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "autumn_leaves_enabled", "Autumn Leaves Enabled",
        &g_config.behaviors.fun.autumnLeaves, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "avoidance_enabled", "Cursor Avoidance Enabled",
        &g_config.behaviors.fun.avoidance, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "boredom_enabled", "Boredom Sigh Enabled",
        &g_config.behaviors.fun.boredom, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "peeking_enabled", "Window Peeking Enabled",
        &g_config.behaviors.fun.peeking, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "affirmations_enabled", "Custom Affirmations Enabled",
        &g_config.behaviors.fun.affirmations, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "interactive_drops_enabled", "Interactive Drops Enabled",
        &g_config.behaviors.fun.interactiveDrops, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "sonic_mode_enabled", "Sonic Mode Enabled",
        &g_config.behaviors.fun.sonicMode, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "toys_enabled", "Toys Enabled",
        &g_config.behaviors.fun.toysEnabled, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "honcker_enabled", "Honcker Enabled",
        &g_config.behaviors.control.honcker, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "jail_enabled", "Jail Enabled",
        &g_config.behaviors.control.jail, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "portals_enabled", "Portals Enabled",
        &g_config.behaviors.control.portals, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "drag_enabled", "Drag Enabled",
        &g_config.behaviors.control.drag, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "nametag_enabled", "Nametag Enabled",
        &g_config.behaviors.info.nametag, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "presence_enabled", "Presence Enabled",
        &g_config.behaviors.info.presence, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "config_gui_enabled", "Config GUI Enabled",
        &g_config.behaviors.info.configGUI, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "visible_enabled", "Visible Enabled",
        &g_config.behaviors.info.visible, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "health_enabled", "Health Enabled",
        &g_config.behaviors.systems.health, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "ai_enabled", "AI Enabled",
        &g_config.behaviors.systems.ai, OnConfigChange));
    r.push_back(CONFIG_BOOL("Behavior", "pomodoro_enabled", "Pomodoro Enabled",
        &g_config.behaviors.systems.pomodoro, OnConfigChange));
    // Behavior hotkey strings
    r.push_back(CONFIG_STRING("Behavior", "honcker_hotkey", "Honcker Hotkey",
        &g_config.behaviors.honcker.hotkey, OnConfigChange));
    r.push_back(CONFIG_STRING("Behavior", "jail_hotkey_o", "Jail Set Hotkey",
        &g_config.behaviors.jail.hotkeyO, OnConfigChange));
    r.push_back(CONFIG_STRING("Behavior", "jail_hotkey_p", "Jail Toggle Hotkey",
        &g_config.behaviors.jail.hotkeyP, OnConfigChange));
    r.push_back(CONFIG_STRING("Behavior", "portal_hotkey_1", "Portal 1 Hotkey",
        &g_config.portal.hotkey1, OnConfigChange));
    r.push_back(CONFIG_STRING("Behavior", "portal_hotkey_2", "Portal 2 Hotkey",
        &g_config.portal.hotkey2, OnConfigChange));
    r.push_back(CONFIG_STRING("Behavior", "portal_hotkey_0", "Portal Toggle Hotkey",
        &g_config.portal.hotkey0, OnConfigChange));
    r.push_back(CONFIG_STRING("Behavior", "breadcrumbs_hotkey", "Breadcrumbs Hotkey",
        &g_config.behaviors.breadCrumbs.hotkey, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Behavior", "affirmations_interval", "Affirmations Interval",
        &g_config.behaviors.affirmations.interval, 30.0f, 3600.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_STRING("Behavior", "affirmations_custom_message", "Custom Affirmation Message",
        &g_config.behaviors.affirmations.customMessage, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Behavior", "interactive_drops_puddle_lifetime", "Puddle Lifetime",
        &g_config.behaviors.interactiveDrops.puddleLifetime, 5.0f, 120.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Behavior", "interactive_drops_flower_grow_time", "Flower Grow Time",
        &g_config.behaviors.interactiveDrops.flowerGrowTime, 1.0f, 60.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Behavior", "interactive_drops_drop_interval", "Interactive Drop Interval",
        &g_config.behaviors.interactiveDrops.dropInterval, 10.0f, 600.0f, 0.5f, OnConfigChange));
}
