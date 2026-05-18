#ifndef RAINBOW_STATE_H
#define RAINBOW_STATE_H

#include "behavior_state.h"

struct RainbowState : public BehaviorState {
    float hue = 0.0f;
    double lastUpdate = 0;

    void Reset() override {
        hue = 0.0f;
        lastUpdate = 0;
    }
};

#endif
