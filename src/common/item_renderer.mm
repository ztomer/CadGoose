#include "item_renderer.h"
#include "config.h"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#endif

static constexpr float kHeldItemBeakOffset = 5.0f;
static constexpr float kTextItemPadding = 5.0f;
static constexpr float kTextItemFontSize = 10.0f;
static constexpr float kAILabelFontSize = 8.0f;
static constexpr float kAILabelPadding = 4.0f;
static constexpr float kAILabelY = 2.0f;

// ============================================================
// Factory
// ============================================================

ItemRenderer* ItemRenderer::ForType(ItemData::Type type) {
    static MemeItemRenderer memeRenderer;
    static TextItemRenderer textRenderer;
    static ToyItemRenderer toyRenderer;

    switch (type) {
        case ItemData::MEME: return &memeRenderer;
        case ItemData::TEXT: return &textRenderer;
        case ItemData::TOY:  return &toyRenderer;
    }
    return &memeRenderer;
}

// ============================================================
// MemeItemRenderer
// ============================================================

void MemeItemRenderer::DrawHeld(CGContextRef ctx, const ItemData* item, float itemW, float itemH) {
    if (item->image) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, 0, itemH);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextDrawImage(ctx, CGRectMake(0, 0, itemW, itemH), item->image);
        CGContextRestoreGState(ctx);
    } else {
        CGContextSetRGBFillColor(ctx, 0.8f, 0.8f, 0.8f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, itemW, itemH));
    }
}

bool MemeItemRenderer::DrawDropped(CGContextRef ctx, const DroppedItem& item, float itemW, float itemH) {
    float x = -itemW / 2.0f;
    float y = -itemH / 2.0f;

    if (item.data->image) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, x, y + itemH);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextDrawImage(ctx, CGRectMake(0, 0, itemW, itemH), item.data->image);
        CGContextRestoreGState(ctx);
    } else {
        CGContextSetRGBFillColor(ctx, g_config.render.memePlaceholderColor.r,
                                 g_config.render.memePlaceholderColor.g,
                                 g_config.render.memePlaceholderColor.b, 1.0);
        CGContextFillRect(ctx, CGRectMake(x, y, itemW, itemH));
    }
    return true; // Show close button
}

// ============================================================
// TextItemRenderer
// ============================================================

void TextItemRenderer::DrawHeld(CGContextRef ctx, const ItemData* item, float itemW, float itemH) {
    if (item->isAIGenerated) {
        CGContextSetRGBFillColor(ctx, 0.96f, 0.94f, 0.88f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, itemW, itemH));
        CGContextSetRGBStrokeColor(ctx, 0.6f, 0.5f, 0.4f, 1.0f);
        CGContextSetLineWidth(ctx, 1);
        CGContextStrokeRect(ctx, CGRectMake(0, 0, itemW, itemH));
    } else {
        CGContextSetRGBFillColor(ctx, 1, 1, 0.9f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, itemW, itemH));
        CGContextSetRGBStrokeColor(ctx, 0, 0, 0, 1.0f);
        CGContextSetLineWidth(ctx, 2);
        CGContextStrokeRect(ctx, CGRectMake(0, 0, itemW, itemH));
    }

    if (item->textContent) {
#ifdef __APPLE__
        NSString* text = [NSString stringWithUTF8String:item->textContent->c_str()];
        NSDictionary* attrs = @{NSFontAttributeName: [NSFont systemFontOfSize:kTextItemFontSize],
                                NSForegroundColorAttributeName: [NSColor blackColor]};
        float textX = kTextItemPadding;
        float textY = kTextItemPadding;
        float textW = itemW - kTextItemPadding * 2;
        float textH = itemH - kTextItemPadding * 2;
        [text drawInRect:NSMakeRect(textX, textY, textW, textH) withAttributes:attrs];
#endif
    }
}

bool TextItemRenderer::DrawDropped(CGContextRef ctx, const DroppedItem& item, float itemW, float itemH) {
    float x = -itemW / 2.0f;
    float y = -itemH / 2.0f;

    if (item.data->isAIGenerated) {
        CGContextSetRGBFillColor(ctx, 0.96f, 0.94f, 0.88f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(x, y, itemW, itemH));
        CGContextSetRGBStrokeColor(ctx, 0.6f, 0.5f, 0.4f, 1.0f);
        CGContextSetLineWidth(ctx, 1);
        CGContextStrokeRect(ctx, CGRectMake(x, y, itemW, itemH));
    } else {
        CGContextSetRGBFillColor(ctx, 1.0, 1.0, 0.8, 1.0);
        CGContextFillRect(ctx, CGRectMake(x, y, itemW, itemH));
    }

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
    float textW = itemW - g_config.render.textNotePadding * 2;
    float textH = itemH - g_config.render.textNotePadding * 2;
    [text drawInRect:NSMakeRect(textX, textY, textW, textH) withAttributes:textAttrs];

    if (item.data->isAIGenerated) {
        NSString* aiLabel = @"AI";
        NSDictionary* labelAttrs = @{
            NSFontAttributeName: [NSFont systemFontOfSize:kAILabelFontSize],
            NSForegroundColorAttributeName: [NSColor colorWithRed:0.5 green:0.4 blue:0.3 alpha:0.6]
        };
        CGSize labelSize = [aiLabel sizeWithAttributes:labelAttrs];
        float labelX = x + itemW - labelSize.width - kAILabelPadding;
        float labelY = y + kAILabelY;
        [aiLabel drawAtPoint:NSMakePoint(labelX, labelY) withAttributes:labelAttrs];
    }
#endif

    return true; // Show close button
}

// ============================================================
// ToyItemRenderer
// ============================================================

void ToyItemRenderer::DrawHeld(CGContextRef ctx, const ItemData* item, float itemW, float itemH) {
    if (item->image) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, 0, itemH);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextDrawImage(ctx, CGRectMake(0, 0, itemW, itemH), item->image);
        CGContextRestoreGState(ctx);
    } else {
        CGContextSetRGBFillColor(ctx, 0.55f, 0.35f, 0.15f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, itemW, itemH));
    }
}

bool ToyItemRenderer::DrawDropped(CGContextRef ctx, const DroppedItem& item, float itemW, float itemH) {
    float x = -itemW / 2.0f;
    float y = -itemH / 2.0f;

    if (item.data->image) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, x, y + itemH);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextDrawImage(ctx, CGRectMake(0, 0, itemW, itemH), item.data->image);
        CGContextRestoreGState(ctx);
    } else {
        CGContextSetRGBFillColor(ctx, 0.55f, 0.35f, 0.15f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(x, y, itemW, itemH));
    }
    return false; // No close button for toys
}
