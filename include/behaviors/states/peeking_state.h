#ifndef PEEKING_STATE_H
#define PEEKING_STATE_H

#include "behavior_state.h"

struct PeekingState : public BehaviorState {
    bool isPeeking = false;
    double peekStartTime = 0;
    int peekSide = 0;
    double nextPeekTime = 0;

    void Reset() override {
        isPeeking = false;
        peekStartTime = 0;
        peekSide = 0;
        nextPeekTime = 0;
    }
};

#endif
