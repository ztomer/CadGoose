// test_cg_renderer.mm — CGRenderer (include/cg_renderer.h) driven against a real
// offscreen CGBitmapContext, plus the CGColor cache (include/cg_color_cache.h)
// and the shared color helpers (renderer_interface.h / render_colors.h).
//
// These paths were at 0% coverage: they are pure CoreGraphics and need no
// window, so they are fully exercisable headlessly. Assertions read back actual
// PIXELS wherever a draw call has an observable result — "it did not crash" is
// not evidence that a renderer drew anything.

#import <CoreGraphics/CoreGraphics.h>
#include <gtest/gtest.h>
#include <cmath>
#include <vector>

#include "cg_renderer.h"
#include "cg_color_cache.h"
#include "render_colors.h"

namespace {

constexpr int kW = 64;
constexpr int kH = 64;

// RGBA8 premultiplied canvas we can both draw into and read back.
class Canvas {
public:
    Canvas() : m_pixels(kW * kH * 4, 0) {
        m_cs = CGColorSpaceCreateDeviceRGB();
        m_ctx = CGBitmapContextCreate(m_pixels.data(), kW, kH, 8, kW * 4, m_cs,
                                      kCGImageAlphaPremultipliedLast);
    }
    ~Canvas() {
        if (m_ctx) CGContextRelease(m_ctx);
        if (m_cs) CGColorSpaceRelease(m_cs);
    }

    CGContextRef ctx() const { return m_ctx; }

    struct RGBA { int r, g, b, a; };

    // Reads a pixel in CG SPACE: (0,0) is the bottom-left, matching the
    // coordinates passed to the draw calls. CGBitmapContext stores rows
    // top-down, so the row flip here is what makes x/y line up with CG space —
    // it is not a convenience flip to top-down. (Getting this backwards is
    // invisible in any test whose geometry is vertically symmetric, which is
    // most of them; the transform tests below are what pin it down.)
    RGBA at(int x, int y) const {
        const int row = kH - 1 - y;
        const unsigned char* p = m_pixels.data() + (row * kW + x) * 4;
        return {p[0], p[1], p[2], p[3]};
    }

    bool anyNonZero() const {
        for (unsigned char v : m_pixels) {
            if (v != 0) return true;
        }
        return false;
    }

    void clear() { std::fill(m_pixels.begin(), m_pixels.end(), 0); }

private:
    std::vector<unsigned char> m_pixels;
    CGColorSpaceRef m_cs = nullptr;
    CGContextRef m_ctx = nullptr;
};

// A small opaque-red CGImage for the image-drawing paths.
CGImageRef MakeTestImage(int w, int h) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef c = CGBitmapContextCreate(nullptr, w, h, 8, 0, cs, kCGImageAlphaPremultipliedLast);
    CGContextSetRGBFillColor(c, 1.0, 0.0, 0.0, 1.0);
    CGContextFillRect(c, CGRectMake(0, 0, w, h));
    CGImageRef img = CGBitmapContextCreateImage(c);
    CGContextRelease(c);
    CGColorSpaceRelease(cs);
    return img;
}

}  // namespace

// ── Color cache ────────────────────────────────────────────

TEST(CGColorCache, PackRGBAQuantizesAndClamps) {
    // Below/above the unit range clamp rather than wrapping.
    EXPECT_EQ(cgcolorcache::PackRGBA(0.0f, 0.0f, 0.0f, 0.0f), 0x00000000u);
    EXPECT_EQ(cgcolorcache::PackRGBA(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffffu);
    EXPECT_EQ(cgcolorcache::PackRGBA(-5.0f, -0.1f, -0.0f, -1.0f), 0x00000000u);
    EXPECT_EQ(cgcolorcache::PackRGBA(2.0f, 9.0f, 1.5f, 4.0f), 0xffffffffu);

    // Channels land in the documented byte order (R<<24 | G<<16 | B<<8 | A).
    EXPECT_EQ(cgcolorcache::PackRGBA(1.0f, 0.0f, 0.0f, 0.0f), 0xff000000u);
    EXPECT_EQ(cgcolorcache::PackRGBA(0.0f, 1.0f, 0.0f, 0.0f), 0x00ff0000u);
    EXPECT_EQ(cgcolorcache::PackRGBA(0.0f, 0.0f, 1.0f, 0.0f), 0x0000ff00u);
    EXPECT_EQ(cgcolorcache::PackRGBA(0.0f, 0.0f, 0.0f, 1.0f), 0x000000ffu);
}

TEST(CGColorCache, RoundsToNearestByte) {
    // 0.5 -> 128 via the +0.5 rounding term, not truncation to 127.
    EXPECT_EQ((cgcolorcache::PackRGBA(0.5f, 0, 0, 0) >> 24) & 0xff, 128u);
    // Values inside one 1/255 step quantize together — this is what stops
    // continuous alpha fades from thrashing the cache.
    EXPECT_EQ(cgcolorcache::PackRGBA(0.5f, 0, 0, 0), cgcolorcache::PackRGBA(0.5015f, 0, 0, 0));
}

TEST(CGColorCache, DeviceRGBIsSharedAndStable) {
    CGColorSpaceRef a = cgcolorcache::DeviceRGB();
    CGColorSpaceRef b = cgcolorcache::DeviceRGB();
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a, b) << "DeviceRGB must hand back one shared immutable colorspace";
}

