// ===========================
// goose_drawing.h
// Modular rendering functions
// ===========================
#ifndef GOOSE_DRAWING_H
#define GOOSE_DRAWING_H

#include "goose.h"
#include "world.h"
#include <vector>

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif

void DrawGoose(Goose* g, CGContextRef ctx);
void DrawHeldItem(Goose* g, CGContextRef ctx);
void DrawFootprints(CGContextRef ctx, const RingBuffer<Footprint, kMaxFootprints>& footprints, double currentTime);
void DrawDroppedItem(CGContextRef ctx, const DroppedItem& item, float viewHeight);
void DrawDebugOverlay(CGContextRef ctx, const std::vector<Goose*>& geese);

// Anger tint - defined in behavior_anger.cpp
float Anger_GetLevel(int gooseId);

#endif