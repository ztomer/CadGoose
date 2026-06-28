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
        float needed = totalExtent * 2.0f;
        if (needed > baseSize) {
            // Quantize up to a coarse grid: the held-item extent changes a little
            // every frame (item rotation/distance), and an exact size means a full
            // NSWindow resize (setFrame) each frame. Snapping to a grid keeps the
            // size stable across frames so updatePosition takes the cheap origin-
            // only path; the transparent click-through window over-sizes for free.
            constexpr float kGooseWindowSizeStep = 48.0f;
            baseSize = std::ceil(needed / kGooseWindowSizeStep) * kGooseWindowSizeStep;
        }
    }
    return baseSize;
}
