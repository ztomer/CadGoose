// ===========================
// renderer_goose_drawer.cpp
// Goose drawing functions
// ===========================
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <cmath>
#import <algorithm>

#include "goose.h"
#include "config.h"
#include "goose_math.h"

#ifdef __cplusplus
extern "C" {
#endif

void DrawEllipse(CGContextRef ctx, Vector2 p, float rx, float ry, float r, float g, float b, float a) {
    CGContextSetRGBFillColor(ctx, r, g, b, a);
    CGContextFillEllipseInRect(ctx, CGRectMake(p.x - rx, p.y - ry, rx * 2, ry * 2));
}

void DrawLine(CGContextRef ctx, Vector2 a, Vector2 b, float width, float r, float g, float bl, float al) {
    CGContextSetRGBStrokeColor(ctx, r, g, bl, al);
    CGContextSetLineWidth(ctx, width);
    CGContextSetLineCap(ctx, kCGLineCapRound);
    CGContextMoveToPoint(ctx, a.x, a.y);
    CGContextAddLineToPoint(ctx, b.x, b.y);
    CGContextStrokePath(ctx);
}

#ifdef __cplusplus
}
#endif

static bool IsDarkAppearance() {
    if (g_config.general.appearanceMode == APPEARANCE_DARK) return true;
    if (g_config.general.appearanceMode == APPEARANCE_LIGHT) return false;
    if (g_config.general.appearanceMode == APPEARANCE_CUSTOM) return false;
    return [[[NSApplication sharedApplication] effectiveAppearance] name] == NSAppearanceNameDarkAqua;
}

