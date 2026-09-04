#ifndef INTERACTIVE_DROPS_STATE_H
#define INTERACTIVE_DROPS_STATE_H

#include "behavior_state.h"
#include "goose_math.h"
#include <vector>

struct InteractivePuddle {
    Vector2 pos{0, 0};
    Vector2 vel{0, 0};
    float radius = 15.0f;
    double spawnTime = 0;
    bool splashed = false;
    float maxRadius = 40.0f;
    float alpha = 0.6f;
};

struct InteractiveFlower {
    Vector2 pos{0, 0};
    double spawnTime = 0;
    float growth = 0.0f;
    float stemHeight = 0.0f;
    float petalSize = 0.0f;
    float hue = 0.0f;
};

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
