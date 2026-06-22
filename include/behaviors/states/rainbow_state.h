#ifndef RAINBOW_STATE_H
#define RAINBOW_STATE_H

#include "behavior_state.h"

struct RainbowState : public BehaviorState {
    float hue = 0.0f;

    void Reset() override {
        hue = 0.0f;
    }
};

#endif
