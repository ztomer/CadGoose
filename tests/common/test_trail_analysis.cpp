// Headless tests of the trail detector's analysis (ROADMAP M1.3). The live
// trail_detection_test captures the screen and runs AnalyzeTrailFrames; these
// feed it synthetic frames, so the detector's verdict on "dropped items on
// screen" and on "a planted trail" is pinned without a display.
//
// The open question from 2026-06-22 was a false positive: frames containing
// dropped items counted as trails. These pin that it does not happen, and that
// a real single-frame trail still does register (a detector that can only say
// "clean" proves nothing).
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "../platform/macos/trail_analysis.h"

namespace {

constexpr int kW = 400;
constexpr int kH = 300;
constexpr int kFrames = 20;
constexpr int kDrop = 14;  // in the second half, where the analysis searches

struct BGRA { uint8_t b, g, r, a; };
constexpr BGRA kDesktop{40, 40, 40, 255};
constexpr BGRA kItemRed{0, 0, 230, 255};
constexpr BGRA kOldItem{0, 200, 0, 255};

CaptureResult Frame(BGRA fill = kDesktop) {
    CaptureResult f;
    f.width = kW;
    f.height = kH;
    f.pixels.resize((size_t)kW * kH * 4);
    for (size_t i = 0; i < f.pixels.size(); i += 4) {
        f.pixels[i + 0] = fill.b;
        f.pixels[i + 1] = fill.g;
        f.pixels[i + 2] = fill.r;
        f.pixels[i + 3] = fill.a;
    }
    return f;
}

void Rect(CaptureResult& f, int x0, int y0, int w, int h, BGRA c) {
    for (int y = y0; y < y0 + h; ++y) {
        for (int x = x0; x < x0 + w; ++x) {
            size_t i = (size_t)(y * f.width + x) * 4;
            f.pixels[i + 0] = c.b;
            f.pixels[i + 1] = c.g;
            f.pixels[i + 2] = c.r;
            f.pixels[i + 3] = c.a;
        }
    }
}

// A fetch run: earlier cycles' dropped items sit on screen throughout, and the
// new item (100x50, the test image's size) lands at kDrop and stays.
std::vector<CaptureResult> DropRun() {
    std::vector<CaptureResult> frames;
    for (int i = 0; i < kFrames; ++i) {
        CaptureResult f = Frame();
        Rect(f, 20, 20, 100, 50, kOldItem);   // dropped by an earlier cycle
        Rect(f, 260, 220, 100, 50, kOldItem);
        if (i >= kDrop) Rect(f, 150, 120, 100, 50, kItemRed);
        frames.push_back(std::move(f));
    }
    return frames;
}

}  // namespace

TEST(TrailAnalysis, DroppedItemsOnScreenAreNotTrails) {
    TrailReport r = AnalyzeTrailFrames(DropRun());
    ASSERT_TRUE(r.analyzed);
    EXPECT_EQ(r.dropFrame, kDrop);
    EXPECT_EQ(r.trailPixels, 0);
    EXPECT_EQ(r.trailClusters, 0);
    EXPECT_FALSE(r.trailDetected);
}

// An earlier item expiring (15 s lifecycle) in the drop frame changes A -> B
// and then stays gone: a disappearance, not a blink.
TEST(TrailAnalysis, ItemExpiringAtTheDropIsNotATrail) {
    auto frames = DropRun();
    for (int i = kDrop; i < kFrames; ++i) Rect(frames[i], 140, 60, 40, 40, kDesktop);
    for (int i = 0; i < kDrop; ++i) Rect(frames[i], 140, 60, 40, 40, kOldItem);
    TrailReport r = AnalyzeTrailFrames(frames);
    ASSERT_TRUE(r.analyzed);
    EXPECT_FALSE(r.trailDetected);
    EXPECT_EQ(r.trailPixels, 0);
}

// The bug the detector exists for: stale pixels composited for exactly one
// frame (the drop frame) beside the new item, gone the frame after.
TEST(TrailAnalysis, PlantedSingleFrameTrailIsDetected) {
    auto frames = DropRun();
    Rect(frames[kDrop], 150, 175, 60, 20, BGRA{230, 230, 230, 255});
    TrailReport r = AnalyzeTrailFrames(frames);
    ASSERT_TRUE(r.analyzed);
    EXPECT_TRUE(r.trailDetected);
    EXPECT_GE(r.trailClusters, 1);
    EXPECT_EQ(r.trailPixels, 60 * 20);
}

// The frames are BGRA: blue is byte 0 and alpha is byte 3. A trail that
// differs from the desktop only in blue must still register.
TEST(TrailAnalysis, BlueOnlyTrailIsDetected) {
    auto frames = DropRun();
    Rect(frames[kDrop], 150, 175, 60, 20, BGRA{240, 40, 40, 255});
    TrailReport r = AnalyzeTrailFrames(frames);
    ASSERT_TRUE(r.analyzed);
    EXPECT_TRUE(r.trailDetected);
}

TEST(TrailAnalysis, AlphaIsNotColour) {
    BGRA opaque{10, 20, 30, 255};
    BGRA clear{10, 20, 30, 0};
    EXPECT_EQ(PixelDelta(&opaque.b, &clear.b), 0.0f);
}
