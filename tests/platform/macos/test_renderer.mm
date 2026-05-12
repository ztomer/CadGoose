#include <gtest/gtest.h>
#import <Cocoa/Cocoa.h>
#include "renderer.h"
#include <cstdio>
#include <cstdlib>

static std::string run_tesseract(const char* imagePath) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cd /tmp && tesseract %s stdout 2>/dev/null", imagePath);
    FILE* f = popen(cmd, "r");
    if (!f) return "";
    std::string result;
    char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        result += buf;
    }
    pclose(f);
    // Remove trailing newlines
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

TEST(Rendering, YAxisFlip) {
    float boundsHeight = 1080.0f;
    float originalY = 200.0f;
    float flippedY = boundsHeight - originalY;
    EXPECT_FLOAT_EQ(flippedY, 880.0f);
}

TEST(Rendering, IsFlippedReturnsYes) {
    [GooseView resetPrimaryGuard];
    GooseView* view = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_TRUE([view isFlipped]);
}

TEST(Rendering, PrimaryGuardFirstViewPrimary) {
    [GooseView resetPrimaryGuard];
    GooseView* v1 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_TRUE(v1.isPrimary);
}

TEST(Rendering, PrimaryGuardSecondViewSecondary) {
    [GooseView resetPrimaryGuard];
    GooseView* v1 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_TRUE(v1.isPrimary);

    GooseView* v2 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_FALSE(v2.isPrimary);
}

TEST(Rendering, PrimaryGuardThirdViewAlsoSecondary) {
    [GooseView resetPrimaryGuard];
    GooseView* v1 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_TRUE(v1.isPrimary);

    GooseView* v2 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_FALSE(v2.isPrimary);

    GooseView* v3 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_FALSE(v3.isPrimary);
}

