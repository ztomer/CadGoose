// config_registry_render.cpp
// Render config option registration
#include "config.h"

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
