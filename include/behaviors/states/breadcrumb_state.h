#ifndef BREADCRUMB_STATE_H
#define BREADCRUMB_STATE_H

#include "behavior_state.h"
#include "goose_math.h"
#include <vector>

struct BreadCrumbState : public BehaviorState {
    struct Crumb {
        Vector2 pos{0, 0};
        Vector2 vel{0, 0};
        float lifetime = 0;
        bool active = true;
    };
    std::vector<Crumb> crumbs;
    double lastThrowTime = 0;
    bool isThrowing = false;

    void Reset() override {
        for (auto& crumb : crumbs) {
            crumb.active = false;
        }
        lastThrowTime = 0;
        isThrowing = false;
    }
};

#endif
