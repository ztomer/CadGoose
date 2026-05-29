#import "ai_local_llm_adapter.h"
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

// Runs the actual generation. LocalLLM_Generate internally waits for the model
// to finish loading (Loading -> Ready) before producing tokens, so callers can
// invoke this whenever the model isn't in an Unavailable/Error state.
static void runLocalLLMGenerate(const std::string& promptStr, float temperature,
                                void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    LocalLLM_Generate(promptStr, temperature, ^(const std::string& result) {
        // `result` is a reference to a temporary std::string that is destroyed
        // as soon as this callback returns (it lives in the caller's frame —
        // e.g. the FoundationModels C trampoline). Convert it to an NSString
        // NOW, synchronously, before handing off to the async block below.
        // Capturing `result` by reference into dispatch_async read freed memory
        // and delivered garbage text to the chat.
        BOOL empty = result.empty();
        NSString* response = empty ? nil : [NSString stringWithUTF8String:result.c_str()];

        dispatch_async(dispatch_get_main_queue(), ^{
            if (empty) {
                // Empty usually means the on-device model *declined* to answer:
                // Apple's FoundationModels enforces guardrails that refuse spicy
                // personas (high evil level / dictator / Stalin prompts) with a
                // guardrailViolation, returning no text. The model is reachable,
                // so keep the connection green and return nil — the chat then
                // falls back to an in-character canned line instead of showing a
                // confusing "returned nothing" error.
                fprintf(stderr, "[AI] Local LLM returned empty (model likely declined — guardrail). Falling back.\n");
                connectedCallback(YES);
                if (completion) completion(nil, nil);
                return;
            }

            if (!response || response.length == 0) {
                fprintf(stderr, "[AI] Local LLM returned invalid UTF-8 or empty\n");
                connectedCallback(NO);
                if (completion) completion(@"HONK! Local brain returned garbled text.", nil);
                return;
            }

            NSString* stripped = stripThinkBlocks(response);
            connectedCallback(YES);

            fprintf(stderr, "[AI] Local LLM response: %zu chars\n", (size_t)stripped.length);
            if (completion) completion(stripped, nil);
        });
    });
}

void completeWithLocalLLM(NSArray* history, float evilLevel, void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    fprintf(stderr, "[AI] Foundation provider: routing to local LLM\n");

    NSMutableString* prompt = [NSMutableString string];
    // Cap the persona only when FoundationModels is the backend — its guardrail
    // refuses the most extreme levels. CoreML/other local models are uncapped.
    float effectiveEvil = FoundationLLM_IsAvailable() ? CapEvilForFoundation(evilLevel) : evilLevel;
    NSString* sysPrompt = systemPromptForEvilLevel(effectiveEvil);
    [prompt appendString:sysPrompt];
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

    std::string promptStr = std::string([prompt UTF8String]);
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
        fprintf(stderr, "[AI] Local LLM unavailable (foundation code=%d)\n", code);
        connectedCallback(NO);
        NSString* why = (code == 0)
            ? @"no local model found — add one in settings or enable Apple Intelligence"
            : FoundationUnavailableMessage(code);
        if (completion) completion([NSString stringWithFormat:@"🦆 HONK! Can't reach the local brain: %@.", why], nil);
        return;
    }
    if (state == LocalLLMState::Error) {
        fprintf(stderr, "[AI] Local LLM in error state\n");
        connectedCallback(NO);
        if (completion) completion(@"🦆 HONK! The local brain hit an error. Check settings.", nil);
        return;
    }

    // state is Ready or Loading. Run on a background queue: LocalLLM_Generate
    // has its own wait-for-Ready loop (blocking usleep, up to ~30s) for the
    // CoreML path, which must NOT run on the main thread or it freezes the UI.
    // The FoundationModels path ignores state and returns immediately.
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        runLocalLLMGenerate(promptStr, temperature, completion, connectedCallback);
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
