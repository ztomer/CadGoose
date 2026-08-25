#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

#ifdef __APPLE__
#import <CoreGraphics/CoreGraphics.h>
#import <CoreFoundation/CoreFoundation.h>
#endif

#include "goose_drawing.h"
#include "cg_renderer.h"
#include "config.h"
#include "world.h"
#include "goose.h"
#include "behavior.h"

// ===========================
// Y-flip helpers
// ===========================
static int cgYToRow(int cgY, int height) {
    return height - 1 - cgY;
}

static int pixelR(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 1];
}
static int pixelG(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 2];
}
static int pixelB(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 3];
}
static int pixelA(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 0];
}



// ===========================
// DrawGoose tests
// ===========================

static Goose* createTestGoose() {
    Config_Init();
    g_config.general.globalScale = 1.0f;
    g_config.physics.isoScaleX = 1.0f;
    g_config.physics.isoScaleY = 1.0f;
    g_config.color.currentBody = {0.8f, 0.8f, 0.8f};
    g_config.color.currentHead = {0.7f, 0.7f, 0.7f};
    g_config.color.currentNeck = {0.75f, 0.75f, 0.75f};
    g_config.color.currentOutline = {0.2f, 0.2f, 0.2f};
    g_config.color.currentBeak = {1.0f, 0.5f, 0.0f};
    g_config.color.currentEye = {0.0f, 0.0f, 0.0f};
    g_config.color.shadow = {0.0f, 0.0f, 0.0f};
    g_config.render.footSize = 10.0f;
    g_config.render.shadowOffsetX = 5.0f;
    g_config.render.shadowOffsetY = -3.0f;
    g_config.render.shadowWidth = 30.0f;
    g_config.render.shadowHeight = 20.0f;
    g_config.render.bodyWidth = 12.0f;
    g_config.render.bodyHeight = 40.0f;
    g_config.render.neckSize = 8.0f;
    g_config.render.head1Size = 14.0f;
    g_config.render.head2Size = 10.0f;
    g_config.render.eyeSize = 8.0f;
    g_config.render.eyeOffsetXFront = 5.0f;
    g_config.render.eyeOffsetY = 12.0f;
    g_config.render.eyeFacingThreshold = 0.3f;
    g_config.render.facingBackThreshold = 0.5f;
    g_config.render.beakWidth = 6.0f;
    g_config.render.beakMaxWidth = 10.0f;
    g_config.render.squashFactor = 0.3f;
    g_config.rig.beakBaseOffset = 5.0f;
    g_config.rig.beakLen = 12.0f;

    Goose* g = new Goose(0, "test", 1920, 1080);
    g->dir = 0;
    g->pos = {250, 250};
    g->lastUpdateTime = 0;
    g->isChewing = false;
    g->isSurprised = false;
    g->isResting = false;
    g->heldItem = nullptr;
    // Compute rig and foot positions based on current pos/dir
    g->UpdateRig();
    g->SolveFeet(0);
    return g;
}

