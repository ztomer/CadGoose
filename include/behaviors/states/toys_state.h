#ifndef TOYS_STATE_H
#define TOYS_STATE_H

#include "behavior_state.h"
#include "goose_math.h"

struct Toy {
    Vector2 pos{0, 0};
    float angle = 0;
    double time{0};
    bool active = false;
    enum class Type : int { Stick = 0, Ball = 1 } type = Type::Stick;
};

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
