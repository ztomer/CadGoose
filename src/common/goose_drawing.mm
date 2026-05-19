// ===========================
// goose_drawing.cpp
// Modular goose rendering
// ===========================
#include "goose.h"
#include "goose_drawing.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "behavior.h"
#include "item_renderer.h"
#include "render_colors.h"
#include <cmath>

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

// --- Drawing constants ---
static constexpr float kOutlineWidthAdd = 2.0f;
static constexpr float kEyeLineWidth = 1.5f;
static constexpr float kSurpriseMarkLineSize = 4.0f;
static constexpr float kSurpriseMarkLineWidth = 3.0f;
static constexpr float kSurpriseMarkOffsetY = 25.0f;
static constexpr float kSurpriseMarkLineOffset = 3.0f;
static constexpr float kSurpriseMarkDotOffset = 4.0f;
static constexpr float kSurpriseMarkDotRadius = 2.0f;
static constexpr float kHeldItemBeakOffset = 5.0f;
static constexpr float kTextItemPadding = 5.0f;
static constexpr float kTextItemFontSize = 10.0f;
static constexpr float kAILabelFontSize = 8.0f;
static constexpr float kAILabelPadding = 4.0f;
static constexpr float kAILabelY = 2.0f;
static constexpr float kLeafSizeBase = 5.0f;
static constexpr float kLeafSizeScale = 5.0f;
static constexpr float kLeafZScale = 900.0f;
static constexpr float kLeafScaleFactor = 2.0f;
static constexpr float kLeafHeightFactor = 0.6f;
static constexpr float kLeafFadeStart = 8.0f;
static constexpr float kLeafFadeDuration = 2.0f;
static constexpr int kLeafColorCount = 4;
static constexpr float kDebugOverlayBoxSize = 20.0f;

static void DrawEllipse(CGContextRef ctx, Vector2 p, float rx, float ry, float r, float g, float b, float a) {
    CGContextSetRGBFillColor(ctx, r, g, b, a);
    CGContextFillEllipseInRect(ctx, CGRectMake(p.x - rx, p.y - ry, rx * 2, ry * 2));
}

static void DrawLine(CGContextRef ctx, Vector2 a, Vector2 b, float width, float r, float g, float bl, float al) {
    CGContextSetRGBStrokeColor(ctx, r, g, bl, al);
    CGContextSetLineWidth(ctx, width);
    CGContextSetLineCap(ctx, kCGLineCapRound);
    CGContextMoveToPoint(ctx, a.x, a.y);
    CGContextAddLineToPoint(ctx, b.x, b.y);
    CGContextStrokePath(ctx);
}

extern float Rainbow_GetHue(int gooseId);

