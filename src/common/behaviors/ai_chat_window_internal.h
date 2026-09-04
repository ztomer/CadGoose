// ai_chat_window_internal.h
// AIState — the per-goose AI behavior state, shared by the behavior
// registration (behavior_ai.mm) and the chat window (ai_chat_window.mm).

#pragma once

#include <string>
#include <vector>
#include "behavior.h"

struct AIState : public BehaviorState {
    std::vector<std::string> conversationHistory;
    double lastQuestionTime = 0;
    bool awaitingResponse = false;

    void Reset() override {
        conversationHistory.clear();
        lastQuestionTime = 0;
        awaitingResponse = false;
    }
};
