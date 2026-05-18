#ifndef BOREDOM_STATE_H
#define BOREDOM_STATE_H

#include "behavior_state.h"

struct BoredomState : public BehaviorState {
    double idleStartTime = 0;
    bool isSighing = false;
    double sighStartTime = 0;
    bool isLyingDown = false;
    double lieDownStartTime = 0;

    void Reset() override {
        idleStartTime = 0;
        isSighing = false;
        sighStartTime = 0;
        isLyingDown = false;
        lieDownStartTime = 0;
    }
};

#endif