TEST(CGColorCache, LookupReturnsIdenticalPointerForSameColor) {
    CGColorRef c1 = cgcolorcache::Lookup(0.25f, 0.5f, 0.75f, 1.0f);
    CGColorRef c2 = cgcolorcache::Lookup(0.25f, 0.5f, 0.75f, 1.0f);
    ASSERT_NE(c1, nullptr);
    // Pointer identity is the entire point: it lets CoreGraphics coalesce its
    // display list without deep-comparing colors.
    EXPECT_EQ(c1, c2);
}

TEST(CGColorCache, LookupDistinguishesDifferentColors) {
    CGColorRef red = cgcolorcache::Lookup(1.0f, 0.0f, 0.0f, 1.0f);
    CGColorRef blue = cgcolorcache::Lookup(0.0f, 0.0f, 1.0f, 1.0f);
    EXPECT_NE(red, blue);
}

TEST(CGColorCache, LookupComponentsRoundTrip) {
    CGColorRef c = cgcolorcache::Lookup(1.0f, 0.5f, 0.0f, 1.0f);
    ASSERT_EQ(CGColorGetNumberOfComponents(c), 4u);
    const CGFloat* comps = CGColorGetComponents(c);
    EXPECT_NEAR(comps[0], 1.0, 0.01);
    EXPECT_NEAR(comps[1], 128.0 / 255.0, 0.01);
    EXPECT_NEAR(comps[2], 0.0, 0.01);
    EXPECT_NEAR(comps[3], 1.0, 0.01);
}

TEST(CGColorCache, EvictsViaRingOnceFullAndStaysUsable) {
    // Cache capacity is 64; walk well past it to drive the ring-eviction branch.
    for (int i = 0; i < 200; ++i) {
        CGColorRef c = cgcolorcache::Lookup(i / 255.0f, 0.0f, 0.0f, 1.0f);
        ASSERT_NE(c, nullptr) << "lookup " << i << " returned null after eviction";
    }
    // Still serving correct colors after the table wrapped.
    CGColorRef c = cgcolorcache::Lookup(0.0f, 1.0f, 0.0f, 1.0f);
    const CGFloat* comps = CGColorGetComponents(c);
    EXPECT_NEAR(comps[1], 1.0, 0.01);
}

TEST(CGColorCache, ContextSettersApplyTheColor) {
    Canvas canvas;
    CGCtx_SetFillColor(canvas.ctx(), 0.0f, 1.0f, 0.0f, 1.0f);
    CGContextFillRect(canvas.ctx(), CGRectMake(0, 0, kW, kH));
    Canvas::RGBA px = canvas.at(32, 32);
    EXPECT_EQ(px.g, 255);
    EXPECT_EQ(px.r, 0);

    CGCtx_SetStrokeColor(canvas.ctx(), 0.0f, 0.0f, 1.0f, 1.0f);
    CGContextSetLineWidth(canvas.ctx(), 8.0);
    CGContextStrokeRect(canvas.ctx(), CGRectMake(10, 10, 40, 40));
    // The stroke landed somewhere blue-dominant along the rect edge.
    bool sawBlue = false;
    for (int x = 0; x < kW && !sawBlue; ++x) {
        for (int y = 0; y < kH; ++y) {
            Canvas::RGBA p = canvas.at(x, y);
            if (p.b > 200 && p.g < 80) { sawBlue = true; break; }
        }
    }
    EXPECT_TRUE(sawBlue) << "stroke color from the cache never reached the canvas";
}

