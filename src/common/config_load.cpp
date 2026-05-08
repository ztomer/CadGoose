#include "config.h"

namespace fs = std::filesystem;

static fs::path ConfigDirPath() {
    if (fs::exists(fs::current_path() / "config" / "config.toml")) {
        return fs::current_path() / "config";
    }

#if defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) {
        if (*home) return fs::path(home) / "Library" / "Application Support" / "CadGoose";
    }
    return fs::current_path();
#else
    if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdgConfig) return fs::path(xdgConfig) / "desktop-goose";
    }
    if (const char* home = std::getenv("HOME")) {
        if (*home) return fs::path(home) / ".config" / "desktop-goose";
    }
    return fs::current_path();
#endif
}

std::string Config_GetPath() {
    return (ConfigDirPath() / "config.toml").string();
}

void Config_LoadDebug(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_bool(config, "debug", "toTerminal", g_config.debug.toTerminal);
    config_helpers::get_bool(config, "debug", "visuals", g_config.debug.visuals);
}

void Config_LoadGeneral(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "general", "globalScale", g_config.general.globalScale);
    config_helpers::get_bool(config, "general", "audioEnabled", g_config.general.audioEnabled);
    config_helpers::get_bool(config, "general", "memesEnabled", g_config.general.memesEnabled);
    config_helpers::get_bool(config, "general", "canadaGooseMode", g_config.general.canadaGooseMode);
}

void Config_LoadScreen(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_int(config, "screen", "defaultWidth", g_config.screen.defaultWidth);
    config_helpers::get_int(config, "screen", "defaultHeight", g_config.screen.defaultHeight);
}

void Config_LoadAsset(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_int(config, "asset", "memePlaceholderW", g_config.asset.memePlaceholderW);
    config_helpers::get_int(config, "asset", "memePlaceholderH", g_config.asset.memePlaceholderH);
    config_helpers::get_int(config, "asset", "textPlaceholderW", g_config.asset.textPlaceholderW);
    config_helpers::get_int(config, "asset", "textPlaceholderH", g_config.asset.textPlaceholderH);
    config_helpers::get_int(config, "asset", "notePlaceholderW", g_config.asset.notePlaceholderW);
    config_helpers::get_int(config, "asset", "notePlaceholderH", g_config.asset.notePlaceholderH);
}

void Config_LoadMovement(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "movement", "baseWalkSpeed", g_config.movement.baseWalkSpeed);
    config_helpers::get_float(config, "movement", "baseRunSpeed", g_config.movement.baseRunSpeed);
    config_helpers::get_float(config, "movement", "maxForce", g_config.movement.maxForce);
    config_helpers::get_float(config, "movement", "moveDistanceThreshold", g_config.movement.moveDistanceThreshold);
    config_helpers::get_float(config, "movement", "friction", g_config.movement.friction);
    config_helpers::get_float(config, "movement", "maxSpeed", g_config.movement.maxSpeed);
    config_helpers::get_float(config, "movement", "runSpeedMultiplier", g_config.movement.runSpeedMultiplier);
    config_helpers::get_float(config, "movement", "initDirectionMax", g_config.movement.initDirectionMax);
    config_helpers::get_float(config, "movement", "speedLerpRate", g_config.movement.speedLerpRate);
    config_helpers::get_float(config, "movement", "arrivalRadius", g_config.movement.arrivalRadius);
    config_helpers::get_float(config, "movement", "runDistanceThreshold", g_config.movement.runDistanceThreshold);
    config_helpers::get_float(config, "movement", "directionBlendRate", g_config.movement.directionBlendRate);
}

