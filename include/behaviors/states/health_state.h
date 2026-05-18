#ifndef HEALTH_STATE_H
#define HEALTH_STATE_H

#include "behavior_state.h"

struct HealthState : public BehaviorState {
    float currentHealth = 100.0f;
    float maxHealth = 100.0f;
    double lastDamageTime = 0;
    float regenAccumulator = 0.0f;
    bool isDead = false;

    void Reset() override {
        currentHealth = 100.0f;
        maxHealth = 100.0f;
        lastDamageTime = 0;
        regenAccumulator = 0.0f;
        isDead = false;
    }
};

#endif
