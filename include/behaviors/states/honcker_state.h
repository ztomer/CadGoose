#ifndef HONCKER_STATE_H
#define HONCKER_STATE_H

#include "behavior_state.h"

struct HonckerState : public BehaviorState {
    double lastHonkTime = 0;
    double lastShowTime = 0;
    bool isOnCooldown = false;
    bool visible = false;

    void Reset() override {
        lastHonkTime = 0;
        lastShowTime = 0;
        isOnCooldown = false;
        visible = false;
    }
};

#endif
