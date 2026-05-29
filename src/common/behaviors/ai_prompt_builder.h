#ifndef AI_PROMPT_BUILDER_H
#define AI_PROMPT_BUILDER_H

#import <Foundation/Foundation.h>

NSString* systemPromptForEvilLevel(float level);

// Apple's on-device FoundationModels enforces safety guardrails that refuse the
// most extreme personas. Empirically, levels up to "villainous" generate fine
// while "evil overlord" / "dictator" get refused. This is the highest evil
// level we send to the FoundationModels backend; other providers are uncapped.
extern const float kFoundationMaxEvilLevel;

// Returns level clamped to kFoundationMaxEvilLevel (no-op if already below).
float CapEvilForFoundation(float level);

// Short human-readable note describing the FoundationModels persona cap, for
// display in the AI settings UI.
NSString* FoundationPersonaCapNote(void);

#endif
