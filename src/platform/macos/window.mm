#import "window.h"
#import "coordinate_system.h"
#include "goose.h"
#include "config.h"
#include "goose_math.h"
#include "world_coord.h"
#include "item_window_logic.h"

static constexpr float kGooseWindowSize = 600.0f;
static constexpr float kHeldItemPadding = 40.0f;
static constexpr float kHeldItemBeakOffset = 5.0f;

// Rotated-bounds maths lives in item_window_logic (tested there). This file and
// item_window.mm each used to carry their own copy of it; they are the same
// computation, with scale folded in as a parameter.

float CalculateGooseWindowSize(const Goose* goose) {
    float baseSize = kGooseWindowSize;
    if (goose && goose->heldItem) {
        float scale = g_config.general.globalScale;
        float itemW = goose->heldItem->w * scale;
        float itemH = goose->heldItem->h * scale;

        Vector2 neckHeadDev = WorldCoord::RigNeckHead(*goose).toVector2();
        float distToBeak = Vector2::Distance({goose->pos.x, goose->pos.y}, neckHeadDev);

        float itemBehindBeak = itemW + kHeldItemBeakOffset;
        DevicePoint rotatedSize = item_window_logic::RotatedBoundsSize(itemW, itemH, goose->dragRot, 1.0f);
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
