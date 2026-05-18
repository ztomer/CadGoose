#ifndef DRAG_STATE_H
#define DRAG_STATE_H

#include "behavior_state.h"

struct DragState : public BehaviorState {
    bool isDragging = false;
    float dragAnchorX = 0, dragAnchorY = 0;
    double dragStartTime = 0;
    float resistanceChance = 0.0f;

    void Reset() override {
        isDragging = false;
        dragAnchorX = 0;
        dragAnchorY = 0;
        dragStartTime = 0;
        resistanceChance = 0.0f;
    }
};

#endif
