#ifndef TOYS_STATE_H
#define TOYS_STATE_H

#include "behavior_state.h"
#include "world.h"

struct ToysState : public BehaviorState {
    static constexpr int MAX_TOYS = 5;
    Toy toys[MAX_TOYS];
    double lastSpawnTime = 0;
    int activeCount = 0;

    void Reset() override {
        for (int i = 0; i < MAX_TOYS; ++i) {
            toys[i].active = false;
        }
        lastSpawnTime = 0;
        activeCount = 0;
    }
};

#endif
