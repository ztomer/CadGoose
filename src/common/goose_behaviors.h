// goose_behaviors.h
// Declarations for split behavior implementation functions
#ifndef GOOSE_BEHAVIORS_H
#define GOOSE_BEHAVIORS_H

#include "goose.h"
#include "cursor_io.h"

// Internal helpers (goose_behaviors_internal.cpp)
void triggerHonk(Goose& g, double time, double cd, double& lastBucket);
void initHonkState(Goose::HonkState& hs, double time);
void updateIdleHonk(Goose& g, double time, double cd, double& lastGeneric);
Vector2 GetSnatchForward(float dir, const Vector2& isoScale);

// Wander behaviors (goose_behaviors_wander.cpp)
CursorAction handleChaseCursor(Goose& g, double time, const CursorState& cursor, int w, int h);
void handleWander(Goose& g, double time, const CursorState& cursor, int w, int h);

// Fetch behaviors (goose_behaviors_fetch.cpp)
void handleFetching(Goose& g, double time, int w, int h);
void handleReturning(Goose& g, double time, int w, int h);
void tryPickupItem(Goose& g, double time, int w, int h);

// Interaction behaviors (goose_behaviors_interact.cpp)
bool isTargetReached(Goose& g, float threshold);

#endif // GOOSE_BEHAVIORS_H