// ── CGRenderer primitives ──────────────────────────────────

TEST(CGRenderer, DrawRectFillsExactPixels) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.DrawRect({16, 16, 32, 32}, RenderColor::Red());

    EXPECT_EQ(canvas.at(32, 32).r, 255) << "center of the rect must be filled";
    EXPECT_EQ(canvas.at(2, 2).a, 0) << "outside the rect must stay untouched";
}

TEST(CGRenderer, DrawRectOutlineLeavesCenterEmpty) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.DrawRectOutline({16, 16, 32, 32}, RenderColor::Green(), 2.0f);

    EXPECT_EQ(canvas.at(32, 32).a, 0) << "an outline must not fill the interior";
    EXPECT_TRUE(canvas.anyNonZero()) << "outline drew nothing at all";
}

TEST(CGRenderer, DrawEllipseFillsCenterButNotCorner) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.DrawEllipse({32, 32}, 20, 20, RenderColor::Blue());

    EXPECT_EQ(canvas.at(32, 32).b, 255) << "ellipse center must be filled";
    EXPECT_EQ(canvas.at(0, 0).a, 0) << "ellipse must not reach the canvas corner";
}

TEST(CGRenderer, DrawEllipseOutlineLeavesCenterEmpty) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.DrawEllipseOutline({32, 32}, 20, 20, RenderColor::Yellow(), 3.0f);

    EXPECT_EQ(canvas.at(32, 32).a, 0);
    EXPECT_TRUE(canvas.anyNonZero());
}

TEST(CGRenderer, DrawLineMarksItsMidpoint) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.DrawLine({8, 32}, {56, 32}, RenderColor::White(), 4.0f);

    EXPECT_GT(canvas.at(32, 32).a, 0) << "a horizontal line must cover its midpoint";
    EXPECT_EQ(canvas.at(32, 4).a, 0) << "line must not bleed to the top of the canvas";
}

TEST(CGRenderer, DrawRoundedRectFillsCenterAndClipsRadius) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    // cornerRadius far larger than half the min dimension: exercises the clamp.
    r.DrawRoundedRect({16, 16, 32, 32}, 500.0f, RenderColor::Red());

    EXPECT_EQ(canvas.at(32, 32).r, 255) << "rounded rect center must be filled";
    // Clamped to a capsule/circle, so the true corner stays empty.
    EXPECT_EQ(canvas.at(17, 17).a, 0) << "radius clamp should round the corner off";
}

TEST(CGRenderer, DrawRoundedRectReusesCachedPathThenRebuilds) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    // Same geometry twice -> second call must hit the thread-local path cache.
    r.DrawRoundedRect({8, 8, 20, 20}, 4.0f, RenderColor::Green());
    r.DrawRoundedRect({8, 8, 20, 20}, 4.0f, RenderColor::Green());
    // Different geometry -> cache key misses and the path is rebuilt/released.
    r.DrawRoundedRect({32, 32, 24, 24}, 6.0f, RenderColor::Blue());

    EXPECT_EQ(canvas.at(18, 18).g, 255) << "first rounded rect missing";
    EXPECT_EQ(canvas.at(44, 44).b, 255) << "second rounded rect missing after cache miss";
}

TEST(CGRenderer, DrawPolygonFillsInteriorAndIgnoresDegenerate) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());

    // Fewer than 3 points is a no-op, not a crash or a stray mark.
    RenderPoint two[] = {{0, 0}, {10, 10}};
    r.DrawPolygon(two, 2, RenderColor::Red());
    EXPECT_FALSE(canvas.anyNonZero()) << "a 2-point polygon must draw nothing";

    RenderPoint tri[] = {{32, 8}, {56, 56}, {8, 56}};
    r.DrawPolygon(tri, 3, RenderColor::Red());
    EXPECT_EQ(canvas.at(32, 40).r, 255) << "triangle interior must be filled";
    EXPECT_EQ(canvas.at(2, 2).a, 0) << "triangle must not fill outside itself";
}