TEST(GooseRender, DrawGoose_Basic) {
    Goose* g = createTestGoose();
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    DrawGoose(g, ctx);

    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    // Body is drawn at rig.body position (pos + up * bodyY)
    // bodyWidth=12, so check slightly below center of the line
    int bx = (int)g->rig.body.x;
    int by = (int)g->rig.body.y;
    EXPECT_GT(pixelR(d, s, bx, by, 500), 100) << "body center at rig.body";
    EXPECT_GT(pixelG(d, s, bx, by, 500), 100);

    // Shadow at pos + shadowOffset
    int sx = (int)(g->pos.x + g_config.render.shadowOffsetX);
    int sy = (int)(g->pos.y + g_config.render.shadowOffsetY);
    EXPECT_EQ(pixelA(d, s, sx, sy, 500), 255) << "shadow should be visible at offset";

    // Far corner should be empty
    EXPECT_EQ(pixelA(d, s, 10, 10, 500), 0) << "far corner should be empty";

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, DrawGoose_Surprised) {
    Goose* g = createTestGoose();
    g->isSurprised = true;
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    DrawGoose(g, ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    // Surprise mark at rig.neckHead + up * (-eyeOffsetY - 25)
    // with up={0,-1}, eyeOffsetY=12 => markPos.y = neckHead.y + 37
    // Line drawn from markPos down to markPos+12, dot at markPos+16
    Vector2 markUp{0, -1};
    float kSurpriseMarkOffsetY = 25.0f;
    Vector2 markPos = g->rig.neckHead + markUp * (-g_config.render.eyeOffsetY - kSurpriseMarkOffsetY);
    int markX = (int)markPos.x;
    int markY = (int)markPos.y;
    EXPECT_GT(pixelR(d, s, markX, markY + 8, 500), 200) << "surprise mark should be visible below neckHead";

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, DrawGoose_Chewing) {
    Goose* g = createTestGoose();
    g->isChewing = true;
    g->chewingStartTime = 0.0;
    g->lastUpdateTime = 0.0; // elapsed=0, beakOpen=0 => not split yet

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    DrawGoose(g, ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    // Beak tip at neckHead + fwd * (beakBaseOffset + beakLen)
    Vector2 chewFwd = Vector2::FromAngleDegrees(g->dir);
    Vector2 chewFwd2{chewFwd.x * g->ISO_SCALE.x, chewFwd.y * g->ISO_SCALE.y};
    Vector2 beakTip = g->rig.neckHead + chewFwd2 * (g_config.rig.beakBaseOffset + g_config.rig.beakLen);
    EXPECT_GT(pixelR(d, s, (int)beakTip.x, (int)beakTip.y, 500), 200) << "beak tip should be orange";
    EXPECT_GT(pixelG(d, s, (int)beakTip.x, (int)beakTip.y, 500), 100);

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, DrawGoose_FacingRight) {
    Goose* g = createTestGoose();
    g->dir = 0;
    g->UpdateRig();
    g->SolveFeet(0);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    DrawGoose(g, ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    int bx = (int)g->rig.body.x;
    int by = (int)g->rig.body.y;
    EXPECT_GT(pixelR(d, s, bx, by, 500), 100) << "body center";
    // Body extends along +x (fwd={1,0}), so right of center should have body
    EXPECT_GT(pixelR(d, s, bx + 10, by, 500), 100) << "right of body center should have body";

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, DrawGoose_FacingLeft) {
    Goose* g = createTestGoose();
    g->dir = 180;
    g->UpdateRig();
    g->SolveFeet(0);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    DrawGoose(g, ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    int bx = (int)g->rig.body.x;
    int by = (int)g->rig.body.y;
    EXPECT_GT(pixelR(d, s, bx, by, 500), 100) << "body center";
    // Body extends along -x (fwd={-1,0}), so left of center should have body
    EXPECT_GT(pixelR(d, s, bx - 10, by, 500), 100) << "left of body center should have body";

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, DrawGoose_Resting) {
    Goose* g = createTestGoose();
    g->isResting = true;

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    DrawGoose(g, ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    int bx = (int)g->rig.body.x;
    int by = (int)g->rig.body.y;
    EXPECT_GT(pixelR(d, s, bx, by, 500), 100) << "body should still draw when resting";

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, DrawGoose_NanPos) {
    Goose* g = createTestGoose();
    g->pos = {NAN, NAN};

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 100, 100, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);

    DrawGoose(g, ctx);

    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);
    EXPECT_EQ(pixelA(d, (int)CGBitmapContextGetBytesPerRow(ctx), 50, 50, 100), 0) << "NaN pos should draw nothing";

    delete g;
    CGContextRelease(ctx);
}

// ===========================
// Googe::render() tests
// ===========================

TEST(GooseRender, GooseRender_CallsDrawGoose) {
    Goose* g = createTestGoose();
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    CGRenderer r(ctx);
    g->draw(&r);

    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);
    int bx = (int)g->rig.body.x;
    int by = (int)g->rig.body.y;
    EXPECT_GT(pixelR(d, s, bx, by, 500), 100) << "draw() should draw goose body";

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

// This case used to assert render() was a NO-OP ("cutover mode"). That design
// is gone: Goose::render() now delegates straight to draw() (goose.cpp), so the
// old expectation contradicted the shipped code. The file had been dropped from
// the build, so nothing caught the drift. Asserting the delegation instead.
TEST(GooseRender, Render_DelegatesToDraw) {
    Goose* g = createTestGoose();
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 500, 500, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    CGRenderer r(ctx);
    g->render(&r);

    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);
    int bx = (int)g->rig.body.x;
    int by = (int)g->rig.body.y;
    EXPECT_GT(pixelR(d, s, bx, by, 500), 100) << "render() should draw the goose body";

    delete g;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGBitmapContext_CreateImageAfterDraw) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 100, 100, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    CGContextSetRGBFillColor(ctx, 1, 0, 0, 1);
    CGContextFillRect(ctx, CGRectMake(10, 10, 30, 30));

    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);
    EXPECT_EQ(pixelR(d, s, 25, 25, 100), 255);

    CGImageRef img = CGBitmapContextCreateImage(ctx);
    CGDataProviderRef dp = CGImageGetDataProvider(img);
    CFDataRef imgData = CGDataProviderCopyData(dp);
    const uint8_t* ib = CFDataGetBytePtr(imgData);
    size_t ibpr = CGImageGetBytesPerRow(img);

    EXPECT_EQ(pixelR(ib, (int)ibpr, 25, 25, 100), 255) << "image should match";

    CFRelease(imgData);
    CGImageRelease(img);
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

// ===========================
// Multi-goose rendering tests
// ===========================

TEST(GooseRender, DrawThreeGeese_AllVisible) {
    g_config.general.globalScale = 1.0f;
    g_config.physics.isoScaleX = 1.0f;
    g_config.physics.isoScaleY = 1.0f;
    g_config.color.currentBody = {0.8f, 0.8f, 0.8f};
    g_config.color.currentHead = {0.7f, 0.7f, 0.7f};
    g_config.color.currentNeck = {0.75f, 0.75f, 0.75f};
    g_config.color.currentOutline = {0.2f, 0.2f, 0.2f};
    g_config.color.currentBeak = {1.0f, 0.5f, 0.0f};
    g_config.color.currentEye = {0.0f, 0.0f, 0.0f};
    g_config.render.footSize = 10.0f;
    g_config.render.shadowOffsetX = 5.0f;
    g_config.render.shadowOffsetY = -3.0f;
    g_config.render.shadowWidth = 30.0f;
    g_config.render.shadowHeight = 20.0f;
    g_config.render.bodyWidth = 12.0f;
    g_config.render.bodyHeight = 40.0f;
    g_config.render.neckSize = 8.0f;
    g_config.render.head1Size = 14.0f;
    g_config.render.head2Size = 10.0f;
    g_config.render.eyeSize = 8.0f;
    g_config.render.eyeOffsetXFront = 5.0f;
    g_config.render.eyeOffsetY = 12.0f;
    g_config.render.eyeFacingThreshold = 0.3f;
    g_config.render.facingBackThreshold = 0.5f;
    g_config.render.beakWidth = 6.0f;
    g_config.render.beakMaxWidth = 10.0f;
    g_config.render.squashFactor = 0.3f;
    g_config.rig.beakBaseOffset = 5.0f;
    g_config.rig.beakLen = 12.0f;

    // Create 3 geese at different positions
    Goose* g1 = new Goose(0, "Alpha", 1920, 1080);
    g1->dir = 0;
    g1->pos = {200, 400};
    g1->UpdateRig();
    g1->SolveFeet(0);

    Goose* g2 = new Goose(1, "Beta", 1920, 1080);
    g2->dir = 90;
    g2->pos = {400, 400};
    g2->UpdateRig();
    g2->SolveFeet(0);

    Goose* g3 = new Goose(2, "Gamma", 1920, 1080);
    g3->dir = 180;
    g3->pos = {600, 400};
    g3->UpdateRig();
    g3->SolveFeet(0);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    // Canvas big enough for all 3 geese
    CGContextRef ctx = CGBitmapContextCreate(NULL, 800, 600, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);

    // Draw all 3 geese into the same context
    DrawGoose(g1, ctx);
    DrawGoose(g2, ctx);
    DrawGoose(g3, ctx);

    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    // Verify each goose's body is visible at its rig.body position
    EXPECT_GT(pixelR(d, s, (int)g1->rig.body.x, (int)g1->rig.body.y, 600), 100)
        << "Goose 1 (Alpha) body visible";
    EXPECT_GT(pixelR(d, s, (int)g2->rig.body.x, (int)g2->rig.body.y, 600), 100)
        << "Goose 2 (Beta) body visible";
    EXPECT_GT(pixelR(d, s, (int)g3->rig.body.x, (int)g3->rig.body.y, 600), 100)
        << "Goose 3 (Gamma) body visible";

    // Verify goose 1 and 2 have different neckHead positions (different directions)
    float dist12 = Vector2::Distance(g1->rig.neckHead, g2->rig.neckHead);
    EXPECT_GT(dist12, 5.0f) << "Geese at different positions have different neckHead coords";

    delete g1;
    delete g2;
    delete g3;
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}
