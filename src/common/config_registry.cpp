#include "config.h"

namespace {

void RegisterCommon(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_BOOL("Debug", "to_terminal", "To Terminal",
        &g_config.debug.toTerminal, OnConfigChange));
    r.push_back(CONFIG_BOOL("Debug", "visuals", "Visuals",
        &g_config.debug.visuals, OnConfigChange));
    r.push_back(CONFIG_FLOAT_EX("General", "global_scale", "Global Scale",
        "Multiplier for goose size and speeds (0.1 - 10.0)",
        &g_config.general.globalScale, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_BOOL_EX("General", "audio_enabled", "Audio Enabled",
        "Play honk sounds when goose moves",
        &g_config.general.audioEnabled, OnConfigChange));
    r.push_back(CONFIG_BOOL_EX("General", "memes_enabled", "Memes Enabled",
        "Allow picking up images and text as items",
        &g_config.general.memesEnabled, OnConfigChange));
    r.push_back(CONFIG_INT_EX("General", "appearance_mode", "Appearance Mode",
        "0=Light, 1=Dark, 2=System (follow macOS), 3=Custom",
        &g_config.general.appearanceMode, 0, 3, OnConfigChange));
    r.push_back(CONFIG_STRING("General", "light_theme_role", "Light Theme Role",
        &g_config.general.lightThemeRole, OnConfigChange));
    r.push_back(CONFIG_STRING("General", "dark_theme_role", "Dark Theme Role",
        &g_config.general.darkThemeRole, OnConfigChange));
    r.push_back(CONFIG_INT("Screen", "default_width", "Default Width",
        &g_config.screen.defaultWidth, 0, 10000, OnConfigChange));
    r.push_back(CONFIG_INT("Screen", "default_height", "Default Height",
        &g_config.screen.defaultHeight, 0, 10000, OnConfigChange));
    r.push_back(CONFIG_INT("Asset", "meme_placeholder_w", "Meme Placeholder W",
        &g_config.asset.memePlaceholderW, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Asset", "meme_placeholder_h", "Meme Placeholder H",
        &g_config.asset.memePlaceholderH, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Asset", "text_placeholder_w", "Text Placeholder W",
        &g_config.asset.textPlaceholderW, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Asset", "text_placeholder_h", "Text Placeholder H",
        &g_config.asset.textPlaceholderH, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Asset", "note_placeholder_w", "Note Placeholder W",
        &g_config.asset.notePlaceholderW, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Asset", "note_placeholder_h", "Note Placeholder H",
        &g_config.asset.notePlaceholderH, 0, 1000, OnConfigChange));
}

void RegisterCursor(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_BOOL("Cursor", "chase_enabled", "Chase Enabled",
        &g_config.cursor.chaseEnabled, OnConfigChange));
    r.push_back(CONFIG_INT("Cursor", "chase_chance", "Chase Chance",
        &g_config.cursor.chaseChance, 0, 100, OnConfigChange));
    r.push_back(CONFIG_BOOL("Cursor", "multi_monitor_enabled", "Multi Monitor Enabled",
        &g_config.cursor.multiMonitorEnabled, OnConfigChange));
}

void RegisterMovement(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Movement", "base_walk_speed", "Base Walk Speed",
        &g_config.movement.baseWalkSpeed, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "base_run_speed", "Base Run Speed",
        &g_config.movement.baseRunSpeed, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "max_force", "Max Force",
        &g_config.movement.maxForce, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "move_distance_threshold", "Move Distance Threshold",
        &g_config.movement.moveDistanceThreshold, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "friction", "Friction",
        &g_config.movement.friction, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "max_speed", "Max Speed",
        &g_config.movement.maxSpeed, 0.0f, 1000.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "run_speed_multiplier", "Run Speed Multiplier",
        &g_config.movement.runSpeedMultiplier, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "init_direction_max", "Init Direction Max",
        &g_config.movement.initDirectionMax, 0.0f, 360.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "speed_lerp_rate", "Speed Lerp Rate",
        &g_config.movement.speedLerpRate, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "arrival_radius", "Arrival Radius",
        &g_config.movement.arrivalRadius, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "run_distance_threshold", "Run Distance Threshold",
        &g_config.movement.runDistanceThreshold, 0.0f, 2000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Movement", "direction_blend_rate", "Direction Blend Rate",
        &g_config.movement.directionBlendRate, 0.0f, 1.0f, 0.01f, OnConfigChange));
}

