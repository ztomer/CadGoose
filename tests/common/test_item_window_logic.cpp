// test_item_window_logic.cpp — the pure decision layer lifted out of ItemWindow.
//
// ItemWindow is an NSWindow subclass and cannot be built headlessly, so this
// logic was unreachable (item_window.mm sat at 0%). Extracted into
// src/common/item_window_logic.cpp, it tests directly.

#include <gtest/gtest.h>
#include <cmath>

#include "item_window_logic.h"

using namespace item_window_logic;

namespace {
constexpr float kEps = 0.01f;
}

// ── RotatedBoundsSize ──────────────────────────────────────

TEST(ItemWindowLogic, UnrotatedBoundsAreJustTheScaledRect) {
    DevicePoint s = RotatedBoundsSize(100.0f, 50.0f, 0.0f, 1.0f);
    EXPECT_NEAR(s.x, 100.0f, kEps);
    EXPECT_NEAR(s.y, 50.0f, kEps);
}

TEST(ItemWindowLogic, ScaleMultipliesBothAxes) {
    DevicePoint s = RotatedBoundsSize(100.0f, 50.0f, 0.0f, 2.0f);
    EXPECT_NEAR(s.x, 200.0f, kEps);
    EXPECT_NEAR(s.y, 100.0f, kEps);
}

TEST(ItemWindowLogic, QuarterTurnSwapsWidthAndHeight) {
    DevicePoint s = RotatedBoundsSize(100.0f, 50.0f, static_cast<float>(M_PI) / 2.0f, 1.0f);
    EXPECT_NEAR(s.x, 50.0f, 0.1f);
    EXPECT_NEAR(s.y, 100.0f, 0.1f);
}

TEST(ItemWindowLogic, HalfTurnRestoresOriginalBounds) {
    DevicePoint s = RotatedBoundsSize(100.0f, 50.0f, static_cast<float>(M_PI), 1.0f);
    EXPECT_NEAR(s.x, 100.0f, 0.1f);
    EXPECT_NEAR(s.y, 50.0f, 0.1f);
}

TEST(ItemWindowLogic, FortyFiveDegreesExpandsToTheDiagonal) {
    // A square rotated 45 degrees needs a box sqrt(2) times its side.
    DevicePoint s = RotatedBoundsSize(100.0f, 100.0f, static_cast<float>(M_PI) / 4.0f, 1.0f);
    const float expected = 100.0f * std::sqrt(2.0f);
    EXPECT_NEAR(s.x, expected, 0.5f);
    EXPECT_NEAR(s.y, expected, 0.5f);
    EXPECT_GT(s.x, 100.0f) << "a rotated item must need MORE room, never less";
}

TEST(ItemWindowLogic, NegativeRotationMatchesPositive) {
    // abs() on the trig terms means direction of rotation cannot shrink the box.
    DevicePoint pos = RotatedBoundsSize(80.0f, 40.0f, 0.7f, 1.0f);
    DevicePoint neg = RotatedBoundsSize(80.0f, 40.0f, -0.7f, 1.0f);
    EXPECT_NEAR(pos.x, neg.x, kEps);
    EXPECT_NEAR(pos.y, neg.y, kEps);
}

// ── ShouldUpdatePosition ───────────────────────────────────

TEST(ItemWindowLogic, FirstPositionUpdateAlwaysRuns) {
    EXPECT_TRUE(ShouldUpdatePosition({0, 0}, {0, 0}, /*hasLastPosition=*/false))
        << "with no previous frame there is nothing to compare against";
}

TEST(ItemWindowLogic, IdenticalPositionSkipsTheUpdate) {
    EXPECT_FALSE(ShouldUpdatePosition({100.0f, 200.0f}, {100.0f, 200.0f}, true));
}

TEST(ItemWindowLogic, SubEpsilonMovementSkipsTheUpdate) {
    EXPECT_FALSE(ShouldUpdatePosition({100.0f, 200.0f}, {100.05f, 200.05f}, true))
        << "movement under 0.1px must not trigger a window reposition";
}

TEST(ItemWindowLogic, MovementAtOrAboveEpsilonTriggersAnUpdate) {
    EXPECT_TRUE(ShouldUpdatePosition({100.0f, 200.0f}, {100.2f, 200.0f}, true));
    EXPECT_TRUE(ShouldUpdatePosition({100.0f, 200.0f}, {100.0f, 200.2f}, true))
        << "either axis alone must be enough";
}

TEST(ItemWindowLogic, NegativeMovementCountsToo) {
    EXPECT_TRUE(ShouldUpdatePosition({100.0f, 200.0f}, {99.0f, 200.0f}, true))
        << "the epsilon compares magnitude, not sign";
}

// ── ComputeWindowFrame ─────────────────────────────────────

TEST(ItemWindowLogic, WindowIsSizedToTheRotatedBounds) {
    auto f = ComputeWindowFrame({100.0f, 100.0f}, 60.0f, 40.0f, 0.0f, 1.0f, 1000.0f);
    EXPECT_NEAR(f.size.x, 60.0f, kEps);
    EXPECT_NEAR(f.size.y, 40.0f, kEps);

    auto rotated = ComputeWindowFrame({100.0f, 100.0f}, 60.0f, 40.0f,
                                      static_cast<float>(M_PI) / 4.0f, 1.0f, 1000.0f);
    EXPECT_GT(rotated.size.x, f.size.x) << "rotation must grow the window";
    EXPECT_GT(rotated.size.y, f.size.y);
}