void Config_LoadPhysics(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "physics", "screenMargin", g_config.physics.screenMargin);
    config_helpers::get_float(config, "physics", "bounceFactorWall", g_config.physics.bounceFactorWall);
    config_helpers::get_float(config, "physics", "bounceFactorCorner", g_config.physics.bounceFactorCorner);
    config_helpers::get_float(config, "physics", "cornerMargin", g_config.physics.cornerMargin);
    config_helpers::get_float(config, "physics", "dragMinDt", g_config.physics.dragMinDt);
    config_helpers::get_float(config, "physics", "dragVelocityThreshold", g_config.physics.dragVelocityThreshold);
    config_helpers::get_float(config, "physics", "dragRotationSpeed", g_config.physics.dragRotationSpeed);
    config_helpers::get_float(config, "physics", "isoScaleX", g_config.physics.isoScaleX);
    config_helpers::get_float(config, "physics", "isoScaleY", g_config.physics.isoScaleY);
    config_helpers::get_float(config, "physics", "edgeAvoidMargin", g_config.physics.edgeAvoidMargin);
    config_helpers::get_float(config, "physics", "edgeLookAheadSpeed", g_config.physics.edgeLookAheadSpeed);
    config_helpers::get_float(config, "physics", "edgeLookAheadBase", g_config.physics.edgeLookAheadBase);
    config_helpers::get_float(config, "physics", "edgeAvoidForce", g_config.physics.edgeAvoidForce);
    config_helpers::get_float(config, "physics", "screenClampTight", g_config.physics.screenClampTight);
    config_helpers::get_float(config, "physics", "screenClampExpanded", g_config.physics.screenClampExpanded);
    config_helpers::get_float(config, "physics", "screenClampBounce", g_config.physics.screenClampBounce);
    config_helpers::get_float(config, "physics", "steerSeekForce", g_config.physics.steerSeekForce);
    config_helpers::get_float(config, "physics", "curveFadeDistance", g_config.physics.curveFadeDistance);
    config_helpers::get_float(config, "physics", "curveTangentForce", g_config.physics.curveTangentForce);
    config_helpers::get_float(config, "physics", "curveFadeMinVel", g_config.physics.curveFadeMinVel);
    config_helpers::get_float(config, "physics", "directionRotateMinVel", g_config.physics.directionRotateMinVel);
    config_helpers::get_float(config, "physics", "directionToCursorDist", g_config.physics.directionToCursorDist);
    config_helpers::get_float(config, "physics", "snapDistance", g_config.physics.snapDistance);
    config_helpers::get_float(config, "physics", "directionReverseMultiplier", g_config.physics.directionReverseMultiplier);
    config_helpers::get_float(config, "physics", "minValidScale", g_config.physics.minValidScale);
}

void Config_LoadSpawn(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "spawn", "marginX", g_config.spawn.marginX);
    config_helpers::get_float(config, "spawn", "marginY", g_config.spawn.marginY);
    config_helpers::get_float(config, "spawn", "targetReachedThresholdReturn", g_config.spawn.targetReachedThresholdReturn);
    config_helpers::get_float(config, "spawn", "targetReachedThresholdNormal", g_config.spawn.targetReachedThresholdNormal);
    config_helpers::get_float(config, "spawn", "targetReachedMinReturn", g_config.spawn.targetReachedMinReturn);
    config_helpers::get_float(config, "spawn", "targetReachedMinNormal", g_config.spawn.targetReachedMinNormal);
    config_helpers::get_float(config, "spawn", "randomTargetMarginX", g_config.spawn.randomTargetMarginX);
    config_helpers::get_float(config, "spawn", "randomTargetMarginY", g_config.spawn.randomTargetMarginY);
    config_helpers::get_float(config, "spawn", "itemDropMarginX", g_config.spawn.itemDropMarginX);
    config_helpers::get_float(config, "spawn", "itemDropMarginY", g_config.spawn.itemDropMarginY);
    config_helpers::get_float(config, "spawn", "itemPickupDistance", g_config.spawn.itemPickupDistance);
    config_helpers::get_float(config, "spawn", "catchThresholdBase", g_config.spawn.catchThresholdBase);
    config_helpers::get_float(config, "spawn", "catchThresholdMin", g_config.spawn.catchThresholdMin);
    config_helpers::get_float(config, "spawn", "wanderTargetMargin", g_config.spawn.wanderTargetMargin);
    config_helpers::get_float(config, "spawn", "wanderTargetOffset", g_config.spawn.wanderTargetOffset);
    config_helpers::get_float(config, "spawn", "fetchEdgeMargin", g_config.spawn.fetchEdgeMargin);
    config_helpers::get_float(config, "spawn", "maxFetchingGeese", g_config.spawn.maxFetchingGeese);
    config_helpers::get_float(config, "spawn", "separationMaxDistance", g_config.spawn.separationMaxDistance);
    config_helpers::get_float(config, "spawn", "separationForceMultiplier", g_config.spawn.separationForceMultiplier);
    config_helpers::get_float(config, "spawn", "separationMinDistance", g_config.spawn.separationMinDistance);
}

