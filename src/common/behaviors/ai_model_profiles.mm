#import "ai_model_profiles.h"
#import <Foundation/Foundation.h>

static BuiltinProfile s_profiles[] = {
    {"qwen*",       0.9, 300, 120, true,  true},
    {"foundation*", 0.8, 200, 60,  false, false},
    {"gemma*",      0.7, 200, 60,  false, false},
    {"nemotron*",   0.7, 200, 60,  false, false},
    {"laguna*",     0.8, 200, 60,  false, false},
    {"minimax*",    0.8, 200, 60,  false, false},
    {"llama*",      0.7, 200, 60,  false, false},
    {nullptr,       0.8, 200, 30,  false, false},
};

static const BuiltinProfile* DefaultProfile() {
    int count = sizeof(s_profiles) / sizeof(s_profiles[0]);
    return &s_profiles[count - 1];
}

const BuiltinProfile* MatchProfile(const char* modelName) {
    if (!modelName) return DefaultProfile();
    NSString* name = [NSString stringWithUTF8String:modelName];
    for (int i = 0; s_profiles[i].pattern; i++) {
        NSString* pat = [NSString stringWithUTF8String:s_profiles[i].pattern];
        if ([pat hasSuffix:@"*"]) {
            NSString* prefix = [pat substringToIndex:pat.length - 1];
            if ([name hasPrefix:prefix])
                return &s_profiles[i];
        }
    }
    return DefaultProfile();
}
