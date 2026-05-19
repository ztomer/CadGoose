// pomodoro_bed.h
// Accessor for the rest-bed sprite owned by the pomodoro behavior.
// Lives outside behavior_pomodoro.cpp because effect-window code in the
// platform layer needs to query bed position/visibility per goose.

#pragma once

#include "goose_math.h"

struct PomodoroBedInfo {
    Vector2 position;
    bool visible;
    void* bedImage; // CGImageRef on macOS, opaque elsewhere
};

PomodoroBedInfo Pomodoro_GetBedInfo(int gooseId);
