// test_goose_window_size.mm — CalculateGooseWindowSize (src/platform/macos/window.mm).
//
// window.mm was at 0%: it is a platform file, but this function is pure enough
// to drive directly given a Goose, so it needs no window and no extraction.
//
// The function sizes the transparent per-goose window. It must always be big
// enough to contain the goose AND anything it is carrying, and it quantizes the
// result to a 48px grid so a slowly-rotating held item does not force an
// NSWindow resize on every single frame.

#include <gtest/gtest.h>
#include <cmath>

#include "window.h"
#include "goose.h"
#include "items.h"
#include "config.h"
#include "actor_manager.h"
#include "world.h"
#include "world_coord.h"

namespace {

constexpr float kBaseSize = 600.0f;
constexpr float kGrid = 48.0f;

class GooseWindowSizeTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_savedScale = g_config.general.globalScale;
        g_config.general.globalScale = 1.0f;
        m_goose = new Goose(1, "Gander", 1920, 1080);
        m_goose->pos = {500.0f, 500.0f};
        // Tick once so the rig is positioned relative to pos. A freshly
        // constructed goose has its rig at the origin, which makes the
        // pos->beak distance ~707px and swamps every other term in the sizing.
        WorldContext world{};
        world.screenWidth = 1920;
        world.screenHeight = 1080;
        m_goose->tick(world, 1.0 / 60.0, 0.016);
    }

    void TearDown() override {
        m_goose->heldItem = nullptr;
        delete m_goose;
        g_config.general.globalScale = m_savedScale;
    }

    // A held item of the given size, owned by the fixture.
    ItemData* holdItem(float w, float h) {
        m_item.type = ItemData::MEME;
        m_item.w = w;
        m_item.h = h;
        m_goose->heldItem = &m_item;
        return &m_item;
    }

    Goose* m_goose = nullptr;
    ItemData m_item{};
    float m_savedScale = 1.0f;
};

}  // namespace

TEST_F(GooseWindowSizeTest, EmptyBeakedGooseGetsTheBaseSize) {
    EXPECT_FLOAT_EQ(CalculateGooseWindowSize(m_goose), kBaseSize);
}

TEST_F(GooseWindowSizeTest, NullGooseGetsTheBaseSize) {
    EXPECT_FLOAT_EQ(CalculateGooseWindowSize(nullptr), kBaseSize)
        << "a null goose must not crash or return a degenerate size";
}

TEST_F(GooseWindowSizeTest, SmallHeldItemStillFitsInTheBaseSize) {
    holdItem(20.0f, 20.0f);
    EXPECT_FLOAT_EQ(CalculateGooseWindowSize(m_goose), kBaseSize)
        << "a small item should not force the window to grow";
}

TEST_F(GooseWindowSizeTest, LargeHeldItemGrowsTheWindow) {
    holdItem(20.0f, 20.0f);
    const float small = CalculateGooseWindowSize(m_goose);

    holdItem(800.0f, 800.0f);
    const float large = CalculateGooseWindowSize(m_goose);

    EXPECT_GT(large, small);
    EXPECT_GT(large, kBaseSize) << "an item larger than the base window must expand it";
}

TEST_F(GooseWindowSizeTest, GrownSizeIsQuantizedToTheGrid) {
    holdItem(800.0f, 600.0f);
    const float size = CalculateGooseWindowSize(m_goose);

    ASSERT_GT(size, kBaseSize);
    const float remainder = std::fmod(size, kGrid);
    EXPECT_TRUE(remainder < 0.01f || std::abs(remainder - kGrid) < 0.01f)
        << "size " << size << " is not a multiple of the " << kGrid << "px grid";
}

TEST_F(GooseWindowSizeTest, QuantizationIsStableAcrossTinyItemChanges) {
    // This is the point of the grid: the held item's extent drifts a little
    // every frame, and an exact size would mean a full NSWindow resize each
    // time. Neighbouring sizes must collapse to the same quantized value.
    holdItem(800.0f, 600.0f);
    const float a = CalculateGooseWindowSize(m_goose);

    holdItem(800.5f, 600.3f);
    const float b = CalculateGooseWindowSize(m_goose);

    EXPECT_FLOAT_EQ(a, b)
        << "sub-grid item drift must not change the window size (a=" << a
        << " b=" << b << ")";
}

