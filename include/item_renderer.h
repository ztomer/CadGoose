#pragma once

// Item rendering strategy pattern.
// Each item type (MEME, TEXT, TOY) has its own renderer implementation.
// Eliminates type-specific branching in goose_drawing.mm.

#include "items.h"

#ifdef __APPLE__
#import <CoreGraphics/CoreGraphics.h>
#endif

class ItemRenderer {
public:
    virtual ~ItemRenderer() = default;

    // Render item held by goose. Context is already translated to beak position.
    virtual void DrawHeld(CGContextRef ctx, const ItemData* item, float itemW, float itemH) = 0;

    // Render item dropped on ground. Context is already translated to item center.
    // Returns true if close button should be drawn (toys return false).
    virtual bool DrawDropped(CGContextRef ctx, const DroppedItem& item, float itemW, float itemH) = 0;

    // Factory: returns the correct renderer for an item type
    static ItemRenderer* ForType(ItemData::Type type);
};

class MemeItemRenderer : public ItemRenderer {
public:
    void DrawHeld(CGContextRef ctx, const ItemData* item, float itemW, float itemH) override;
    bool DrawDropped(CGContextRef ctx, const DroppedItem& item, float itemW, float itemH) override;
};

class TextItemRenderer : public ItemRenderer {
public:
    void DrawHeld(CGContextRef ctx, const ItemData* item, float itemW, float itemH) override;
    bool DrawDropped(CGContextRef ctx, const DroppedItem& item, float itemW, float itemH) override;
};

class ToyItemRenderer : public ItemRenderer {
public:
    void DrawHeld(CGContextRef ctx, const ItemData* item, float itemW, float itemH) override;
    bool DrawDropped(CGContextRef ctx, const DroppedItem& item, float itemW, float itemH) override;
};