void Config_LoadRig(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "rig", "underbodyY", g_config.rig.underbodyY);
    config_helpers::get_float(config, "rig", "bodyY", g_config.rig.bodyY);
    config_helpers::get_float(config, "rig", "neckBaseX", g_config.rig.neckBaseX);
    config_helpers::get_float(config, "rig", "neckHeightIdle", g_config.rig.neckHeightIdle);
    config_helpers::get_float(config, "rig", "neckHeightMoving", g_config.rig.neckHeightMoving);
    config_helpers::get_float(config, "rig", "neckExtIdle", g_config.rig.neckExtIdle);
    config_helpers::get_float(config, "rig", "neckExtMoving", g_config.rig.neckExtMoving);
    config_helpers::get_float(config, "rig", "headBaseX", g_config.rig.headBaseX);
    config_helpers::get_float(config, "rig", "headBaseY", g_config.rig.headBaseY);
    config_helpers::get_float(config, "rig", "head1OffsetX", g_config.rig.head1OffsetX);
    config_helpers::get_float(config, "rig", "head1OffsetY", g_config.rig.head1OffsetY);
    config_helpers::get_float(config, "rig", "head1OffsetZ", g_config.rig.head1OffsetZ);
    config_helpers::get_float(config, "rig", "head2OffsetX", g_config.rig.head2OffsetX);
    config_helpers::get_float(config, "rig", "head2OffsetY", g_config.rig.head2OffsetY);
    config_helpers::get_float(config, "rig", "beakBaseOffset", g_config.rig.beakBaseOffset);
    config_helpers::get_float(config, "rig", "beakLen", g_config.rig.beakLen);
    config_helpers::get_float(config, "rig", "beakWidth", g_config.rig.beakWidth);
    config_helpers::get_float(config, "rig", "neckLerpRate", g_config.rig.neckLerpRate);
    config_helpers::get_float(config, "rig", "runSpeedThreshold", g_config.rig.runSpeedThreshold);
    config_helpers::get_float(config, "rig", "strideMax", g_config.rig.strideMax);
    config_helpers::get_float(config, "rig", "footSpacing", g_config.rig.footSpacing);
    config_helpers::get_float(config, "rig", "stepLiftHeight", g_config.rig.stepLiftHeight);
    config_helpers::get_float(config, "rig", "footOffsetY", g_config.rig.footOffsetY);
    config_helpers::get_float(config, "rig", "headForwardBias", g_config.rig.headForwardBias);
}

void Config_LoadCursor(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_bool(config, "cursor", "chaseEnabled", g_config.cursor.chaseEnabled);
    config_helpers::get_int(config, "cursor", "chaseChance", g_config.cursor.chaseChance);
    config_helpers::get_bool(config, "cursor", "multiMonitorEnabled", g_config.cursor.multiMonitorEnabled);
}