void RegisterPhysics(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Physics", "screen_margin", "Screen Margin",
        &g_config.physics.screenMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "bounce_factor_wall", "Bounce Factor Wall",
        &g_config.physics.bounceFactorWall, 0.0f, 2.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "bounce_factor_corner", "Bounce Factor Corner",
        &g_config.physics.bounceFactorCorner, 0.0f, 2.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "corner_margin", "Corner Margin",
        &g_config.physics.cornerMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "drag_min_dt", "Drag Min Dt",
        &g_config.physics.dragMinDt, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "drag_velocity_threshold", "Drag Velocity Threshold",
        &g_config.physics.dragVelocityThreshold, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "drag_rotation_speed", "Drag Rotation Speed",
        &g_config.physics.dragRotationSpeed, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "iso_scale_x", "Iso Scale X",
        &g_config.physics.isoScaleX, 0.1f, 2.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "iso_scale_y", "Iso Scale Y",
        &g_config.physics.isoScaleY, 0.1f, 2.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "edge_avoid_margin", "Edge Avoid Margin",
        &g_config.physics.edgeAvoidMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "edge_look_ahead_speed", "Edge Look Ahead Speed",
        &g_config.physics.edgeLookAheadSpeed, 0.0f, 100.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "edge_look_ahead_base", "Edge Look Ahead Base",
        &g_config.physics.edgeLookAheadBase, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "edge_avoid_force", "Edge Avoid Force",
        &g_config.physics.edgeAvoidForce, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "screen_clamp_tight", "Screen Clamp Tight",
        &g_config.physics.screenClampTight, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "screen_clamp_expanded", "Screen Clamp Expanded",
        &g_config.physics.screenClampExpanded, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "screen_clamp_bounce", "Screen Clamp Bounce",
        &g_config.physics.screenClampBounce, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "steer_seek_force", "Steer Seek Force",
        &g_config.physics.steerSeekForce, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "curve_fade_distance", "Curve Fade Distance",
        &g_config.physics.curveFadeDistance, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "curve_tangent_force", "Curve Tangent Force",
        &g_config.physics.curveTangentForce, 0.0f, 100.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "curve_fade_min_vel", "Curve Fade Min Vel",
        &g_config.physics.curveFadeMinVel, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "direction_rotate_min_vel", "Direction Rotate Min Vel",
        &g_config.physics.directionRotateMinVel, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "direction_to_cursor_dist", "Direction To Cursor Dist",
        &g_config.physics.directionToCursorDist, 0.0f, 2000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "snap_distance", "Snap Distance",
        &g_config.physics.snapDistance, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "direction_reverse_multiplier", "Direction Reverse Multiplier",
        &g_config.physics.directionReverseMultiplier, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Physics", "min_valid_scale", "Min Valid Scale",
        &g_config.physics.minValidScale, 0.01f, 1.0f, 0.01f, OnConfigChange));
}

