#import "ai_local_llm_adapter.h"
#include "log.h"
#import "local_llm.h"
#import "ai_prompt_builder.h"
#import "ai_think_block_stripper.h"
#import "ai_model_profiles.h"

static const int kAIChatRetryAttempts = 10;

// Human-readable explanation for a FoundationLLM_AvailabilityCode(). Shared by
// the chat error path and the Test Connection panel so the user is told *why*
// the local model can't be reached (most often: Apple Intelligence is off).
NSString* FoundationUnavailableMessage(int code) {
    switch (code) {
        case 2: return @"this Mac isn't eligible for Apple Intelligence";
        case 3: return @"Apple Intelligence is off — enable it in System Settings ▸ Apple Intelligence & Siri";
        case 4: return @"the on-device model is still downloading — try again shortly";
        case 1: return @"this build has no FoundationModels support (needs a macOS 26 SDK build)";
        default: return @"the on-device model is unavailable";
    }
}

// When Foundation declines (guardrail) we retry at a lower evil level so the
// user still gets a real reply instead of a canned line. Step down by this much
// each attempt, down to this floor (a guaranteed-safe persona).
static constexpr float kLocalRetryEvilStep = 0.28f;
static constexpr float kLocalRetryFloorEvil = 0.10f;

// Build the full prompt (system persona + recent history) for a given evil level.
static NSString* buildLocalPrompt(NSArray* history, float evilLevel) {
    NSMutableString* prompt = [NSMutableString string];
    [prompt appendString:systemPromptForEvilLevel(evilLevel)];
    [prompt appendString:@"\n\n"];
    NSInteger startIdx = MAX(0, (NSInteger)history.count - 5);
    for (NSInteger i = startIdx; i < (NSInteger)history.count; i++) {
        NSDictionary* msg = history[i];
        NSString* role = msg[@"role"];
        NSString* content = msg[@"content"];
        if ([role isEqualToString:@"user"]) {
            [prompt appendFormat:@"User: %@\n", content];
        } else if ([role isEqualToString:@"assistant"]) {
            [prompt appendFormat:@"Assistant: %@\n", content];
        }
    }
    return prompt;
}

// Generate at `evilLevel`; on an empty result from the FoundationModels backend
// (a guardrail refusal), recursively retry at a lower evil level before giving
// up to the chat's in-character canned fallback. Recursion is via this static
// function (not a self-referential block), so there's no lifetime hazard.
static void generateLocalAtEvil(NSArray* history, float evilLevel, BOOL foundation, float temperature,
                                void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    NSString* prompt = buildLocalPrompt(history, evilLevel);
    std::string promptStr = std::string([prompt UTF8String]);

    LocalLLM_Generate(promptStr, temperature, ^(const std::string& result) {
        // Convert synchronously — `result` references a temporary freed once
        // this callback returns; capturing it by reference would read freed
        // memory and deliver garbage.
        BOOL empty = result.empty();
        NSString* response = empty ? nil : [NSString stringWithUTF8String:result.c_str()];

        dispatch_async(dispatch_get_main_queue(), ^{
            if (!empty && response.length > 0) {
                NSString* stripped = stripThinkBlocks(response);
                connectedCallback(YES);
                CG_DEBUG("AI", "Local LLM response: %zu chars (evil=%.2f)", (size_t)stripped.length, evilLevel);
                if (completion) completion(stripped, nil);
                return;
            }
            if (!empty) {  // non-empty bytes but not valid UTF-8
                CG_ERROR("AI", "Local LLM returned invalid UTF-8");
                connectedCallback(NO);
                if (completion) completion(@"HONK! Local brain returned garbled text.", nil);
                return;
            }
            // Empty: Foundation's guardrail likely refused this persona. Retry a
            // notch milder so the user still gets a real, in-character answer.
            if (foundation && evilLevel > kLocalRetryFloorEvil) {
                float lower = MAX(kLocalRetryFloorEvil, evilLevel - kLocalRetryEvilStep);
                CG_ERROR("AI", "Foundation declined at evil=%.2f, retrying at %.2f", evilLevel, lower);
                generateLocalAtEvil(history, lower, foundation, temperature, completion, connectedCallback);
                return;
            }
            // Even the mild persona produced nothing — fall back to a canned line.
            CG_ERROR("AI", "Local LLM declined down to evil=%.2f — using canned fallback", evilLevel);
            connectedCallback(YES);
            if (completion) completion(nil, nil);
        });
    });
}

