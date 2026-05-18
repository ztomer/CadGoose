#ifndef PORTAL_STATE_H
#define PORTAL_STATE_H

#include "behavior_state.h"

struct PortalState : public BehaviorState {
    struct Portal {
        float x = 0, y = 0;
        bool active = false;
        int portalId = 0;
        float width = 80.0f;
        float height = 80.0f;
    };
    Portal portalA;
    Portal portalB;
    bool portalsEnabled = true;
    bool justTeleported = false;

    void Reset() override {
        portalA.active = false;
        portalB.active = false;
        portalsEnabled = true;
        justTeleported = false;
    }
};

#endif
