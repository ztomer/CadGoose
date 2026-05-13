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

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

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

    DrawLine(ctx, bodyFront, bodyBack, g_config.render.bodyWidth, bodyColorR, bodyColorG, bodyColorB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, g_config.render.neckSize, neckR, neckG, neckB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, g_config.render.head1Size, headR, headG, headB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, g_config.render.head2Size, headR, headG, headB, 1.0f);

    float beakW = std::min(g_config.render.beakWidth, g_config.render.beakMaxWidth);
    Vector2 beakBase = g->rig.neckHead + fwd * g_config.rig.beakBaseOffset;
    Vector2 beakTip = beakBase + fwd * g_config.rig.beakLen;
    DrawLine(ctx, beakBase, beakTip, beakW, beakR, beakG, beakB, 1.0f);

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
    
    // Offset so the right edge of the item is at the beak, centered vertically
    CGContextTranslateCTM(ctx, -g->heldItem->w - 5.0f, -g->heldItem->h / 2.0f);

    if (g->heldItem->type == ItemData::MEME && g->heldItem->image) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, 0, g->heldItem->h);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextDrawImage(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h), g->heldItem->image);
        CGContextRestoreGState(ctx);
    } else if (g->heldItem->type == ItemData::MEME) {
        CGContextSetRGBFillColor(ctx, 0.8f, 0.8f, 0.8f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h));
    } else if (g->heldItem->type == ItemData::TEXT) {
        CGContextSetRGBFillColor(ctx, 1, 1, 0.9f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h));
        CGContextSetRGBStrokeColor(ctx, 0, 0, 0, 1.0f);
        CGContextSetLineWidth(ctx, 2);
        CGContextStrokeRect(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h));

        if (g->heldItem->textContent) {
#ifdef __APPLE__
            NSString* text = [NSString stringWithUTF8String:g->heldItem->textContent->c_str()];
            NSDictionary* attrs = @{NSFontAttributeName: [NSFont systemFontOfSize:10.0],
                                    NSForegroundColorAttributeName: [NSColor blackColor]};
            float textX = 5;
            float textY = 5;
            float textW = g->heldItem->w - 10;
            float textH = g->heldItem->h - 10;
            [text drawInRect:NSMakeRect(textX, textY, textW, textH) withAttributes:attrs];
#endif
        }
    }

    CGContextRestoreGState(ctx);
}

void DrawFootprints(CGContextRef ctx, const std::list<Footprint>& footprints, double currentTime) {
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

void DrawLeaves(CGContextRef ctx, const std::list<LeafPile>& leafPiles, double currentTime) {
    if (!g_config.behaviors.fun.autumnLeaves) return;
#ifdef __APPLE__
    struct ColorRGB { float r, g, b; };
    ColorRGB colors[4] = {
        {208/255.0f, 122/255.0f, 45/255.0f},
        {234/255.0f, 198/255.0f, 54/255.0f},
        {172/255.0f, 193/255.0f, 79/255.0f},
        {208/255.0f, 87/255.0f,  64/255.0f}
    };

    for (const auto& pile : leafPiles) {
        float tKicked = pile.timeSinceKicked;
        float alpha = 1.0f;
        if (tKicked > 0.0f) {
            float age = currentTime - tKicked;
            if (age > 8.0f) {
                alpha = std::max(0.0f, 1.0f - (age - 8.0f) / 2.0f);
            }
        }
        if (alpha <= 0.0f) continue;

        for (int i = 0; i < pile.leaves.size(); i++) {
            const Leaf& leaf = pile.leaves[i];
            Vector2 p = leaf.GetScreenOffset(1.0f) + pile.pos;
            float sz = 5.0f + 5.0f * (leaf.curPosZ / 900.0f);
            sz *= 2.0f;
            ColorRGB c = colors[leaf.colorIndex % 4];
            CGContextSetRGBFillColor(ctx, c.r, c.g, c.b, alpha);
            CGContextFillEllipseInRect(ctx, CGRectMake(p.x - sz/2.0f, p.y - (sz*0.6f)/2.0f, sz, sz*0.6f));
        }
    }
#endif
}

void DrawDroppedItem(CGContextRef ctx, const DroppedItem& item, float viewHeight) {
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, item.pos.x, item.pos.y);
    CGContextRotateCTM(ctx, -item.rotation);

    float x = -item.data->w / 2.0f;
    float y = -item.data->h / 2.0f;

    if (item.data->type == ItemData::TEXT) {
        CGContextSetRGBFillColor(ctx, 1.0, 1.0, 0.8, 1.0);
        CGContextFillRect(ctx, CGRectMake(x, y, item.data->w, item.data->h));

#ifdef __APPLE__
        static NSDictionary* textAttrs = nil;
        if (!textAttrs) {
            textAttrs = @{
                NSFontAttributeName: [NSFont systemFontOfSize:g_config.render.textNoteFontSize],
                NSForegroundColorAttributeName: [NSColor blackColor]
            };
        }

        NSString* text = [NSString stringWithUTF8String:item.data->Text().c_str()];
        float textX = x + g_config.render.textNotePadding;
        float textY = y + g_config.render.textNotePadding;
        float textW = item.data->w - g_config.render.textNotePadding * 2;
        float textH = item.data->h - g_config.render.textNotePadding * 2;
        [text drawInRect:NSMakeRect(textX, textY, textW, textH) withAttributes:textAttrs];
#endif
    } else if (item.data->type == ItemData::MEME) {
        if (item.data->image) {
            CGContextSaveGState(ctx);
            CGContextTranslateCTM(ctx, x, y + item.data->h);
            CGContextScaleCTM(ctx, 1.0, -1.0);
            CGContextDrawImage(ctx, CGRectMake(0, 0, item.data->w, item.data->h), item.data->image);
            CGContextRestoreGState(ctx);
        } else {
            CGContextSetRGBFillColor(ctx, g_config.render.memePlaceholderColor.r,
                                     g_config.render.memePlaceholderColor.g,
                                     g_config.render.memePlaceholderColor.b, 1.0);
            CGContextFillRect(ctx, CGRectMake(x, y, item.data->w, item.data->h));
        }
    }

    float closeX = -item.data->w / 2.0f;
    float closeY = -item.data->h / 2.0f;
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

    CGContextRestoreGState(ctx);
}

void DrawDebugOverlay(CGContextRef ctx, const std::list<Goose>& geese) {
    if (!g_config.debug.visuals) return;

    CGContextSetRGBStrokeColor(ctx, 1, 0, 0, 1);
    CGContextSetLineWidth(ctx, 1);
    for (const auto& g : geese) {
        CGContextStrokeRect(ctx, CGRectMake(g.pos.x - 20, g.pos.y - 20, 40, 40));
    }
}