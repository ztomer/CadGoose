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

TEST_F(GooseWindowSizeTest, RotationFeedsIntoTheExtent) {
    // Rotation is read by the sizing (via the rotated bounding box), so the
    // window size responds to it.
    holdItem(900.0f, 60.0f);

    m_goose->dragRot = 0.0f;
    const float unrotated = CalculateGooseWindowSize(m_goose);

    m_goose->dragRot = static_cast<float>(M_PI) / 4.0f;
    const float rotated = CalculateGooseWindowSize(m_goose);

    EXPECT_NE(rotated, unrotated) << "dragRot must reach the size computation";
}

// Documents a known conservativeness gap rather than asserting it is correct.
//
// The sizing uses max(rotatedAABB.x, rotatedAABB.y) * 0.5 as the item's extent
// from its own centre. The TRUE extent is half the diagonal, which does not
// change under rotation. For a long thin item the AABB's max dimension SHRINKS
// as it rotates toward 45 degrees (900x60 -> ~679x679), so the computed extent
// drops from 450 to ~339 while the real requirement stays ~451.
//
// This test pins the current behaviour so a future change is a deliberate one.
// Whether it visibly clips a rotated held item in the app is not established
// here — the window also carries 40px of padding and the item is drawn out at
// the beak, so the slack may absorb it.
TEST_F(GooseWindowSizeTest, RotatedLongItemExtentIsBelowTheHalfDiagonalBound) {
    constexpr float w = 900.0f, h = 60.0f;
    const float halfDiagonal = std::sqrt(w * w + h * h) * 0.5f;

    // What the implementation computes at 45 degrees.
    const float cosA = std::abs(std::cos(static_cast<float>(M_PI) / 4.0f));
    const float sinA = std::abs(std::sin(static_cast<float>(M_PI) / 4.0f));
    const float aabbX = w * cosA + h * sinA;
    const float aabbY = w * sinA + h * cosA;
    const float computedExtent = std::max(aabbX, aabbY) * 0.5f;

    EXPECT_LT(computedExtent, halfDiagonal)
        << "computed extent " << computedExtent << " vs true bound " << halfDiagonal;

    // And the resulting window is correspondingly smaller than the unrotated one.
    holdItem(w, h);
    m_goose->dragRot = 0.0f;
    const float unrotated = CalculateGooseWindowSize(m_goose);
    m_goose->dragRot = static_cast<float>(M_PI) / 4.0f;
    const float rotated = CalculateGooseWindowSize(m_goose);

    EXPECT_LT(rotated, unrotated)
        << "current behaviour: rotating a long thin item shrinks its window";
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
