#ifndef ACID_STATE_H
#define ACID_STATE_H

#include "behavior_state.h"

struct AcidState : public BehaviorState {
    float rotationAccumulator = 0.0f;
    double lastHonkTime = 0;
    bool isSpinning = false;

    void Reset() override {
        rotationAccumulator = 0.0f;
        lastHonkTime = 0;
        isSpinning = false;
    }
};

#endif