void Config_LoadSnatch(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "snatch", "radiusBase", g_config.snatch.radiusBase);
    config_helpers::get_float(config, "snatch", "radiusRange", g_config.snatch.radiusRange);
    config_helpers::get_float(config, "snatch", "angularSpeedBase", g_config.snatch.angularSpeedBase);
    config_helpers::get_float(config, "snatch", "speedMultiplier", g_config.snatch.speedMultiplier);
    config_helpers::get_float(config, "snatch", "offsetMax", g_config.snatch.offsetMax);
    config_helpers::get_float(config, "snatch", "duration", g_config.snatch.duration);
    config_helpers::get_float(config, "snatch", "pullDistance", g_config.snatch.pullDistance);
    config_helpers::get_float(config, "snatch", "lateralBiasLimit", g_config.snatch.lateralBiasLimit);
    config_helpers::get_float(config, "snatch", "forwardBiasScale", g_config.snatch.forwardBiasScale);
    config_helpers::get_float(config, "snatch", "forwardBiasMin", g_config.snatch.forwardBiasMin);
    config_helpers::get_float(config, "snatch", "forwardBiasMax", g_config.snatch.forwardBiasMax);
    config_helpers::get_float(config, "snatch", "tier3MaxStep", g_config.snatch.tier3MaxStep);
    config_helpers::get_float(config, "snatch", "tier3MinDelta", g_config.snatch.tier3MinDelta);
    config_helpers::get_float(config, "snatch", "angularSpeedRandomRange", g_config.snatch.angularSpeedRandomRange);
}

void Config_LoadMud(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_bool(config, "mud", "enabled", g_config.mud.enabled);
    config_helpers::get_int(config, "mud", "chance", g_config.mud.chance);
    config_helpers::get_float(config, "mud", "lifetime", g_config.mud.lifetime);
}

void Config_LoadHonk(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "honk", "minGap", g_config.honk.minGap);
    config_helpers::get_float(config, "honk", "idleMin", g_config.honk.idleMin);
    config_helpers::get_float(config, "honk", "idleMax", g_config.honk.idleMax);
    config_helpers::get_float(config, "honk", "genericCooldown", g_config.honk.genericCooldown);
    config_helpers::get_float(config, "honk", "chaseCooldown", g_config.honk.chaseCooldown);
    config_helpers::get_float(config, "honk", "fetchCooldown", g_config.honk.fetchCooldown);
    config_helpers::get_float(config, "honk", "idleCheckAhead", g_config.honk.idleCheckAhead);
    config_helpers::get_int(config, "honk", "idleChanceDivisor", g_config.honk.idleChanceDivisor);
    config_helpers::get_int(config, "honk", "wanderHonkDivisor", g_config.honk.wanderHonkDivisor);
}

void Config_LoadStep(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "step", "timeFetch", g_config.step.timeFetch);
    config_helpers::get_float(config, "step", "timeWander", g_config.step.timeWander);
    config_helpers::get_float(config, "step", "timeSnatch", g_config.step.timeSnatch);
    config_helpers::get_float(config, "step", "stepTriggerWalk", g_config.step.stepTriggerWalk);
    config_helpers::get_float(config, "step", "stepTriggerRun", g_config.step.stepTriggerRun);
    config_helpers::get_float(config, "step", "overshootWalk", g_config.step.overshootWalk);
    config_helpers::get_float(config, "step", "overshootRun", g_config.step.overshootRun);
    config_helpers::get_float(config, "step", "durationWalk", g_config.step.durationWalk);
    config_helpers::get_float(config, "step", "durationRun", g_config.step.durationRun);
    config_helpers::get_float(config, "step", "liftWalk", g_config.step.liftWalk);
    config_helpers::get_float(config, "step", "liftRun", g_config.step.liftRun);
    config_helpers::get_float(config, "step", "snapDistance", g_config.step.snapDistance);
    config_helpers::get_float(config, "step", "distFactorBase", g_config.step.distFactorBase);
    config_helpers::get_float(config, "step", "distFactorMin", g_config.step.distFactorMin);
    config_helpers::get_float(config, "step", "distFactorMax", g_config.step.distFactorMax);
    config_helpers::get_float(config, "step", "durationMin", g_config.step.durationMin);
    config_helpers::get_float(config, "step", "durationMax", g_config.step.durationMax);
    config_helpers::get_float(config, "step", "leftFootAngle", g_config.step.leftFootAngle);
    config_helpers::get_float(config, "step", "rightFootAngle", g_config.step.rightFootAngle);
    config_helpers::get_float(config, "step", "footSpacing", g_config.step.footSpacing);
    config_helpers::get_float(config, "step", "zeroVelocityThreshold", g_config.step.zeroVelocityThreshold);
    config_helpers::get_float(config, "step", "minDuration", g_config.step.minDuration);
}