void RegisterSpawn(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Spawn", "margin_x", "Margin X",
        &g_config.spawn.marginX, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "margin_y", "Margin Y",
        &g_config.spawn.marginY, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "target_reached_threshold_return", "Target Reached Threshold Return",
        &g_config.spawn.targetReachedThresholdReturn, 1.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "target_reached_threshold_normal", "Target Reached Threshold Normal",
        &g_config.spawn.targetReachedThresholdNormal, 1.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "target_reached_min_return", "Target Reached Min Return",
        &g_config.spawn.targetReachedMinReturn, 1.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "target_reached_min_normal", "Target Reached Min Normal",
        &g_config.spawn.targetReachedMinNormal, 1.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "random_target_margin_x", "Random Target Margin X",
        &g_config.spawn.randomTargetMarginX, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "random_target_margin_y", "Random Target Margin Y",
        &g_config.spawn.randomTargetMarginY, 0.0f, 1000.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "item_drop_margin_x", "Item Drop Margin X",
        &g_config.spawn.itemDropMarginX, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "item_drop_margin_y", "Item Drop Margin Y",
        &g_config.spawn.itemDropMarginY, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "item_pickup_distance", "Item Pickup Distance",
        &g_config.spawn.itemPickupDistance, 1.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "catch_threshold_base", "Catch Threshold Base",
        &g_config.spawn.catchThresholdBase, 1.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "catch_threshold_min", "Catch Threshold Min",
        &g_config.spawn.catchThresholdMin, 1.0f, 50.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "wander_target_margin", "Wander Target Margin",
        &g_config.spawn.wanderTargetMargin, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "wander_target_offset", "Wander Target Offset",
        &g_config.spawn.wanderTargetOffset, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "fetch_edge_margin", "Fetch Edge Margin",
        &g_config.spawn.fetchEdgeMargin, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_INT("Spawn", "max_fetching_geese", "Max Fetching Geese",
        &g_config.spawn.maxFetchingGeese, 0, 100, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "separation_max_distance", "Separation Max Distance",
        &g_config.spawn.separationMaxDistance, 1.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "separation_force_multiplier", "Separation Force Multiplier",
        &g_config.spawn.separationForceMultiplier, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Spawn", "separation_min_distance", "Separation Min Distance",
        &g_config.spawn.separationMinDistance, 1.0f, 200.0f, 1.0f, OnConfigChange));
}

void RegisterRig(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Rig", "underbody_y", "Underbody Y",
        &g_config.rig.underbodyY, -100.0f, 100.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "body_y", "Body Y",
        &g_config.rig.bodyY, -100.0f, 100.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "neck_base_x", "Neck Base X",
        &g_config.rig.neckBaseX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "neck_height_idle", "Neck Height Idle",
        &g_config.rig.neckHeightIdle, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "neck_height_moving", "Neck Height Moving",
        &g_config.rig.neckHeightMoving, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "neck_ext_idle", "Neck Ext Idle",
        &g_config.rig.neckExtIdle, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "neck_ext_moving", "Neck Ext Moving",
        &g_config.rig.neckExtMoving, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head_base_x", "Head Base X",
        &g_config.rig.headBaseX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head_base_y", "Head Base Y",
        &g_config.rig.headBaseY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head1_offset_x", "Head1Offset X",
        &g_config.rig.head1OffsetX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head1_offset_y", "Head1Offset Y",
        &g_config.rig.head1OffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head1_offset_z", "Head1Offset Z",
        &g_config.rig.head1OffsetZ, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head2_offset_x", "Head2Offset X",
        &g_config.rig.head2OffsetX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head2_offset_y", "Head2Offset Y",
        &g_config.rig.head2OffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "beak_base_offset", "Beak Base Offset",
        &g_config.rig.beakBaseOffset, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "beak_len", "Beak Len",
        &g_config.rig.beakLen, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "beak_width", "Beak Width",
        &g_config.rig.beakWidth, 0.0f, 30.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "neck_lerp_rate", "Neck Lerp Rate",
        &g_config.rig.neckLerpRate, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "run_speed_threshold", "Run Speed Threshold",
        &g_config.rig.runSpeedThreshold, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "stride_max", "Stride Max",
        &g_config.rig.strideMax, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "foot_spacing", "Foot Spacing",
        &g_config.rig.footSpacing, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "step_lift_height", "Step Lift Height",
        &g_config.rig.stepLiftHeight, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "foot_offset_y", "Foot Offset Y",
        &g_config.rig.footOffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Rig", "head_forward_bias", "Head Forward Bias",
        &g_config.rig.headForwardBias, -1.0f, 1.0f, 0.01f, OnConfigChange));
}

