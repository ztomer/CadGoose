#include "baby_stalin_actor.h"
#include "assets.h"
#include "config.h"
#include "goose_drawing.h"

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif

// ============================================================
// Drawing helpers
// ============================================================

static void DrawEllipseF(CGContextRef ctx, float cx, float cy, float rx, float ry, float r, float g, float b, float a) {
    CGContextSetRGBFillColor(ctx, r, g, b, a);
    CGContextFillEllipseInRect(ctx, CGRectMake(cx - rx, cy - ry, rx * 2, ry * 2));
}

static void DrawLineF(CGContextRef ctx, float x1, float y1, float x2, float y2, float w, float r, float g, float b, float a) {
    CGContextSetRGBStrokeColor(ctx, r, g, b, a);
    CGContextSetLineWidth(ctx, w);
    CGContextSetLineCap(ctx, kCGLineCapRound);
    CGContextMoveToPoint(ctx, x1, y1);
    CGContextAddLineToPoint(ctx, x2, y2);
    CGContextStrokePath(ctx);
}

// ============================================================
// BabyStalinActor
// ============================================================

static CGImageRef s_headImage = nullptr;

static CGImageRef GetHeadImage() {
    if (!s_headImage) {
        s_headImage = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/stalin_head.png");
    }
    return s_headImage;
}

BabyStalinActor::BabyStalinActor(int id_, const std::string& name_, int screenW, int screenH)
    : Goose(id_, name_, screenW, screenH) {
    m_canHonk = false;
}

#ifdef __APPLE__

static constexpr float kBabyBodyRX = 26.0f;
static constexpr float kBabyBodyRY = 32.0f;
static constexpr float kBabyHeadR = 36.0f;
static constexpr float kArmLen = 16.0f;
static constexpr float kArmW = 5.0f;
static constexpr float kEyeR = 3.0f;

void BabyStalinActor::drawBody(CGContextRef ctx) {
    if (!ctx) return;
    if (!std::isfinite(pos.x) || !std::isfinite(pos.y)) return;

    float globalScale = g_config.general.globalScale;

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, pos.x, pos.y);
    CGContextScaleCTM(ctx, globalScale, globalScale);
    CGContextTranslateCTM(ctx, -pos.x, -pos.y);

    // Shadow
    DrawEllipseF(ctx,
        pos.x + g_config.render.shadowOffsetX,
        pos.y + g_config.render.shadowOffsetY,
        g_config.render.shadowWidth / 2,
        g_config.render.shadowHeight / 2,
        g_config.color.shadow.r, g_config.color.shadow.g, g_config.color.shadow.b, 0.3f);

    // Feet (same rig as goose)
    float beakR = g_config.color.currentBeak.r;
    float beakG = g_config.color.currentBeak.g;
    float beakB = g_config.color.currentBeak.b;
    DrawEllipseF(ctx, rig.lFoot.currentPos.x, rig.lFoot.currentPos.y,
        g_config.render.footSize / 2, g_config.render.footSize / 2,
        beakR, beakG, beakB, 1.0f);
    DrawEllipseF(ctx, rig.rFoot.currentPos.x, rig.rFoot.currentPos.y,
        g_config.render.footSize / 2, g_config.render.footSize / 2,
        beakR, beakG, beakB, 1.0f);

    // Baby body — round ellipse
    float bodyCX = rig.body.x;
    float bodyCY = rig.body.y;
    DrawEllipseF(ctx, bodyCX, bodyCY, kBabyBodyRX, kBabyBodyRY,
        g_config.color.currentBody.r, g_config.color.currentBody.g, g_config.color.currentBody.b, 1.0f);

    // Arms at sides of body
    float armBaseY = bodyCY + kBabyBodyRY * 0.2f;
    DrawLineF(ctx, bodyCX - kBabyBodyRX - 2, armBaseY, bodyCX - kBabyBodyRX - 2 + kArmLen, armBaseY,
        kArmW, g_config.color.currentBody.r * 0.8f, g_config.color.currentBody.g * 0.8f, g_config.color.currentBody.b * 0.8f, 1.0f);
    DrawLineF(ctx, bodyCX + kBabyBodyRX + 2, armBaseY, bodyCX + kBabyBodyRX + 2 - kArmLen, armBaseY,
        kArmW, g_config.color.currentBody.r * 0.8f, g_config.color.currentBody.g * 0.8f, g_config.color.currentBody.b * 0.8f, 1.0f);

    // Head — photo texture clipped to circle, then rendered eyes on top
    float headCX = rig.neckHead.x;
    float headCY = rig.neckHead.y;
    float headDiam = kBabyHeadR * 2.0f;

    CGImageRef headImg = GetHeadImage();
    if (headImg) {
        // Maintain aspect ratio — scale so width fills circle, center vertically
        float imgW = (float)CGImageGetWidth(headImg);
        float imgH = (float)CGImageGetHeight(headImg);
        float scale = headDiam / imgW;
        float drawW = headDiam;
        float drawH = imgH * scale;
        float drawY = headCY - drawH / 2.0f;

        // Clip to circle
        CGContextSaveGState(ctx);
        CGContextAddArc(ctx, headCX, headCY, kBabyHeadR, 0, 2.0 * M_PI, 0);
        CGContextClip(ctx);

        // Flip Y for CGImage top-left origin
        CGContextTranslateCTM(ctx, headCX - drawW / 2.0f, drawY + drawH);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextDrawImage(ctx, CGRectMake(0, 0, drawW, drawH), headImg);
        CGContextRestoreGState(ctx);
    } else {
        // Fallback: solid color head
        DrawEllipseF(ctx, headCX, headCY, kBabyHeadR, kBabyHeadR,
            g_config.color.currentHead.r, g_config.color.currentHead.g, g_config.color.currentHead.b, 1.0f);
    }

    // Rendered eyes on top of photo head
    float eyeY = headCY - kBabyHeadR * 0.15f;
    float eyeOffX = kBabyHeadR * 0.35f;
    DrawEllipseF(ctx, headCX - eyeOffX, eyeY, kEyeR, kEyeR,
        g_config.color.currentEye.r, g_config.color.currentEye.g, g_config.color.currentEye.b, 1.0f);
    DrawEllipseF(ctx, headCX + eyeOffX, eyeY, kEyeR, kEyeR,
        g_config.color.currentEye.r, g_config.color.currentEye.g, g_config.color.currentEye.b, 1.0f);

    // Held item (same as goose — draw at beak tip)
    if (heldItem) {
        DrawHeldItem(this, ctx);
    }

    CGContextRestoreGState(ctx);
}

#endif