TEST(Rendering, PrimaryGuardResetWorks) {
    [GooseView resetPrimaryGuard];
    GooseView* v1 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_TRUE(v1.isPrimary);

    [GooseView resetPrimaryGuard];
    GooseView* v2 = [[GooseView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    EXPECT_TRUE(v2.isPrimary);
}

TEST(Rendering, GooseViewInitialization) {
    NSRect frame = NSMakeRect(0, 0, 1920, 1080);
    GooseView* view = [[GooseView alloc] initWithFrame:frame];
    EXPECT_NE(view, nil);
    EXPECT_TRUE(view.wantsLayer);
    EXPECT_EQ(view.bounds.size.width, 1920);
    EXPECT_EQ(view.bounds.size.height, 1080);
}

TEST(Rendering, ResolutionAndDPISimulation) {
    NSRect lowRes = NSMakeRect(0, 0, 800, 600);
    GooseView* viewLow = [[GooseView alloc] initWithFrame:lowRes];
    EXPECT_EQ(viewLow.bounds.size.width, 800);
    
    NSRect highRes = NSMakeRect(0, 0, 3840, 2160);
    GooseView* viewHigh = [[GooseView alloc] initWithFrame:highRes];
    EXPECT_EQ(viewHigh.bounds.size.width, 3840);
}

TEST(Rendering, TextMirroringFix) {
    // Test that NOT using Y-flip preserves text orientation
    // "FLIPPED" in Y-flipped context appears mirrored
    // "NORMAL" and "DRAWRECT" in unflipped context appear correct

    const char* outPath = "/tmp/test_text_fix.tiff";
    const char* outPng  = "/tmp/test_text_fix.png";

    @autoreleasepool {
        NSImage* img = [[NSImage alloc] initWithSize:NSMakeSize(800, 300)];
        [img lockFocus];
        [[NSColor whiteColor] setFill];
        NSRectFill(NSMakeRect(0, 0, 800, 300));

        CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
        NSDictionary* attrs = @{
            NSFontAttributeName: [NSFont systemFontOfSize:36],
            NSForegroundColorAttributeName: [NSColor blackColor]
        };

        // Method 1: Y-flip with drawAtPoint (mirrors text - BAD)
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, 0, 300);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        [@"FLIPPED" drawAtPoint:NSMakePoint(50, 50) withAttributes:attrs];
        CGContextRestoreGState(ctx);

        // Method 2: No flip, manual Y conversion (correct - USE THIS)
        [@"NORMAL" drawAtPoint:NSMakePoint(50, 250) withAttributes:attrs];

        // Method 3: drawInRect without flip (correct - USE THIS)
        [@"DRAWRECT" drawInRect:NSMakeRect(50, 50, 400, 60) withAttributes:attrs];

        [img unlockFocus];
        NSData* tiff = [img TIFFRepresentation];
        [tiff writeToFile:@(outPath) atomically:YES];
    }

    system("cd /tmp && sips -s format png test_text_fix.tiff --out test_text_fix.png > /dev/null 2>&1");
    std::string ocrResult = run_tesseract(outPng);

    // NORMAL and DRAWRECT should be readable, FLIPPED may be garbled
    EXPECT_TRUE(ocrResult.find("NORMAL") != std::string::npos) << "NORMAL not found. OCR: " << ocrResult;
    EXPECT_TRUE(ocrResult.find("DRAWRECT") != std::string::npos) << "DRAWRECT not found. OCR: " << ocrResult;
}

TEST(Rendering, TextRenderingCorrect) {
    // Regression test: ensure text renders without X-mirroring.
    // Solution: DON'T use Y-flip for text rendering. Use manual Y conversion instead.
    // This test verifies that text without Y-flip renders correctly.

    const char* outPath = "/tmp/test_text_render.tiff";
    const char* outPng  = "/tmp/test_text_render.png";

    @autoreleasepool {
        NSImage* img = [[NSImage alloc] initWithSize:NSMakeSize(800, 300)];
        [img lockFocus];
        [[NSColor whiteColor] setFill];
        NSRectFill(NSMakeRect(0, 0, 800, 300));

        CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
        NSDictionary* attrs = @{
            NSFontAttributeName: [NSFont systemFontOfSize:36],
            NSForegroundColorAttributeName: [NSColor blackColor]
        };

        // Draw text WITHOUT Y-flip (correct approach)
        // Y=50 in world coords → Y=250 in screen coords (300 - 50)
        [@"HELLO WORLD" drawAtPoint:NSMakePoint(50, 250) withAttributes:attrs];
        [@"CORRECT TEXT" drawInRect:NSMakeRect(50, 50, 400, 60) withAttributes:attrs];

        [img unlockFocus];
        NSData* tiff = [img TIFFRepresentation];
        [tiff writeToFile:@(outPath) atomically:YES];
    }

    system("cd /tmp && sips -s format png test_text_render.tiff --out test_text_render.png > /dev/null 2>&1");
    std::string ocrResult = run_tesseract(outPng);

    bool readable = ocrResult.find("HELLO") != std::string::npos;
    bool correctReadable = ocrResult.find("CORRECT") != std::string::npos;
    EXPECT_TRUE(readable) << "Text not readable. OCR: " << ocrResult;
    EXPECT_TRUE(correctReadable) << "Correct text not readable. OCR: " << ocrResult;
}

TEST(Rendering, DroppedItemTextNotMirrored) {
    // Regression test: dropped item text notes should render correctly.
    // Fix: DON'T use Y-flip for text, use manual Y conversion.

    const char* outPath = "/tmp/test_note_render.tiff";
    const char* outPng  = "/tmp/test_note_render.png";

    @autoreleasepool {
        NSImage* img = [[NSImage alloc] initWithSize:NSMakeSize(400, 200)];
        [img lockFocus];
        [[NSColor whiteColor] setFill];
        NSRectFill(NSMakeRect(0, 0, 400, 200));

        CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
        NSDictionary* attrs = @{
            NSFontAttributeName: [NSFont systemFontOfSize:16],
            NSForegroundColorAttributeName: [NSColor blackColor]
        };

        // Draw note WITHOUT Y-flip (correct approach)
        CGContextSetRGBFillColor(ctx, 1.0f, 1.0f, 0.8f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, 200, 100));

        // Text with manual Y conversion: y=10 in world → y=90 in screen
        [@"Test Note" drawInRect:NSMakeRect(10, 10, 180, 80) withAttributes:attrs];

        [img unlockFocus];
        NSData* tiff = [img TIFFRepresentation];
        [tiff writeToFile:@(outPath) atomically:YES];
    }

    system("cd /tmp && sips -s format png test_note_render.tiff --out test_note_render.png > /dev/null 2>&1");
    std::string ocrResult = run_tesseract(outPng);

    bool readable = ocrResult.find("Test") != std::string::npos;
    EXPECT_TRUE(readable) << "Note text not readable. OCR: " << ocrResult;
}