TEST(ItemWindowLogic, ScreenOriginIsFlippedFromDeviceSpace) {
    // DEVICE is Y-down from the top; SCREEN is Y-up from the bottom. An item
    // near the TOP in device space must yield a HIGH screen y.
    constexpr float screenH = 1000.0f;
    auto top = ComputeWindowFrame({100.0f, 0.0f}, 40.0f, 40.0f, 0.0f, 1.0f, screenH);
    auto bottom = ComputeWindowFrame({100.0f, 900.0f}, 40.0f, 40.0f, 0.0f, 1.0f, screenH);

    EXPECT_GT(top.origin.y, bottom.origin.y)
        << "a device-top item must map to a larger screen y than a device-bottom one";
}

TEST(ItemWindowLogic, HorizontalPositionIsUnaffectedByTheFlip) {
    auto left = ComputeWindowFrame({0.0f, 500.0f}, 40.0f, 40.0f, 0.0f, 1.0f, 1000.0f);
    auto right = ComputeWindowFrame({800.0f, 500.0f}, 40.0f, 40.0f, 0.0f, 1.0f, 1000.0f);
    EXPECT_LT(left.origin.x, right.origin.x);
    EXPECT_NEAR(right.origin.x - left.origin.x, 800.0f, 0.5f)
        << "x should translate one-for-one";
}

TEST(ItemWindowLogic, ScaleGrowsTheWindowAroundTheItem) {
    auto small = ComputeWindowFrame({500.0f, 500.0f}, 50.0f, 50.0f, 0.0f, 1.0f, 1000.0f);
    auto large = ComputeWindowFrame({500.0f, 500.0f}, 50.0f, 50.0f, 0.0f, 2.0f, 1000.0f);
    EXPECT_NEAR(large.size.x, small.size.x * 2.0f, 0.5f);
    EXPECT_NEAR(large.size.y, small.size.y * 2.0f, 0.5f);
}

// ── Hit testing ────────────────────────────────────────────

TEST(ItemWindowLogic, LocalPointMapsToDeviceByItemOrigin) {
    DevicePoint d = LocalViewPointToDevice({100.0f, 200.0f}, {10.0f, 20.0f});
    EXPECT_NEAR(d.x, 110.0f, kEps);
    EXPECT_NEAR(d.y, 220.0f, kEps);
}

TEST(ItemWindowLogic, PointAtItemCentreIsInside) {
    // Local coords run from the item's top-left, so the centre of an
    // unrotated 100x60 item is at (50,30).
    EXPECT_TRUE(IsLocalPointInsideItem({200.0f, 300.0f}, {50.0f, 30.0f},
                                       100.0f, 60.0f, 0.0f, 1.0f));
}

TEST(ItemWindowLogic, PointFarOutsideIsRejected) {
    EXPECT_FALSE(IsLocalPointInsideItem({200.0f, 300.0f}, {5000.0f, 5000.0f},
                                        100.0f, 60.0f, 0.0f, 1.0f));
}

TEST(ItemWindowLogic, RotationChangesWhichCornersAreInside) {
    // The corner of the bounding box of a 45-degree-rotated square is outside
    // the square itself — this is exactly what makes click-through work.
    const DevicePoint itemPos{200.0f, 200.0f};
    const DevicePoint corner{2.0f, 2.0f};  // near the item's top-left corner

    EXPECT_TRUE(IsLocalPointInsideItem(itemPos, corner, 100.0f, 100.0f, 0.0f, 1.0f))
        << "unrotated: the corner is inside the square";
    EXPECT_FALSE(IsLocalPointInsideItem(itemPos, corner, 100.0f, 100.0f,
                                        static_cast<float>(M_PI) / 4.0f, 1.0f))
        << "rotated 45 degrees: that same corner falls outside the diamond";
}

// ── Sync decisions ─────────────────────────────────────────

TEST(ItemWindowLogic, MissingWindowForAValidItemMeansCreate) {
    EXPECT_EQ(DecideSyncAction(/*hasWindow=*/false, /*itemValid=*/true, false, false),
              SyncAction::Create);
}

TEST(ItemWindowLogic, WindowForADeadItemMeansDestroy) {
    EXPECT_EQ(DecideSyncAction(true, /*itemValid=*/false, false, false),
              SyncAction::Destroy);
    EXPECT_EQ(DecideSyncAction(true, false, /*cursorInside=*/true, true),
              SyncAction::Destroy)
        << "an invalid item is torn down regardless of cursor state";
}

TEST(ItemWindowLogic, NoWindowAndNoItemIsNothingToDo) {
    EXPECT_EQ(DecideSyncAction(false, false, false, false), SyncAction::None);
}

TEST(ItemWindowLogic, CursorEnteringMakesTheWindowInteractive) {
    EXPECT_EQ(DecideSyncAction(true, true, /*cursorInside=*/true,
                               /*currentlyInteractive=*/false),
              SyncAction::SetInteractive);
}

TEST(ItemWindowLogic, CursorLeavingRestoresClickThrough) {
    EXPECT_EQ(DecideSyncAction(true, true, /*cursorInside=*/false,
                               /*currentlyInteractive=*/true),
              SyncAction::SetClickThrough);
}

TEST(ItemWindowLogic, AlreadyInTheRightStateReportsNoChange) {
    // This is what keeps syncWindows from thrashing ignoresMouseEvents every
    // frame, which previously caused repeated window reordering.
    EXPECT_EQ(DecideSyncAction(true, true, true, true), SyncAction::None);
    EXPECT_EQ(DecideSyncAction(true, true, false, false), SyncAction::None);
}
