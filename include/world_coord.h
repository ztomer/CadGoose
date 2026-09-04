// world_coord.h
// WorldCoord — coordinate transformation helpers using the typed-coordinate
// system (DevicePoint / WorldPoint / ScreenPoint / ViewPoint). All methods
// route through CoordTransform in coordinate_system.h and read the current
// global scale from g_config.

#pragma once

#include "coordinate_system.h"
#include "config.h"
#include "goose.h"
#include "items.h"

class WorldCoord {
public:
    // goose-local world coords → device coords
    static DevicePoint WorldToDevice(WorldPoint worldPos, DevicePoint goosePos, float globalScale) {
        return CoordTransform::WorldToDevice(worldPos, goosePos, globalScale);
    }

    static DevicePoint WorldToDevice(WorldPoint worldPos, const Goose& goose) {
        return CoordTransform::WorldToDevice(worldPos, DevicePoint{goose.pos.x, goose.pos.y}, g_config.general.globalScale);
    }

    // origin-relative world coords → device coords
    static DevicePoint OriginToDevice(WorldPoint worldPos) {
        return {worldPos.x * g_config.general.globalScale, worldPos.y * g_config.general.globalScale};
    }

    // rig parts are in goose-local world space
    static DevicePoint RigNeckHead(const Goose& goose) {
        return WorldToDevice(WorldPoint{goose.rig.neckHead.x, goose.rig.neckHead.y}, goose);
    }

    static DevicePoint RigBody(const Goose& goose) {
        return WorldToDevice(WorldPoint{goose.rig.body.x, goose.rig.body.y}, goose);
    }

    // item coordinate helpers (all return DEVICE coords)
    static DevicePoint ItemCenter(const DroppedItem& item) {
        return ItemCoords::Center({item.pos.x, item.pos.y}, item.data->w, item.data->h, g_config.general.globalScale);
    }

    static DevicePoint ItemHalfSize(const ItemData* item) {
        return ItemCoords::HalfSize(item->w, item->h, g_config.general.globalScale);
    }

    static DevicePoint ItemSize(const ItemData* item) {
        return ItemCoords::Size(item->w, item->h, g_config.general.globalScale);
    }

    // scalar scaling
    static float Scale(float worldValue) {
        return CoordTransform::Scale(worldValue, g_config.general.globalScale);
    }

    // Y-flip helpers for Linux Cairo
    static DevicePoint FromCairo(float cairoX, float cairoY, float screenHeight) {
        return CoordTransform::CairoToDevice(cairoX, cairoY, screenHeight);
    }

    static Vector2 ToCairo(DevicePoint devicePos, float screenHeight) {
        return CoordTransform::DeviceToCairo(devicePos, screenHeight);
    }
};