void DrawGoose(Goose* g, CGContextRef ctx) {
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
    beakR = g_config.color.currentBeak.r; beakG = g_config.color.currentBeak.g; beakB = g_config.color.currentBeak.b;
    eyeR = g_config.color.currentEye.r; eyeG = g_config.color.currentEye.g; eyeB = g_config.color.currentEye.b;

    DrawEllipse(ctx, g->pos + Vector2{g_config.render.shadowOffsetX, g_config.render.shadowOffsetY},
                g_config.render.shadowWidth / 2, g_config.render.shadowHeight / 2,
                g_config.color.shadow.r, g_config.color.shadow.g, g_config.color.shadow.b, 0.3f);

    DrawEllipse(ctx, g->rig.lFoot.currentPos, g_config.render.footSize / 2, g_config.render.footSize / 2, beakR, beakG, beakB, 1.0f);
    DrawEllipse(ctx, g->rig.rFoot.currentPos, g_config.render.footSize / 2, g_config.render.footSize / 2, beakR, beakG, beakB, 1.0f);

    Vector2 bodyFront = g->rig.body + fwd * (g_config.render.bodyHeight / 2.0f);
    Vector2 bodyBack  = g->rig.body - fwd * (g_config.render.bodyHeight / 2.0f);
    Vector2 underFront = g->rig.underbody + fwd * (g_config.render.bodyHeight * 0.3f);
    Vector2 underBack  = g->rig.underbody - fwd * (g_config.render.bodyHeight * 0.3f);

    float headR, headG, headB;
    float neckR, neckG, neckB;
    float bodyColorR, bodyColorG, bodyColorB;
    float outlineR, outlineG, outlineB;

    if (g_config.behaviors.fun.rainbow) {
        float hue = Rainbow_GetHue(g->id);
        float r, b, c;
        HSV_to_RGB(hue, 1.0f, 0.85f, &r, &b, &c);
        bodyColorR = headR = neckR = r;
        bodyColorG = headG = neckG = b;
        bodyColorB = headB = neckB = c;
        outlineR = outlineG = outlineB = 0.3f;
    } else {
        headR = g_config.color.currentHead.r; headG = g_config.color.currentHead.g; headB = g_config.color.currentHead.b;
        neckR = g_config.color.currentNeck.r; neckG = g_config.color.currentNeck.g; neckB = g_config.color.currentNeck.b;
        bodyColorR = g_config.color.currentBody.r; bodyColorG = g_config.color.currentBody.g; bodyColorB = g_config.color.currentBody.b;
        outlineR = g_config.color.currentOutline.r; outlineG = g_config.color.currentOutline.g; outlineB = g_config.color.currentOutline.b;
    }

    // Anger tint: blend body colors toward red when goose is angry
    float angerLevel = Anger_GetLevel(g->id);
    if (angerLevel > g_config.behaviors.anger.minVisualThreshold) {
        float t = angerLevel / 100.0f;
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
        bodyColorR = lerp(bodyColorR, 1.0f, t * 0.5f);
        bodyColorG = lerp(bodyColorG, 0.1f, t * 0.5f);
        bodyColorB = lerp(bodyColorB, 0.0f, t * 0.5f);
        headR = lerp(headR, 1.0f, t * 0.4f);
        headG = lerp(headG, 0.1f, t * 0.4f);
        headB = lerp(headB, 0.0f, t * 0.4f);
        neckR = lerp(neckR, 1.0f, t * 0.4f);
        neckG = lerp(neckG, 0.1f, t * 0.4f);
        neckB = lerp(neckB, 0.0f, t * 0.4f);
    }

    DrawLine(ctx, bodyFront, bodyBack, g_config.render.bodyWidth + kOutlineWidthAdd, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, g_config.render.neckSize + kOutlineWidthAdd, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, g_config.render.head1Size + kOutlineWidthAdd, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, g_config.render.head2Size + kOutlineWidthAdd, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, underFront, underBack, g_config.render.bodyWidth - 7.0f, outlineR, outlineG, outlineB, 1.0f);

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, g->rig.body.x, g->rig.body.y);
    float squash = Lerp(1.0f, g_config.render.squashFactor, back);
    CGContextScaleCTM(ctx, 1.0f, squash);
    CGContextTranslateCTM(ctx, -g->rig.body.x, -g->rig.body.y);

    DrawLine(ctx, bodyFront, bodyBack, g_config.render.bodyWidth, bodyColorR, bodyColorG, bodyColorB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, g_config.render.neckSize, neckR, neckG, neckB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, g_config.render.head1Size, headR, headG, headB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, g_config.render.head2Size, headR, headG, headB, 1.0f);

    float beakW = std::min(g_config.render.beakWidth, g_config.render.beakMaxWidth);
    Vector2 beakBase = g->rig.neckHead + fwd * g_config.rig.beakBaseOffset;
    Vector2 beakTip = beakBase + fwd * g_config.rig.beakLen;

    const double kChewDuration = 0.4;
    const float kChewFrequencyHz = 10.0f;
    const float kChewMaxAmplitude = 6.0f;
    const float kBeakSplitThreshold = 0.5f;
    const float kSplitBeakWidthFactor = 0.7f;
    float beakOpen = 0.0f;
    if (g->isChewing) {
        double elapsed = g->lastUpdateTime - g->chewingStartTime;
        if (elapsed >= kChewDuration) {
            g->isChewing = false;
        } else {
            double phase = elapsed * kChewFrequencyHz * 2.0 * M_PI;
            float rawOsc = 0.5f * (1.0f - std::cos(phase));
            float decay = 1.0f - (float)(elapsed / kChewDuration);
            beakOpen = rawOsc * decay * kChewMaxAmplitude;
        }
    }

    if (beakOpen > kBeakSplitThreshold) {
        Vector2 perp = Vector2::Normalize(Vector2{-fwd.y, fwd.x});
        Vector2 upperTip = beakTip + perp * beakOpen;
        Vector2 lowerTip = beakTip - perp * beakOpen;
        DrawLine(ctx, beakBase, upperTip, beakW * kSplitBeakWidthFactor, beakR, beakG, beakB, 1.0f);
        DrawLine(ctx, beakBase, lowerTip, beakW * kSplitBeakWidthFactor, beakR, beakG, beakB, 1.0f);
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

    bool eyesClosed = g->isResting;

    if (back > g_config.render.eyeFacingThreshold) {
        if (eyesClosed) {
            DrawLine(ctx, eyeCenter - side * (g_config.render.eyeSize / 2.0f), eyeCenter + side * (g_config.render.eyeSize / 2.0f), kEyeLineWidth, eyeR, eyeG, eyeB, 1.0f);
        } else if (g->isSurprised) {
            DrawEllipse(ctx, eyeCenter, g_config.render.eyeSize * 0.8f, g_config.render.eyeSize * 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
            DrawEllipse(ctx, eyeCenter, g_config.render.eyeSize * 0.3f, g_config.render.eyeSize * 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            DrawEllipse(ctx, eyeCenter, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
        }
    } else {
        Vector2 lEye = eyeCenter - side * eyeSep;
        Vector2 rEye = eyeCenter + side * eyeSep;
        if (eyesClosed) {
            DrawLine(ctx, lEye - side * (g_config.render.eyeSize / 2.0f), lEye + side * (g_config.render.eyeSize / 2.0f), kEyeLineWidth, eyeR, eyeG, eyeB, 1.0f);
            DrawLine(ctx, rEye - side * (g_config.render.eyeSize / 2.0f), rEye + side * (g_config.render.eyeSize / 2.0f), kEyeLineWidth, eyeR, eyeG, eyeB, 1.0f);
        } else if (g->isSurprised) {
            DrawEllipse(ctx, lEye, g_config.render.eyeSize * 0.8f, g_config.render.eyeSize * 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
            DrawEllipse(ctx, lEye, g_config.render.eyeSize * 0.3f, g_config.render.eyeSize * 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
            DrawEllipse(ctx, rEye, g_config.render.eyeSize * 0.8f, g_config.render.eyeSize * 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
            DrawEllipse(ctx, rEye, g_config.render.eyeSize * 0.3f, g_config.render.eyeSize * 0.3f, 0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            DrawEllipse(ctx, lEye, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
            DrawEllipse(ctx, rEye, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
        }
    }

    // Surprise mark
    if (g->isSurprised) {
        Vector2 markPos = g->rig.neckHead + up * (-g_config.render.eyeOffsetY - kSurpriseMarkOffsetY);
        CGContextSetRGBFillColor(ctx, kSurpriseMarkR, kSurpriseMarkG, kSurpriseMarkB, 0.9f);
        CGContextSetRGBStrokeColor(ctx, 0.0f, 0.0f, 0.0f, 1.0f);
        CGContextSetLineWidth(ctx, kSurpriseMarkLineWidth);
        DrawLine(ctx, markPos, markPos + up * (-kSurpriseMarkLineSize * kSurpriseMarkLineOffset), kSurpriseMarkLineWidth, kSurpriseMarkR, kSurpriseMarkG, kSurpriseMarkB, 1.0f);
        DrawEllipse(ctx, markPos + up * (-kSurpriseMarkLineSize * kSurpriseMarkLineOffset - kSurpriseMarkDotOffset), kSurpriseMarkDotRadius, kSurpriseMarkDotRadius, kSurpriseMarkR, kSurpriseMarkG, kSurpriseMarkB, 1.0f);
    }

    if (g->heldItem && !facingBack) {
        DrawHeldItem(g, ctx);
    }

    CGContextRestoreGState(ctx);
}

void DrawHeldItem(Goose* g, CGContextRef ctx) {
    if (!g->heldItem) return;
    CGContextSaveGState(ctx);

    Vector2 beak = g->GetBeakTipDevice();
    CGContextTranslateCTM(ctx, beak.x, beak.y);
    float dragRad = g->dragRot;
    CGContextRotateCTM(ctx, -dragRad);
    
    // Scale item dimensions to device coords
    float itemW = g->heldItem->w * g_config.general.globalScale;
    float itemH = g->heldItem->h * g_config.general.globalScale;
    
    // Offset so the right edge of the item is at the beak, centered vertically
    CGContextTranslateCTM(ctx, -itemW - kHeldItemBeakOffset, -itemH / 2.0f);

    ItemRenderer* renderer = ItemRenderer::ForType(g->heldItem->type);
    renderer->DrawHeld(ctx, g->heldItem, itemW, itemH);

    CGContextRestoreGState(ctx);
}

void DrawFootprints(CGContextRef ctx, const RingBuffer<Footprint, kMaxFootprints>& footprints, double currentTime) {
    for (const auto& fp : footprints) {
        float age = (float)(currentTime - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        float alpha = std::max(0.0f, 1.0f - (age / life));
        if (alpha <= 0) continue;

        CGContextSetRGBFillColor(ctx, g_config.color.footprint.r, g_config.color.footprint.g, g_config.color.footprint.b, alpha * g_config.color.footprintAlphaMultiplier);

        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, fp.pos.x, fp.pos.y);
        CGContextRotateCTM(ctx, fp.dir);
        CGContextFillEllipseInRect(ctx, CGRectMake(-g_config.render.footprintWidth/2, -g_config.render.footprintHeight/2, g_config.render.footprintWidth, g_config.render.footprintHeight));
        CGContextRestoreGState(ctx);
    }
}

void DrawDroppedItem(CGContextRef ctx, const DroppedItem& item, float viewHeight) {
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, item.pos.x, item.pos.y);
    CGContextRotateCTM(ctx, -item.rotation);

    // Scale item dimensions to device coords
    float scale = g_config.general.globalScale;
    float itemW = item.data->w * scale;
    float itemH = item.data->h * scale;

    ItemRenderer* renderer = ItemRenderer::ForType(item.data->type);
    bool showClose = renderer->DrawDropped(ctx, item, itemW, itemH);

    if (showClose) {
        float closeX = -itemW / 2.0f;
        float closeY = -itemH / 2.0f;
        CGContextSetRGBFillColor(ctx, g_config.render.closeButtonColor.r,
                                 g_config.render.closeButtonColor.g,
                                 g_config.render.closeButtonColor.b, 0.8);
        CGContextFillRect(ctx, CGRectMake(closeX, closeY, g_config.render.closeButtonSize, g_config.render.closeButtonSize));
        CGContextSetRGBStrokeColor(ctx, g_config.render.closeButtonStroke.r,
                                   g_config.render.closeButtonStroke.g,
                                   g_config.render.closeButtonStroke.b, 1.0);
        CGContextSetLineWidth(ctx, 2.0);
        CGContextMoveToPoint(ctx, closeX + 4, closeY + 4);
        CGContextAddLineToPoint(ctx, closeX + g_config.render.closeButtonSize - 4, closeY + g_config.render.closeButtonSize - 4);
        CGContextMoveToPoint(ctx, closeX + g_config.render.closeButtonSize - 4, closeY + 4);
        CGContextAddLineToPoint(ctx, closeX + 4, closeY + g_config.render.closeButtonSize - 4);
        CGContextStrokePath(ctx);
    }

    CGContextRestoreGState(ctx);
}

void DrawDebugOverlay(CGContextRef ctx, const std::vector<Goose*>& geese) {
    if (!g_config.debug.visuals) return;

    CGContextSetRGBStrokeColor(ctx, 1, 0, 0, 1);
    CGContextSetLineWidth(ctx, 1);
    for (const auto* g : geese) {
        CGContextStrokeRect(ctx, CGRectMake(g->pos.x - kDebugOverlayBoxSize, g->pos.y - kDebugOverlayBoxSize, kDebugOverlayBoxSize * 2, kDebugOverlayBoxSize * 2));
    }
}