void Renderer_DrawGoose(Goose* g, CGContextRef ctx) {
    if (!std::isfinite(g->pos.x) || !std::isfinite(g->pos.y)) return;

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, g->pos.x, g->pos.y);
    CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
    CGContextTranslateCTM(ctx, -g->pos.x, -g->pos.y);

    Vector2 rawFwd = Vector2::FromAngleDegrees(g->dir);
    Vector2 fwd{ rawFwd.x * g->ISO_SCALE.x, rawFwd.y * g->ISO_SCALE.y };

    float facing = Dot(Vector2::Normalize(fwd), Vector2{0, 1});
    float back = Clamp(-facing, 0.0f, 1.0f);
    bool facingBack = (back > g_config.render.facingBackThreshold);

    float beakR, beakG, beakB;
    float eyeR, eyeG, eyeB;
    if (g_config.general.appearanceMode == APPEARANCE_CUSTOM) {
        beakR = g_config.color.customBeak.r; beakG = g_config.color.customBeak.g; beakB = g_config.color.customBeak.b;
        eyeR = g_config.color.customEye.r; eyeG = g_config.color.customEye.g; eyeB = g_config.color.customEye.b;
    } else if (IsDarkAppearance()) {
        beakR = g_config.color.canadaBeak.r; beakG = g_config.color.canadaBeak.g; beakB = g_config.color.canadaBeak.b;
        eyeR = g_config.color.canadaEye.r; eyeG = g_config.color.canadaEye.g; eyeB = g_config.color.canadaEye.b;
    } else {
        beakR = g_config.color.beak.r; beakG = g_config.color.beak.g; beakB = g_config.color.beak.b;
        eyeR = g_config.color.eye.r; eyeG = g_config.color.eye.g; eyeB = g_config.color.eye.b;
    }

    DrawEllipse(ctx, g->pos + Vector2{g_config.render.shadowOffsetX, g_config.render.shadowOffsetY},
                g_config.render.shadowWidth / 2, g_config.render.shadowHeight / 2,
                g_config.color.shadow.r, g_config.color.shadow.g, g_config.color.shadow.b, 0.3f);

    DrawEllipse(ctx, g->rig.lFoot.currentPos, g_config.render.footSize / 2, g_config.render.footSize / 2,
                beakR, beakG, beakB, 1.0f);
    DrawEllipse(ctx, g->rig.rFoot.currentPos, g_config.render.footSize / 2, g_config.render.footSize / 2,
                beakR, beakG, beakB, 1.0f);

    Vector2 bodyFront = g->rig.body + fwd * (g_config.render.bodyHeight / 2.0f);
    Vector2 bodyBack  = g->rig.body - fwd * (g_config.render.bodyHeight / 2.0f);
    Vector2 underFront = g->rig.underbody + fwd * (g_config.render.bodyHeight * 0.3f);
    Vector2 underBack  = g->rig.underbody - fwd * (g_config.render.bodyHeight * 0.3f);

    float headR, headG, headB;
    float neckR, neckG, neckB;
    float bodyR, bodyG, bodyB;
    float outlineR, outlineG, outlineB;

    if (g_config.behaviors.fun.rainbow && g_config.behaviors.fun.rainbow) {
        float hue = 0.0f;
        extern float Rainbow_GetHue(int gooseId);
        hue = Rainbow_GetHue(g->id);
        float r, g, b;
        HSV_to_RGB(hue, 1.0f, 0.85f, &r, &g, &b);
        bodyR = headR = neckR = r;
        bodyG = headG = neckG = g;
        bodyB = headB = neckB = b;
        outlineR = outlineG = outlineB = 0.3f;
    } else if (g_config.general.appearanceMode == APPEARANCE_CUSTOM) {
        headR = g_config.color.customHead.r; headG = g_config.color.customHead.g; headB = g_config.color.customHead.b;
        neckR = g_config.color.customNeck.r; neckG = g_config.color.customNeck.g; neckB = g_config.color.customNeck.b;
        bodyR = g_config.color.customBody.r; bodyG = g_config.color.customBody.g; bodyB = g_config.color.customBody.b;
        outlineR = g_config.color.customOutline.r; outlineG = g_config.color.customOutline.g; outlineB = g_config.color.customOutline.b;
    } else if (IsDarkAppearance()) {
        headR = g_config.color.canadaHead.r; headG = g_config.color.canadaHead.g; headB = g_config.color.canadaHead.b;
        neckR = g_config.color.canadaNeck.r; neckG = g_config.color.canadaNeck.g; neckB = g_config.color.canadaNeck.b;
        bodyR = g_config.color.canadaBody.r; bodyG = g_config.color.canadaBody.g; bodyB = g_config.color.canadaBody.b;
        outlineR = g_config.color.canadaOutline.r; outlineG = g_config.color.canadaOutline.g; outlineB = g_config.color.canadaOutline.b;
    } else {
        bodyR = g_config.color.goose.r; bodyG = g_config.color.goose.g; bodyB = g_config.color.goose.b;
        neckR = bodyR; neckG = bodyG; neckB = bodyB;
        headR = bodyR; headG = bodyG; headB = bodyB;
        outlineR = 0.82f; outlineG = 0.82f; outlineB = 0.82f;
    }

    DrawLine(ctx, bodyFront, bodyBack, g_config.render.bodyWidth + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, g_config.render.neckSize + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, g_config.render.head1Size + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, g_config.render.head2Size + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, underFront, underBack, g_config.render.bodyWidth - 7.0f, outlineR, outlineG, outlineB, 1.0f);

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, g->rig.body.x, g->rig.body.y);
    float squash = Lerp(1.0f, g_config.render.squashFactor, back);
    CGContextScaleCTM(ctx, 1.0f, squash);
    CGContextTranslateCTM(ctx, -g->rig.body.x, -g->rig.body.y);

    DrawLine(ctx, bodyFront, bodyBack, g_config.render.bodyWidth, bodyR, bodyG, bodyB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, g_config.render.neckSize, neckR, neckG, neckB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, g_config.render.head1Size, headR, headG, headB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, g_config.render.head2Size, headR, headG, headB, 1.0f);

    float beakW = std::min(g_config.render.beakWidth, g_config.render.beakMaxWidth);
    Vector2 beakBase = g->rig.neckHead + fwd * g_config.rig.beakBaseOffset;
    Vector2 beakTip = beakBase + fwd * g_config.rig.beakLen;

    const double kChewDuration = 0.4;
    float beakOpen = 0.0f;
    if (g->isChewing) {
        double elapsed = g->lastUpdateTime - g->chewingStartTime;
        if (elapsed >= kChewDuration) {
            g->isChewing = false;
        } else {
            double phase = elapsed * 10.0 * 2.0 * M_PI;
            float rawOsc = 0.5f * (1.0f - std::cos(phase));
            float decay = 1.0f - (float)(elapsed / kChewDuration);
            beakOpen = rawOsc * decay * 6.0f;
        }
    }

    if (beakOpen > 0.5f) {
        Vector2 perp = Vector2::Normalize(Vector2{-fwd.y, fwd.x});
        Vector2 upperTip = beakTip + perp * beakOpen;
        Vector2 lowerTip = beakTip - perp * beakOpen;
        DrawLine(ctx, beakBase, upperTip, beakW * 0.7f, beakR, beakG, beakB, 1.0f);
        DrawLine(ctx, beakBase, lowerTip, beakW * 0.7f, beakR, beakG, beakB, 1.0f);
    } else {
        DrawLine(ctx, beakBase, beakTip, beakW, beakR, beakG, beakB, 1.0f);
    }

    CGContextRestoreGState(ctx);

    Vector2 rawSide = Vector2::FromAngleDegrees(g->dir + 90.0f);
    Vector2 side{ rawSide.x * g->ISO_SCALE.x, rawSide.y * g->ISO_SCALE.y };
    Vector2 up{ 0, -1 };

    float eyeSep = Lerp(5.0f, g_config.render.eyeOffsetXFront, back);
    float eyeLift = Lerp(0.0f, 1.5f, back);
    Vector2 eyeCenter = g->rig.neckHead + up * (-g_config.render.eyeOffsetY + eyeLift);

    if (back > g_config.render.eyeFacingThreshold) {
        DrawEllipse(ctx, eyeCenter, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
    } else {
        DrawEllipse(ctx, eyeCenter - side * eyeSep, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
        DrawEllipse(ctx, eyeCenter + side * eyeSep, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
    }

    if (g->heldItem && !facingBack) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, g->rig.head2.x, g->rig.head2.y);
        CGContextRotateCTM(ctx, g->dir * (float)(M_PI / 180.0));
        CGContextSetRGBFillColor(ctx, g_config.color.droppedItem.r, g_config.color.droppedItem.g, g_config.color.droppedItem.b, 1.0f);
        CGContextFillEllipseInRect(ctx, CGRectMake(0, -5, 15, 10));
        CGContextRestoreGState(ctx);
    }

    CGContextRestoreGState(ctx);
}

