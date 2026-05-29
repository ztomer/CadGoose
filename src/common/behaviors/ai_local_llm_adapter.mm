#import "ai_local_llm_adapter.h"
#import "local_llm.h"
#import "ai_prompt_builder.h"
#import "ai_think_block_stripper.h"
#import "ai_model_profiles.h"

static const int kAIChatRetryAttempts = 10;

// Runs the actual generation. LocalLLM_Generate internally waits for the model
// to finish loading (Loading -> Ready) before producing tokens, so callers can
// invoke this whenever the model isn't in an Unavailable/Error state.
static void runLocalLLMGenerate(const std::string& promptStr, float temperature,
                                void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    LocalLLM_Generate(promptStr, temperature, ^(const std::string& result) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (result.empty()) {
                fprintf(stderr, "[AI] Local LLM returned empty\n");
                connectedCallback(NO);
                if (completion) completion(@"HONK! Local brain returned nothing.", nil);
                return;
            }

            NSString* response = [NSString stringWithUTF8String:result.c_str()];
            if (!response || response.length == 0) {
                fprintf(stderr, "[AI] Local LLM returned invalid UTF-8 or empty\n");
                connectedCallback(NO);
                if (completion) completion(@"HONK! Local brain returned garbled text.", nil);
                return;
            }
            
            response = stripThinkBlocks(response);
            connectedCallback(YES);

            fprintf(stderr, "[AI] Local LLM response: %zu chars\n", (size_t)response.length);
            if (completion) completion(response, nil);
        });
    });
}

void completeWithLocalLLM(NSArray* history, float evilLevel, void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    fprintf(stderr, "[AI] Foundation provider: routing to local LLM\n");

    NSMutableString* prompt = [NSMutableString string];
    NSString* sysPrompt = systemPromptForEvilLevel(evilLevel);
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
        fprintf(stderr, "[AI] Local LLM unavailable\n");
        connectedCallback(NO);
        if (completion) completion(@"🦆 HONK! No local model found. Enable local LLM in settings.", nil);
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