TEST(CGRenderer, SetAlphaAttenuatesSubsequentDraws) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.SetAlpha(0.5f);
    r.DrawRect({0, 0, kW, kH}, RenderColor::Red());

    int red = canvas.at(32, 32).r;
    EXPECT_GT(red, 100);
    EXPECT_LT(red, 200) << "alpha 0.5 should roughly halve the channel, got " << red;
}

// ── Transforms and state ───────────────────────────────────

TEST(CGRenderer, TranslateMovesDrawing) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.Translate(32, 32);
    r.DrawRect({0, 0, 16, 16}, RenderColor::Red());

    // The rect now occupies CG x/y 32..48 instead of 0..16.
    EXPECT_EQ(canvas.at(40, 40).r, 255) << "rect should be drawn at the translated origin";
    EXPECT_EQ(canvas.at(4, 4).a, 0) << "nothing should remain at the untranslated origin";
}

TEST(CGRenderer, ScaleEnlargesDrawing) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.Scale(4.0f, 4.0f);
    r.DrawRect({0, 0, 8, 8}, RenderColor::Green());

    // 8x8 scaled 4x covers CG 0..32 in both axes from the bottom-left origin.
    EXPECT_EQ(canvas.at(30, 30).g, 255) << "scaled rect must cover the enlarged area";
    EXPECT_EQ(canvas.at(40, 40).a, 0) << "scaled rect must stop at 32, not run past it";
}

TEST(CGRenderer, RotateChangesOrientation) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.Translate(32, 32);
    r.Rotate(static_cast<float>(M_PI) / 4.0f);
    r.DrawRect({-16, -2, 32, 4}, RenderColor::White());

    // A horizontal bar rotated 45 degrees about the center now runs along the
    // rising diagonal, so points at (32+d, 32+d) are on it and the flat
    // horizontal extremes are not.
    EXPECT_GT(canvas.at(32, 32).a, 0) << "rotated bar must still cross the center";
    EXPECT_GT(canvas.at(40, 40).a, 0) << "rotated bar must run along the diagonal";
    EXPECT_EQ(canvas.at(46, 32).a, 0) << "rotated bar must no longer lie horizontally";
}

TEST(CGRenderer, SaveAndRestoreStateUnwindsTransform) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.SaveState();
    r.Translate(40, 40);
    r.SetAlpha(0.0f);
    r.RestoreState();
    // After restore the translate and the alpha are both gone.
    r.DrawRect({0, 0, 16, 16}, RenderColor::Red());

    EXPECT_EQ(canvas.at(8, 8).r, 255) << "restore must undo the translate and the alpha";
    EXPECT_EQ(canvas.at(48, 48).a, 0) << "the saved translate must not have survived restore";
}

// ── Images ─────────────────────────────────────────────────

TEST(CGRenderer, DrawImageRendersIntoDestRect) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    CGImageRef img = MakeTestImage(8, 8);

    r.DrawImage(img, {16, 16, 32, 32});
    EXPECT_EQ(canvas.at(32, 32).r, 255) << "image pixels must land inside destRect";

    CGImageRelease(img);
}

TEST(CGRenderer, DrawImageIgnoresNull) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.DrawImage(nullptr, {0, 0, 32, 32});
    EXPECT_FALSE(canvas.anyNonZero()) << "a null image must be a no-op";
}

TEST(CGRenderer, GetImageSizeReportsDimensions) {
    CGImageRef img = MakeTestImage(12, 20);
    Canvas canvas;
    CGRenderer r(canvas.ctx());

    float w = -1, h = -1;
    EXPECT_TRUE(r.GetImageSize(img, &w, &h));
    EXPECT_FLOAT_EQ(w, 12.0f);
    EXPECT_FLOAT_EQ(h, 20.0f);

    // Null out-params are tolerated independently.
    EXPECT_TRUE(r.GetImageSize(img, nullptr, nullptr));
    float onlyW = -1;
    EXPECT_TRUE(r.GetImageSize(img, &onlyW, nullptr));
    EXPECT_FLOAT_EQ(onlyW, 12.0f);

    CGImageRelease(img);
}

TEST(CGRenderer, GetImageSizeRejectsNull) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    float w = -1, h = -1;
    EXPECT_FALSE(r.GetImageSize(nullptr, &w, &h));
    EXPECT_FLOAT_EQ(w, -1.0f) << "out-params must be left alone when the image is null";
}

// ── Text ───────────────────────────────────────────────────

