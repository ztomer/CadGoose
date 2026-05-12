// ===========================
// goose_drawing.h
// Modular rendering functions
// ===========================
#ifndef GOOSE_DRAWING_H
#define GOOSE_DRAWING_H

#include "goose.h"
#include "world.h"

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif

void DrawGoose(Goose* g, CGContextRef ctx);
void DrawHeldItem(Goose* g, CGContextRef ctx);
void DrawFootprints(CGContextRef ctx, const std::list<Footprint>& footprints, double currentTime);
void DrawLeaves(CGContextRef ctx, const std::list<LeafPile>& leafPiles, double currentTime);
void DrawDroppedItem(CGContextRef ctx, const DroppedItem& item, float viewHeight);
void DrawDebugOverlay(CGContextRef ctx, const std::list<Goose>& geese);

// Anger tint - defined in behavior_anger.cpp
float Anger_GetLevel(int gooseId);

#endif