void RegisterHonk(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Honk", "min_gap", "Min Gap",
        &g_config.honk.minGap, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Honk", "idle_min", "Idle Min",
        &g_config.honk.idleMin, 0.0f, 60.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Honk", "idle_max", "Idle Max",
        &g_config.honk.idleMax, 0.0f, 120.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Honk", "generic_cooldown", "Generic Cooldown",
        &g_config.honk.genericCooldown, 0.0f, 30.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Honk", "chase_cooldown", "Chase Cooldown",
        &g_config.honk.chaseCooldown, 0.0f, 30.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Honk", "fetch_cooldown", "Fetch Cooldown",
        &g_config.honk.fetchCooldown, 0.0f, 30.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Honk", "idle_check_ahead", "Idle Check Ahead",
        &g_config.honk.idleCheckAhead, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_INT("Honk", "idle_chance_divisor", "Idle Chance Divisor",
        &g_config.honk.idleChanceDivisor, 1, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Honk", "wander_honk_divisor", "Wander Honk Divisor",
        &g_config.honk.wanderHonkDivisor, 1, 1000, OnConfigChange));
}

void RegisterSnatch(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Snatch", "radius_base", "Radius Base",
        &g_config.snatch.radiusBase, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "radius_range", "Radius Range",
        &g_config.snatch.radiusRange, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "angular_speed_base", "Angular Speed Base",
        &g_config.snatch.angularSpeedBase, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "speed_multiplier", "Speed Multiplier",
        &g_config.snatch.speedMultiplier, 0.0f, 5.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "offset_max", "Offset Max",
        &g_config.snatch.offsetMax, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "duration", "Duration",
        &g_config.snatch.duration, 0.1f, 60.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "pull_distance", "Pull Distance",
        &g_config.snatch.pullDistance, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "lateral_bias_limit", "Lateral Bias Limit",
        &g_config.snatch.lateralBiasLimit, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "forward_bias_scale", "Forward Bias Scale",
        &g_config.snatch.forwardBiasScale, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "forward_bias_min", "Forward Bias Min",
        &g_config.snatch.forwardBiasMin, -100.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "forward_bias_max", "Forward Bias Max",
        &g_config.snatch.forwardBiasMax, -100.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "tier3_max_step", "Tier3Max Step",
        &g_config.snatch.tier3MaxStep, 0.0f, 50.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "tier3_min_delta", "Tier3Min Delta",
        &g_config.snatch.tier3MinDelta, 0.0f, 50.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Snatch", "angular_speed_random_range", "Angular Speed Random Range",
        &g_config.snatch.angularSpeedRandomRange, 0.0f, 5.0f, 0.1f, OnConfigChange));
}

void RegisterStep(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Step", "time_fetch", "Time Fetch",
        &g_config.step.timeFetch, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "time_wander", "Time Wander",
        &g_config.step.timeWander, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "time_snatch", "Time Snatch",
        &g_config.step.timeSnatch, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "step_trigger_walk", "Step Trigger Walk",
        &g_config.step.stepTriggerWalk, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "step_trigger_run", "Step Trigger Run",
        &g_config.step.stepTriggerRun, 0.0f, 500.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "overshoot_walk", "Overshoot Walk",
        &g_config.step.overshootWalk, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "overshoot_run", "Overshoot Run",
        &g_config.step.overshootRun, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "duration_walk", "Duration Walk",
        &g_config.step.durationWalk, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "duration_run", "Duration Run",
        &g_config.step.durationRun, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "lift_walk", "Lift Walk",
        &g_config.step.liftWalk, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "lift_run", "Lift Run",
        &g_config.step.liftRun, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "snap_distance", "Snap Distance",
        &g_config.step.snapDistance, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "dist_factor_base", "Dist Factor Base",
        &g_config.step.distFactorBase, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "dist_factor_min", "Dist Factor Min",
        &g_config.step.distFactorMin, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "dist_factor_max", "Dist Factor Max",
        &g_config.step.distFactorMax, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "duration_min", "Duration Min",
        &g_config.step.durationMin, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "duration_max", "Duration Max",
        &g_config.step.durationMax, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "left_foot_angle", "Left Foot Angle",
        &g_config.step.leftFootAngle, -180.0f, 180.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "right_foot_angle", "Right Foot Angle",
        &g_config.step.rightFootAngle, -180.0f, 180.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "foot_spacing", "Foot Spacing",
        &g_config.step.footSpacing, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "zero_velocity_threshold", "Zero Velocity Threshold",
        &g_config.step.zeroVelocityThreshold, 0.0f, 100.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Step", "min_duration", "Min Duration",
        &g_config.step.minDuration, 0.0f, 0.5f, 0.01f, OnConfigChange));
}