void Config_LoadItem(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "item", "pickupCooldown", g_config.item.pickupCooldown);
    config_helpers::get_float(config, "item", "itemLifetime", g_config.item.itemLifetime);
    config_helpers::get_int(config, "item", "memePickupChance", g_config.item.memePickupChance);
    config_helpers::get_int(config, "item", "fetchBaseChance", g_config.item.fetchBaseChance);
    config_helpers::get_int(config, "item", "maxFetchBias", g_config.item.maxFetchBias);
    config_helpers::get_int(config, "item", "maxFetchGeese", g_config.item.maxFetchGeese);
    config_helpers::get_int(config, "item", "memeFetchBiasMax", g_config.item.memeFetchBiasMax);
    config_helpers::get_int(config, "item", "noteFetchBiasMax", g_config.item.noteFetchBiasMax);
    config_helpers::get_int(config, "item", "attackMouseBiasMax", g_config.item.attackMouseBiasMax);
    config_helpers::get_int(config, "item", "heistChancePercent", g_config.item.heistChancePercent);
    config_helpers::get_int(config, "item", "heistApproachMargin", g_config.item.heistApproachMargin);
}

void Config_LoadRender(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_float(config, "render", "shadowOffsetX", g_config.render.shadowOffsetX);
    config_helpers::get_float(config, "render", "shadowOffsetY", g_config.render.shadowOffsetY);
    config_helpers::get_float(config, "render", "shadowWidth", g_config.render.shadowWidth);
    config_helpers::get_float(config, "render", "shadowHeight", g_config.render.shadowHeight);
    config_helpers::get_float(config, "render", "footSize", g_config.render.footSize);
    config_helpers::get_float(config, "render", "bodyWidth", g_config.render.bodyWidth);
    config_helpers::get_float(config, "render", "bodyHeight", g_config.render.bodyHeight);
    config_helpers::get_float(config, "render", "neckSize", g_config.render.neckSize);
    config_helpers::get_float(config, "render", "head1Size", g_config.render.head1Size);
    config_helpers::get_float(config, "render", "head2Size", g_config.render.head2Size);
    config_helpers::get_float(config, "render", "beakWidth", g_config.render.beakWidth);
    config_helpers::get_float(config, "render", "beakHeight", g_config.render.beakHeight);
    config_helpers::get_float(config, "render", "beakMaxWidth", g_config.render.beakMaxWidth);
    config_helpers::get_float(config, "render", "eyeSize", g_config.render.eyeSize);
    config_helpers::get_float(config, "render", "eyeOffsetXBack", g_config.render.eyeOffsetXBack);
    config_helpers::get_float(config, "render", "eyeOffsetXFront", g_config.render.eyeOffsetXFront);
    config_helpers::get_float(config, "render", "eyeOffsetY", g_config.render.eyeOffsetY);
    config_helpers::get_float(config, "render", "eyeFacingThreshold", g_config.render.eyeFacingThreshold);
    config_helpers::get_float(config, "render", "clickRadius", g_config.render.clickRadius);
    config_helpers::get_float(config, "render", "footprintWidth", g_config.render.footprintWidth);
    config_helpers::get_float(config, "render", "footprintHeight", g_config.render.footprintHeight);
    config_helpers::get_float(config, "render", "droppedItemSize", g_config.render.droppedItemSize);
    config_helpers::get_float(config, "render", "frameRate", g_config.render.frameRate);
    config_helpers::get_float(config, "render", "frameDt", g_config.render.frameDt);
    config_helpers::get_int(config, "render", "debugTickMod", g_config.render.debugTickMod);
    config_helpers::get_float(config, "render", "isoScaleX", g_config.render.isoScaleX);
    config_helpers::get_float(config, "render", "isoScaleY", g_config.render.isoScaleY);
    config_helpers::get_float(config, "render", "squashFactor", g_config.render.squashFactor);
    config_helpers::get_float(config, "render", "facingBackThreshold", g_config.render.facingBackThreshold);
    config_helpers::get_float(config, "render", "closeButtonSize", g_config.render.closeButtonSize);
    config_helpers::get_float(config, "render", "closeButtonMargin", g_config.render.closeButtonMargin);
    config_helpers::get_float(config, "render", "textNoteFontSize", g_config.render.textNoteFontSize);
    config_helpers::get_float(config, "render", "textNotePadding", g_config.render.textNotePadding);
}