TEST(CGRenderer, DrawTextMarksTheCanvas) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    r.DrawText("Hi", {8, 40}, RenderColor::White(), 24.0f);
    EXPECT_TRUE(canvas.anyNonZero()) << "DrawText produced no pixels at all";
}

TEST(CGRenderer, MeasureTextGrowsWithLengthAndSize) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());

    float shortW = r.MeasureText("i", 20.0f);
    float longW = r.MeasureText("wwwwww", 20.0f);
    EXPECT_GT(shortW, 0.0f);
    EXPECT_GT(longW, shortW) << "a longer string must measure wider";

    EXPECT_GT(r.MeasureText("abc", 40.0f), r.MeasureText("abc", 10.0f))
        << "a larger font must measure wider";
}

TEST(CGRenderer, MeasureTextHandlesEmptyAndNull) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    EXPECT_FLOAT_EQ(r.MeasureText("", 20.0f), 0.0f);
    EXPECT_FLOAT_EQ(r.MeasureText(nullptr, 20.0f), 0.0f);
}

TEST(CGRenderer, FontCacheQuantizesAndSurvivesEviction) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());

    // Sizes within the 0.5pt quantization step collapse to one cache entry.
    EXPECT_FLOAT_EQ(r.MeasureText("abc", 20.0f), r.MeasureText("abc", 20.1f));

    // The font cache holds 16 entries; walk past it to force the eviction branch.
    for (int i = 1; i <= 40; ++i) {
        float size = static_cast<float>(i) * 1.5f;
        EXPECT_GT(r.MeasureText("m", size), 0.0f) << "measure failed at size " << size;
    }
    // Still functional after eviction wrapped the table.
    EXPECT_GT(r.MeasureText("m", 20.0f), 0.0f);
}

// ── Interface plumbing ─────────────────────────────────────

TEST(CGRenderer, NativeContextReturnsTheBackingContext) {
    Canvas canvas;
    CGRenderer r(canvas.ctx());
    EXPECT_EQ(r.nativeContext(), (void*)canvas.ctx());
}

TEST(CGRenderer, UsableThroughTheAbstractInterface) {
    Canvas canvas;
    CGRenderer concrete(canvas.ctx());
    IRenderer& r = concrete;  // virtual dispatch through IRenderer

    r.SaveState();
    r.DrawRect({0, 0, kW, kH}, RenderColor::Blue());
    r.RestoreState();

    EXPECT_EQ(canvas.at(32, 32).b, 255) << "virtual dispatch must reach CGRenderer";
}

TEST(RenderColor, NamedConstructorsHaveExpectedChannels) {
    EXPECT_FLOAT_EQ(RenderColor::White().r, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::White().a, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::Black().r, 0.0f);
    EXPECT_FLOAT_EQ(RenderColor::Black().a, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::Red().r, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::Green().g, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::Blue().b, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::Yellow().r, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::Yellow().g, 1.0f);
    EXPECT_FLOAT_EQ(RenderColor::Clear().a, 0.0f);
}

TEST(RenderColors, PaletteHelpersCarryAlphaThrough) {
    EXPECT_FLOAT_EQ(MakeStickBrown().r, kStickBrownR);
    EXPECT_FLOAT_EQ(MakeStickBrown(0.25f).a, 0.25f);
    EXPECT_FLOAT_EQ(MakeToyBallRed().g, kToyBallRedG);
    EXPECT_FLOAT_EQ(MakeAIPaperCream().b, kAIPaperCreamB);
    EXPECT_FLOAT_EQ(MakeNametagBg().r, kNametagBgR);
    EXPECT_FLOAT_EQ(MakeHealthBarBg().g, kHealthBarBgG);
    EXPECT_FLOAT_EQ(MakeStemGreen().g, kStemGreenG);
    EXPECT_FLOAT_EQ(MakeFlowerCenter().b, kFlowerCenterB);
    EXPECT_FLOAT_EQ(MakeBreadcrumbGolden().r, kBreadcrumbGoldenR);
    EXPECT_FLOAT_EQ(MakeJailOrange().g, kJailOrangeG);
    EXPECT_FLOAT_EQ(MakePeekEyeSkin().r, kPeekEyeSkinR);
    EXPECT_FLOAT_EQ(MakeSurpriseMark(0.5f).a, 0.5f);
}
