#ifndef JAIL_STATE_H
#define JAIL_STATE_H

#include "behavior_state.h"

struct JailState : public BehaviorState {
    bool isJailed = false;

    void Reset() override {
        isJailed = false;
    }
};

#endif
