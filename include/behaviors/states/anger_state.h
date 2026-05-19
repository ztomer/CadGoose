#ifndef ANGER_STATE_H
#define ANGER_STATE_H

#include "behavior_state.h"
#include "event_bus.h"

struct AngerState : public BehaviorState {
    float angerLevel = 0.0f;
    double lastAngerIncrease = 0;
    double lastPunchTime = 0;
    bool isPunching = false;
    SubscriptionId honkSub = 0;
    SubscriptionId cursorSub = 0;

    void Reset() override {
        angerLevel = 0.0f;
        lastAngerIncrease = 0;
        lastPunchTime = 0;
        isPunching = false;
        honkSub = 0;
        cursorSub = 0;
    }
};

#endif
