#import "window.h"
#import "coordinate_system.h"
#include "goose.h"
#include "config.h"
#include "goose_math.h"
#include "world_coord.h"

static constexpr float kGooseWindowSize = 600.0f;
static constexpr float kHeldItemPadding = 40.0f;
static constexpr float kHeldItemBeakOffset = 5.0f;

static DevicePoint RotatedBoundsSize(float width, float height, float rotation) {
    float cosA = std::abs(std::cos(rotation));
    float sinA = std::abs(std::sin(rotation));
    return {width * cosA + height * sinA, width * sinA + height * cosA};
}

float CalculateGooseWindowSize(const Goose* goose) {
    float baseSize = kGooseWindowSize;
    if (goose && goose->heldItem) {
        float scale = g_config.general.globalScale;
        float itemW = goose->heldItem->w * scale;
        float itemH = goose->heldItem->h * scale;

        Vector2 neckHeadDev = WorldCoord::RigNeckHead(*goose).toVector2();
        float distToBeak = Vector2::Distance({goose->pos.x, goose->pos.y}, neckHeadDev);

        float itemBehindBeak = itemW + kHeldItemBeakOffset;
        DevicePoint rotatedSize = RotatedBoundsSize(itemW, itemH, goose->dragRot);
        float maxItemExtent = std::max(rotatedSize.x, rotatedSize.y) * 0.5f;

        float totalExtent = distToBeak + itemBehindBeak + maxItemExtent + kHeldItemPadding;
        baseSize = std::max(baseSize, totalExtent * 2.0f);
    }
    return baseSize;
}