TEST_F(GooseWindowSizeTest, QuantizationRoundsUpNeverDown) {
    holdItem(800.0f, 600.0f);
    const float size = CalculateGooseWindowSize(m_goose);

    // Whatever the exact requirement was, the returned size must be >= it.
    // Growing the item slightly must never shrink the window below the grid
    // step it already occupied.
    holdItem(810.0f, 600.0f);
    EXPECT_GE(CalculateGooseWindowSize(m_goose), size)
        << "a bigger item must never produce a smaller window";
}

TEST_F(GooseWindowSizeTest, RotationDoesNotChangeTheExtent) {
    // The held-item extent is the item's half-diagonal, which is
    // rotation-invariant: no rotation angle can place a corner of the item
    // farther from the item's centre than half its diagonal. The window size
    // therefore must not respond to dragRot at all.
    holdItem(900.0f, 60.0f);

    m_goose->dragRot = 0.0f;
    const float unrotated = CalculateGooseWindowSize(m_goose);

    for (int deg = 15; deg < 360; deg += 15) {
        m_goose->dragRot = deg * static_cast<float>(M_PI) / 180.0f;
        EXPECT_FLOAT_EQ(CalculateGooseWindowSize(m_goose), unrotated)
            << "window size changed at " << deg
            << " degrees — extent is no longer rotation-invariant";
    }
}

// The sizing must meet the TRUE bound: half the item diagonal.
//
// History: the extent was max(rotatedAABB.x, rotatedAABB.y) * 0.5, which
// SHRINKS as a long thin item rotates toward 45 degrees (900x60 drops from
// 450 to ~339) while the real requirement stays ~451 — rotating a long item
// made its window smaller and could clip it. Fixed to use the
// rotation-invariant half-diagonal; this test proves the bound is met at
// every angle, including the worst case (45 degrees for long thin items).
TEST_F(GooseWindowSizeTest, ExtentMeetsTheHalfDiagonalBoundAtEveryAngle) {
    constexpr float w = 900.0f, h = 60.0f;
    const float halfDiagonal = std::sqrt(w * w + h * h) * 0.5f;

    holdItem(w, h);

    // distToBeak + itemBehindBeak + padding are rotation-invariant terms; if
    // the window size is constant across angles (previous test), checking it
    // at 45 degrees against the full half-diagonal bound covers all of them.
    m_goose->dragRot = static_cast<float>(M_PI) / 4.0f;
    const float size = CalculateGooseWindowSize(m_goose);

    // size >= 2 * (distToBeak + itemW + beakOffset + halfDiag + padding),
    // quantized UP to the grid — so it can never be below the exact bound.
    Vector2 neckHeadDev = WorldCoord::RigNeckHead(*m_goose).toVector2();
    const float distToBeak = Vector2::Distance({m_goose->pos.x, m_goose->pos.y}, neckHeadDev);
    const float exactBound =
        2.0f * (distToBeak + w + 5.0f + halfDiagonal + 40.0f);

    EXPECT_GE(size, exactBound)
        << "window size " << size << " is below the exact requirement " << exactBound;
}

TEST_F(GooseWindowSizeTest, GlobalScaleFeedsIntoTheHeldItemExtent) {
    holdItem(400.0f, 400.0f);

    g_config.general.globalScale = 1.0f;
    const float atOne = CalculateGooseWindowSize(m_goose);

    g_config.general.globalScale = 3.0f;
    const float atThree = CalculateGooseWindowSize(m_goose);

    EXPECT_GT(atThree, atOne)
        << "globalScale scales the held item, so the window must follow";
}

TEST_F(GooseWindowSizeTest, SizeIsNeverBelowTheBaseForAnyItem) {
    for (float w : {1.0f, 50.0f, 300.0f, 1200.0f}) {
        for (float h : {1.0f, 50.0f, 300.0f, 1200.0f}) {
            holdItem(w, h);
            EXPECT_GE(CalculateGooseWindowSize(m_goose), kBaseSize)
                << "item " << w << "x" << h << " produced an undersized window";
        }
    }
}