void RegisterItem(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Item", "pickup_cooldown", "Pickup Cooldown",
        &g_config.item.pickupCooldown, 0.0f, 10.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Item", "item_lifetime", "Item Lifetime",
        &g_config.item.itemLifetime, 0.0f, 3600.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "meme_pickup_chance", "Meme Pickup Chance",
        &g_config.item.memePickupChance, 0, 100, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "fetch_base_chance", "Fetch Base Chance",
        &g_config.item.fetchBaseChance, 0, 100, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "max_fetch_bias", "Max Fetch Bias",
        &g_config.item.maxFetchBias, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "max_fetch_geese", "Max Fetch Geese",
        &g_config.item.maxFetchGeese, 0, 100, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "meme_fetch_bias_max", "Meme Fetch Bias Max",
        &g_config.item.memeFetchBiasMax, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "note_fetch_bias_max", "Note Fetch Bias Max",
        &g_config.item.noteFetchBiasMax, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "attack_mouse_bias_max", "Attack Mouse Bias Max",
        &g_config.item.attackMouseBiasMax, 0, 1000, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "heist_chance_percent", "Heist Chance Percent",
        &g_config.item.heistChancePercent, 0, 100, OnConfigChange));
    r.push_back(CONFIG_INT("Item", "heist_approach_margin", "Heist Approach Margin",
        &g_config.item.heistApproachMargin, 0, 500, OnConfigChange));
}

void RegisterMud(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_BOOL("Mud", "enabled", "Enabled",
        &g_config.mud.enabled, OnConfigChange));
    r.push_back(CONFIG_INT("Mud", "chance", "Chance",
        &g_config.mud.chance, 0, 100, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Mud", "lifetime", "Lifetime",
        &g_config.mud.lifetime, 0.0f, 60.0f, 1.0f, OnConfigChange));
}

