#import "ai_local_llm_adapter.h"
#import "local_llm.h"
#import "ai_prompt_builder.h"
#import "ai_think_block_stripper.h"
#import "ai_model_profiles.h"

static const int kAIChatRetryAttempts = 10;

void completeWithLocalLLM(NSArray* history, float evilLevel, void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    fprintf(stderr, "[AI] Foundation provider: routing to local CoreML LLM\n");

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

    if (state != LocalLLMState::Ready) {
        fprintf(stderr, "[AI] Local LLM not ready, state=%d\n", (int)state);
        connectedCallback(NO);
        if (completion) completion(@"🦆 HONK! The local brain isn't ready. Enable local LLM in settings.", nil);
        return;
    }

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
