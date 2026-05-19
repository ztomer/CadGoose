#ifndef AI_MODEL_PROFILES_H
#define AI_MODEL_PROFILES_H

struct BuiltinProfile {
    const char* pattern;
    float temperature;
    int maxTokens;
    int timeoutSecs;
    bool hasReasoningContent;
    bool prependJsonTrigger;
};

const BuiltinProfile* MatchProfile(const char* modelName);

#endif