void Renderer_DrawHeldItem(Goose* g, CGContextRef ctx) {
    if (!g->heldItem) return;
    Vector2 rawFwd = Vector2::FromAngleDegrees(g->dir);
    Vector2 fwd{ rawFwd.x * g->ISO_SCALE.x, rawFwd.y * g->ISO_SCALE.y };
    float facing = Dot(Vector2::Normalize(fwd), Vector2{0, 1});
    float back = Clamp(-facing, 0.0f, 1.0f);
    if (back > g_config.render.facingBackThreshold) return;

    Vector2 rawSide = Vector2::FromAngleDegrees(g->dir + 90.0f);
    Vector2 side{ rawSide.x * g->ISO_SCALE.x, rawSide.y * g->ISO_SCALE.y };
    Vector2 up{ 0, -1 };

    float eyeSep = Lerp(5.0f, g_config.render.eyeOffsetXFront, back);
    float eyeLift = Lerp(0.0f, 1.5f, back);
    Vector2 eyeCenter = g->rig.neckHead + up * (-g_config.render.eyeOffsetY + eyeLift);

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, eyeCenter.x, eyeCenter.y);
    CGContextRotateCTM(ctx, g->dir * (float)(M_PI / 180.0));

    CGContextSetRGBFillColor(ctx, g_config.color.eyeHighlight.r, g_config.color.eyeHighlight.g, g_config.color.eyeHighlight.b, g_config.color.eyeHighlight.a);
    CGContextFillEllipseInRect(ctx, CGRectMake(3, -4, 3, 3));

    CGContextRestoreGState(ctx);
}