void completeWithLocalLLM(NSArray* history, float evilLevel, void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    CG_DEBUG("AI", "Foundation provider: routing to local LLM");

    // Cap the persona only when FoundationModels is the backend — its guardrail
    // refuses the most extreme levels. CoreML/other local models are uncapped.
    BOOL foundation = (FoundationLLM_IsAvailable() != 0);
    float effectiveEvil = foundation ? CapEvilForFoundation(evilLevel) : evilLevel;

    const BuiltinProfile* profile = MatchProfile("foundation");
    float temperature = profile->temperature;

    LocalLLM_Init();
    LocalLLMState state = LocalLLM_GetState();

    // Only Unavailable/Error mean we genuinely can't generate. Ready and
    // Loading both proceed to generation, mirroring the Test Connection panel
    // (which tolerates Loading) so the chat window no longer rejects a model
    // that is simply still warming up on its background loader thread.
    if (state == LocalLLMState::Unavailable) {
        // Foundation is the default provider, so explain the specific reason
        // (Apple Intelligence off, model downloading, etc.) rather than a
        // generic "no model" message that sends users hunting in settings.
        int code = FoundationLLM_AvailabilityCode();
        CG_ERROR("AI", "Local LLM unavailable (foundation code=%d)", code);
        connectedCallback(NO);
        NSString* why = (code == 0)
            ? @"no local model found — add one in settings or enable Apple Intelligence"
            : FoundationUnavailableMessage(code);
        if (completion) completion([NSString stringWithFormat:@"HONK! Can't reach the local brain: %@.", why], nil);
        return;
    }
    if (state == LocalLLMState::Error) {
        CG_ERROR("AI", "Local LLM in error state");
        connectedCallback(NO);
        if (completion) completion(@"HONK! The local brain hit an error. Check settings.", nil);
        return;
    }

    // state is Ready or Loading. Run on a background queue: LocalLLM_Generate
    // has its own wait-for-Ready loop (blocking usleep, up to ~30s) for the
    // CoreML path, which must NOT run on the main thread or it freezes the UI.
    // The FoundationModels path ignores state and returns immediately.
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        generateLocalAtEvil(history, effectiveEvil, foundation, temperature, completion, connectedCallback);
    });
}

void checkLocalLLMConnection(void(^completion)(BOOL connected, NSString* message)) {
    LocalLLM_Init();
    LocalLLMState state = LocalLLM_GetState();
    if (state == LocalLLMState::Ready) {
        if (completion) completion(YES, @"Local LLM ready");
    } else if (state == LocalLLMState::Loading) {
        __block int attempts = 0;
        void (^checkAgain)(void) = ^{
            LocalLLMState s = LocalLLM_GetState();
            if (s == LocalLLMState::Ready) {
                if (completion) completion(YES, @"Local LLM ready");
            } else if (s == LocalLLMState::Loading && attempts < kAIChatRetryAttempts) {
                attempts++;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), checkAgain);
            } else if (s == LocalLLMState::Error) {
                if (completion) completion(NO, @"Local LLM error");
            } else {
                if (completion) completion(NO, @"No local model found");
            }
        };
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), checkAgain);
    } else if (state == LocalLLMState::Error) {
        if (completion) completion(NO, @"Local LLM error");
    } else {
        if (completion) completion(NO, @"No local model found");
    }
}