void Config_LoadColor(const toml::basic_value<toml::type_config>& config) {
    config_helpers::get_color_rgb(config, "color", "goose", "goose", g_config.color.goose.r, g_config.color.goose.g, g_config.color.goose.b);
    config_helpers::get_color_rgb(config, "color", "beak", "beak", g_config.color.beak.r, g_config.color.beak.g, g_config.color.beak.b);
    config_helpers::get_color_rgb(config, "color", "eye", "eye", g_config.color.eye.r, g_config.color.eye.g, g_config.color.eye.b);
    config_helpers::get_color_rgba(config, "color", "eyeHighlight", "eyeHighlight", g_config.color.eyeHighlight.r, g_config.color.eyeHighlight.g, g_config.color.eyeHighlight.b, g_config.color.eyeHighlight.a);
    config_helpers::get_color_rgb(config, "color", "shadow", "shadow", g_config.color.shadow.r, g_config.color.shadow.g, g_config.color.shadow.b);
    config_helpers::get_color_rgb(config, "color", "footprint", "footprint", g_config.color.footprint.r, g_config.color.footprint.g, g_config.color.footprint.b);
    config_helpers::get_float(config, "color", "footprintAlphaMultiplier", g_config.color.footprintAlphaMultiplier);
    config_helpers::get_color_rgb(config, "color", "droppedItem", "droppedItem", g_config.color.droppedItem.r, g_config.color.droppedItem.g, g_config.color.droppedItem.b);
    config_helpers::get_color_rgb(config, "color", "canadaHead", "canadaHead", g_config.color.canadaHead.r, g_config.color.canadaHead.g, g_config.color.canadaHead.b);
    config_helpers::get_color_rgb(config, "color", "canadaNeck", "canadaNeck", g_config.color.canadaNeck.r, g_config.color.canadaNeck.g, g_config.color.canadaNeck.b);
    config_helpers::get_color_rgb(config, "color", "canadaBody", "canadaBody", g_config.color.canadaBody.r, g_config.color.canadaBody.g, g_config.color.canadaBody.b);
    config_helpers::get_color_rgb(config, "color", "canadaOutline", "canadaOutline", g_config.color.canadaOutline.r, g_config.color.canadaOutline.g, g_config.color.canadaOutline.b);
    config_helpers::get_color_rgb(config, "color", "canadaBeak", "canadaBeak", g_config.color.canadaBeak.r, g_config.color.canadaBeak.g, g_config.color.canadaBeak.b);
    config_helpers::get_color_rgb(config, "color", "canadaEye", "canadaEye", g_config.color.canadaEye.r, g_config.color.canadaEye.g, g_config.color.canadaEye.b);
}

void Config_Load(const toml::basic_value<toml::type_config>& config) {
    Config_LoadDebug(config);
    Config_LoadGeneral(config);
    Config_LoadScreen(config);
    Config_LoadAsset(config);
    Config_LoadMovement(config);
    Config_LoadPhysics(config);
    Config_LoadSpawn(config);
    Config_LoadRig(config);
    Config_LoadCursor(config);
    Config_LoadSnatch(config);
    Config_LoadMud(config);
    Config_LoadHonk(config);
    Config_LoadStep(config);
    Config_LoadItem(config);
    Config_LoadRender(config);
    Config_LoadColor(config);
}

static void Config_LoadFromFile(const fs::path& path) {
    auto config = toml::parse(path.string());
    Config_Load(config);
}

void Config_LoadAll() {
    std::vector<fs::path> candidates = {
        fs::path(Config_GetPath()),
        fs::current_path() / "config.toml",
        fs::current_path() / "config.ini",
    };
    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            try {
                Config_LoadFromFile(path);
                return;
            } catch (const std::exception&) {
                continue;
            }
        }
    }
}