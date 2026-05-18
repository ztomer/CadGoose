#ifndef INTERACTIVE_DROPS_STATE_H
#define INTERACTIVE_DROPS_STATE_H

#include "behavior_state.h"
#include "world.h"
#include <vector>

struct InteractiveDropsState : public BehaviorState {
    std::vector<InteractivePuddle> puddles;
    std::vector<InteractiveFlower> flowers;
    double lastDropTime = 0;

    void Reset() override {
        puddles.clear();
        flowers.clear();
        lastDropTime = 0;
    }
};

#endif