void RegisterRender(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_FLOAT("Render", "shadow_offset_x", "Shadow Offset X",
        &g_config.render.shadowOffsetX, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "shadow_offset_y", "Shadow Offset Y",
        &g_config.render.shadowOffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "shadow_width", "Shadow Width",
        &g_config.render.shadowWidth, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "shadow_height", "Shadow Height",
        &g_config.render.shadowHeight, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "foot_size", "Foot Size",
        &g_config.render.footSize, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "body_width", "Body Width",
        &g_config.render.bodyWidth, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "body_height", "Body Height",
        &g_config.render.bodyHeight, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "neck_size", "Neck Size",
        &g_config.render.neckSize, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "head1_size", "Head1Size",
        &g_config.render.head1Size, 0.0f, 100.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "head2_size", "Head2Size",
        &g_config.render.head2Size, 0.0f, 100.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "beak_width", "Beak Width",
        &g_config.render.beakWidth, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "beak_height", "Beak Height",
        &g_config.render.beakHeight, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "beak_max_width", "Beak Max Width",
        &g_config.render.beakMaxWidth, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "eye_size", "Eye Size",
        &g_config.render.eyeSize, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "eye_offset_x_back", "Eye Offset Xback",
        &g_config.render.eyeOffsetXBack, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "eye_offset_x_front", "Eye Offset Xfront",
        &g_config.render.eyeOffsetXFront, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "eye_offset_y", "Eye Offset Y",
        &g_config.render.eyeOffsetY, -50.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "eye_facing_threshold", "Eye Facing Threshold",
        &g_config.render.eyeFacingThreshold, -1.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "click_radius", "Click Radius",
        &g_config.render.clickRadius, 0.0f, 100.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "footprint_width", "Footprint Width",
        &g_config.render.footprintWidth, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "footprint_height", "Footprint Height",
        &g_config.render.footprintHeight, 0.0f, 50.0f, 0.5f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "dropped_item_size", "Dropped Item Size",
        &g_config.render.droppedItemSize, 0.0f, 200.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "frame_rate", "Frame Rate",
        &g_config.render.frameRate, 1.0f, 120.0f, 1.0f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "frame_dt", "Frame Dt",
        &g_config.render.frameDt, 0.001f, 0.1f, 0.001f, OnConfigChange));
    r.push_back(CONFIG_INT("Render", "debug_tick_mod", "Debug Tick Mod",
        &g_config.render.debugTickMod, 1, 100, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "iso_scale_x", "Iso Scale X",
        &g_config.render.isoScaleX, 0.1f, 2.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "iso_scale_y", "Iso Scale Y",
        &g_config.render.isoScaleY, 0.1f, 2.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "squash_factor", "Squash Factor",
        &g_config.render.squashFactor, 0.1f, 2.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_FLOAT("Render", "facing_back_threshold", "Facing Back Threshold",
        &g_config.render.facingBackThreshold, -1.0f, 1.0f, 0.01f, OnConfigChange));
}

}

void RegisterAI(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_INT("AI", "provider_type", "Provider Type",
        &g_config.ai.providerType, 0, 2, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "osaurus_port", "Osaurus Port",
        &g_config.ai.osaurusPort, 0, 65535, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "ollama_port", "Ollama Port",
        &g_config.ai.ollamaPort, 0, 65535, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "custom_port", "Custom Port",
        &g_config.ai.customPort, 0, 65535, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "osaurus_model", "Osaurus Model",
        &g_config.ai.osaurusModel, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "ollama_model", "Ollama Model",
        &g_config.ai.ollamaModel, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "custom_endpoint", "Custom Endpoint",
        &g_config.ai.customEndpoint, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "custom_model", "Custom Model",
        &g_config.ai.customModel, OnConfigChange));
    r.push_back(CONFIG_FLOAT("AI", "evil_level", "Evil Level",
        &g_config.ai.evilLevel, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_BOOL("AI", "show_status_bar", "Show Status Bar",
        &g_config.ai.showStatusBar, OnConfigChange));
    r.push_back(CONFIG_BOOL("AI", "enable_mcp", "Enable MCP Server",
        &g_config.ai.enableMCP, OnConfigChange));
    r.push_back(CONFIG_BOOL("AI", "use_unix_socket", "Use Unix Socket",
        &g_config.ai.useUnixSocket, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "unix_socket_path", "Unix Socket Path",
        &g_config.ai.unixSocketPath, OnConfigChange));
}

void Config_InitRegistry() {
    g_configRegistry.clear();

    RegisterCommon(g_configRegistry);
    RegisterCursor(g_configRegistry);
    RegisterMovement(g_configRegistry);
    RegisterPhysics(g_configRegistry);
    RegisterSpawn(g_configRegistry);
    RegisterRig(g_configRegistry);
    RegisterHonk(g_configRegistry);
    RegisterSnatch(g_configRegistry);
    RegisterStep(g_configRegistry);
    RegisterItem(g_configRegistry);
    RegisterMud(g_configRegistry);
    RegisterRender(g_configRegistry);
    RegisterAI(g_configRegistry);
}