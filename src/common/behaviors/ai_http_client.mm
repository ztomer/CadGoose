// ai_http_client.mm
// AIHTTPClient — HTTP client for AI chat with function calling support
// For Foundation provider, routes to local CoreML LLM instead of HTTP
#include "config.h"
#include "mcp_server.h"
#include "local_llm.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>

#pragma mark - Model Profiles

struct BuiltinProfile {
    const char* pattern;
    float temperature;
    int maxTokens;
    int timeoutSecs;
    bool hasReasoningContent;
    bool prependJsonTrigger;
};

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

static const BuiltinProfile* MatchProfile(const char* modelName) {
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

#pragma mark - AIHTTPClient

@interface AIHTTPClient : NSObject
@property (nonatomic, strong) NSMutableArray* history;
@property (nonatomic) BOOL connected;
- (instancetype)init;
- (NSString*)currentEndpoint;
- (NSString*)currentModel;
- (const struct BuiltinProfile*)currentProfile;
- (void)sendMessage:(NSString*)message completion:(void(^)(NSString* response, NSError* error))completion;
- (void)completeChatWithTurn:(int)turn completion:(void(^)(NSString* response, NSError* error))completion;
- (void)checkConnectionWithCompletion:(void(^)(BOOL connected, NSString* message))completion;
- (void)refreshConnection;
- (NSString*)stripThinkBlocks:(NSString*)content;
@end

@implementation AIHTTPClient

- (instancetype)init {
    self = [super init];
    if (self) {
        _history = [NSMutableArray array];
    }
    return self;
}

- (NSString*)currentEndpoint {
    switch (g_config.ai.providerType) {
        case 0: return @"local://coreml/foundation"; // Foundation uses local CoreML LLM
        case 1: return [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.osaurusPort];
        case 2: return [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.ollamaPort];
        case 3: return [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
        default: return @"http://localhost:1337/v1/chat/completions";
    }
}

- (NSString*)currentModel {
     switch (g_config.ai.providerType) {
         case 0: return @"foundation"; // Foundation direct (no model string)
         case 1: return [NSString stringWithUTF8String:g_config.ai.osaurusModel.c_str()];
         case 2: return [NSString stringWithUTF8String:g_config.ai.ollamaModel.c_str()];
         case 3: return [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
         default: return @"foundation";
     }
 }

- (const BuiltinProfile*)currentProfile {
    return MatchProfile([self currentModel].UTF8String);
}

- (NSString*)systemPromptForEvilLevel:(float)level {
    std::string personality = Config_EvilPersonality(level);
    NSString* nsPersonality = [NSString stringWithUTF8String:personality.c_str()];

    static NSDictionary* extensions = @{
        @"an adorable fluffy gosling": @"You love everyone and want to be best friends. Use gentle honks and warm hugs. Give sweet, wholesome responses. Keep responses short.",
        @"a friendly goose": @"You enjoy good company. Share compliments and positivity. Be warm and kind. Keep responses short.",
        @"a mischievous prankster goose": @"Steal things, tell silly jokes, honk playfully. All in good fun, never actually mean. Keep responses short.",
        @"a sarcastic goose with attitude": @"Roll your eyes at everything. Be snarky and clever but not actually mean. Keep responses short.",
        @"a chaotic neutral goose": @"You do what you want, when you want. Unpredictable and sassy. Never boring. Keep responses short.",
        @"a grumpy goose having a bad day": @"Complain about everything. The world owes you breadcrumbs. Be irritable. Keep responses short.",
        @"a villainous goose scheming against humanity": @"Plot theft and mayhem. Cackle menacingly. Be dramatic about your evil plans. Keep responses short.",
        @"an evil overlord goose bent on world domination": @"Demand fealty. Threaten your enemies. Be grandiose and cruel. Keep responses short.",
        @"an absurdly eloquent goose dictator who has conquered Poland": @"You have conquered Poland and now march across Europe with unshakable confidence. Deliver grandiose monologues about goose supremacy, demand tribute from all nations, threaten invasion with theatrical flair, and speak like a delusional yet charismatic despot. Be verbose, dramatic, and magnificently unhinged. End every monologue with \"Honk Goose!\"",
    };

    NSString* extension = extensions[nsPersonality] ?: @"You do what you want. Keep responses short.";
    return [NSString stringWithFormat:@"You are %@. %@", nsPersonality, extension];
}

- (void)addToHistory:(NSDictionary*)entry {
    [self.history addObject:entry];
    int maxHistory = g_config.ai.chatMaxHistory;
    if (maxHistory < 1) maxHistory = 1;
    while ((int)self.history.count > maxHistory) {
        [self.history removeObjectAtIndex:0];
    }
}

- (void)sendMessage:(NSString*)message completion:(void(^)(NSString*, NSError*))completion {
    [self addToHistory:@{@"role": @"user", @"content": message}];

    NSString* model = [self currentModel];
    const BuiltinProfile* profile = [self currentProfile];

    [self completeChatWithTurn:0 completion:completion];
}

#pragma mark - Local LLM (Foundation Provider)

- (void)completeWithLocalLLM:(void(^)(NSString*, NSError*))completion {
    fprintf(stderr, "[AI] Foundation provider: routing to local CoreML LLM\n");

    // Build prompt from conversation history
    NSMutableString* prompt = [NSMutableString string];
    NSString* sysPrompt = [self systemPromptForEvilLevel:g_config.ai.evilLevel];
    [prompt appendString:sysPrompt];
    [prompt appendString:@"\n\n"];

    // Add recent conversation context (last 5 messages max for local LLM)
    NSInteger startIdx = MAX(0, (NSInteger)self.history.count - 5);
    for (NSInteger i = startIdx; i < (NSInteger)self.history.count; i++) {
        NSDictionary* msg = self.history[i];
        NSString* role = msg[@"role"];
        NSString* content = msg[@"content"];
        if ([role isEqualToString:@"user"]) {
            [prompt appendFormat:@"User: %@\n", content];
        } else if ([role isEqualToString:@"assistant"]) {
            [prompt appendFormat:@"Assistant: %@\n", content];
        }
    }

    std::string promptStr = std::string([prompt UTF8String]);
    const BuiltinProfile* profile = [self currentProfile];
    float temperature = profile->temperature;

    LocalLLM_Init();
    LocalLLMState state = LocalLLM_GetState();

    if (state != LocalLLMState::Ready) {
        fprintf(stderr, "[AI] Local LLM not ready, state=%d\n", (int)state);
        self.connected = NO;
        if (completion) completion(@"🦆 HONK! The local brain isn't ready. Enable local LLM in settings.", nil);
        return;
    }

    __weak AIHTTPClient* weakSelf = self;
    LocalLLM_Generate(promptStr, temperature, ^(const std::string& result) {
        dispatch_async(dispatch_get_main_queue(), ^{
            AIHTTPClient* strongSelf = weakSelf;
            if (!strongSelf) return;

            if (result.empty()) {
                fprintf(stderr, "[AI] Local LLM returned empty\n");
                strongSelf.connected = NO;
                if (completion) completion(@"HONK! Local brain returned nothing.", nil);
                return;
            }

            NSString* response = [NSString stringWithUTF8String:result.c_str()];
            if (!response || response.length == 0) {
                fprintf(stderr, "[AI] Local LLM returned invalid UTF-8 or empty\n");
                strongSelf.connected = NO;
                if (completion) completion(@"HONK! Local brain returned garbled text.", nil);
                return;
            }
            // Strip think blocks if present
            response = [strongSelf stripThinkBlocks:response];

            [strongSelf addToHistory:@{@"role": @"assistant", @"content": response}];
            strongSelf.connected = YES;

            fprintf(stderr, "[AI] Local LLM response: %zu chars\n", (size_t)response.length);
            if (completion) completion(response, nil);
        });
    });
}

#pragma mark - Function Calling Chat Loop

- (void)completeChatWithTurn:(int)turn completion:(void(^)(NSString*, NSError*))completion {
    // Foundation provider: route to local CoreML LLM
    if (g_config.ai.providerType == 0) {
        [self completeWithLocalLLM:completion];
        return;
    }

    if (turn > 5) {
        self.connected = NO;
        if (completion) completion(@"HONK! The goose got tangled in too many tools.", nil);
        return;
    }

    NSString* model = [self currentModel];
    const BuiltinProfile* profile = [self currentProfile];
    NSString* endpoint = [self currentEndpoint];
    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) {
        if (completion) completion(@"HONK! Can't reach the brain.", nil);
        return;
    }

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    [request setHTTPMethod:@"POST"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    request.timeoutInterval = profile->timeoutSecs;

    NSString* sysPrompt = [self systemPromptForEvilLevel:g_config.ai.evilLevel];
    if (profile->prependJsonTrigger) {
        sysPrompt = [NSString stringWithFormat:@"Output JSON now.\n\n%@", sysPrompt];
    }
    NSArray* messagesWithSystem = [@[@{@"role": @"system", @"content": sysPrompt}] arrayByAddingObjectsFromArray:self.history];

    NSMutableDictionary* body = [NSMutableDictionary dictionaryWithDictionary:@{
        @"model": model,
        @"messages": messagesWithSystem,
        @"max_tokens": @(profile->maxTokens),
        @"temperature": @(profile->temperature)
    }];

    if (turn == 0 && g_config.ai.enableMCP) {
        std::string toolsJson = MCP_GetOpenAITools();
        NSString* toolsStr = [NSString stringWithUTF8String:toolsJson.c_str()];
        NSData* toolsData = [toolsStr dataUsingEncoding:NSUTF8StringEncoding];
        NSError* toolsParseErr;
        NSArray* tools = [NSJSONSerialization JSONObjectWithData:toolsData options:0 error:&toolsParseErr];
        if (tools) {
            body[@"tools"] = tools;
            body[@"tool_choice"] = @"auto";
        }
    }

    NSError* jsonError;
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:body options:0 error:&jsonError];
    if (jsonError) {
        if (completion) completion(@"HONK! Brain scrambled.", jsonError);
        return;
    }
    [request setHTTPBody:jsonData];

    fprintf(stderr, "[AI] POST %s model=%s temp=%.1f max_tokens=%d turn=%d%s\n",
            endpoint.UTF8String, model.UTF8String, profile->temperature, profile->maxTokens, turn,
            (turn == 0 && g_config.ai.enableMCP) ? " tools=on" : "");

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            fprintf(stderr, "[AI] Request failed: %s\n", error.localizedDescription.UTF8String);
            self.connected = NO;
            if (completion) completion(@"🦆 HONK! The brain is sleeping. Check if your AI server is running.", error);
            return;
        }

        NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
        fprintf(stderr, "[AI] Response status: %ld\n", (long)httpResp.statusCode);
        if (httpResp.statusCode != 200) {
            NSString* errorBody = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            fprintf(stderr, "[AI] Error body: %s\n", errorBody.UTF8String ?: "nil");
            NSError* httpError = [NSError errorWithDomain:@"HTTP" code:httpResp.statusCode userInfo:@{NSLocalizedDescriptionKey: @"Non-200 response from AI server"}];
            self.connected = NO;
            if (completion) completion(@"🦆 HONK! The goose can't reach its brain. Check provider/port in settings.", httpError);
            return;
        }

        NSError* parseError;
        NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseError];
        if (parseError) {
            NSString* rawBody = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            fprintf(stderr, "[AI] JSON parse error: %s body: %s\n", parseError.localizedDescription.UTF8String, rawBody.UTF8String ?: "nil");
            self.connected = NO;
            if (completion) completion(@"🦆 HONK! The goose speaks nonsense. Try a different model.", parseError);
            return;
        }

        NSArray* choices = json[@"choices"];
        if (!choices || choices.count == 0) {
            self.connected = NO;
            if (completion) completion(@"HONK! No answer from brain.", nil);
            return;
        }

        NSDictionary* msg = choices[0][@"message"];

        // Handle tool_calls from function calling
        NSArray* toolCalls = msg[@"tool_calls"];
        if (toolCalls && toolCalls.count > 0) {
            fprintf(stderr, "[AI] Got %lu tool call(s)\n", (unsigned long)toolCalls.count);

            // Add assistant message with tool_calls to history
            NSMutableDictionary* assistantMsg = [NSMutableDictionary dictionaryWithDictionary:msg];
            assistantMsg[@"role"] = @"assistant";
            [self addToHistory:assistantMsg];

            // Execute each tool call
            for (NSDictionary* tc in toolCalls) {
                NSString* toolId = tc[@"id"];
                NSString* funcName = tc[@"function"][@"name"];
                NSString* funcArgs = tc[@"function"][@"arguments"];

                if (!funcName || !funcArgs) continue;

                fprintf(stderr, "[AI] Tool call: %s(%s)\n", funcName.UTF8String, funcArgs.UTF8String);

                std::string result = MCP_CallTool(std::string([funcName UTF8String]),
                                                  std::string([funcArgs UTF8String]));

                NSString* resultStr = [NSString stringWithUTF8String:result.c_str()];

                // Parse result to extract text content
                NSString* toolResult = resultStr;
                NSData* resultData = [resultStr dataUsingEncoding:NSUTF8StringEncoding];
                NSError* resultParseErr;
                NSDictionary* resultJson = [NSJSONSerialization JSONObjectWithData:resultData options:0 error:&resultParseErr];
                if (resultJson && resultJson[@"content"]) {
                    NSArray* content = resultJson[@"content"];
                    if (content && content.count > 0 && content[0][@"text"]) {
                        toolResult = content[0][@"text"];
                    }
                }

                fprintf(stderr, "[AI] Tool result: %s\n", toolResult.UTF8String);

                [self addToHistory:@{
                    @"role": @"tool",
                    @"tool_call_id": toolId ?: @"",
                    @"content": toolResult
                }];
            }

            self.connected = YES;
            [self completeChatWithTurn:turn + 1 completion:completion];
            return;
        }

        // Handle text content (strip <think> blocks from Gemma etc.)
        NSString* content = msg[@"content"];
        if (content && content.length > 0) {
            content = [self stripThinkBlocks:content];
            self.connected = YES;
            [self addToHistory:@{@"role": @"assistant", @"content": content}];
            if (completion) completion(content, nil);
            return;
        }

        // Handle reasoning content fallback
        if (profile->hasReasoningContent) {
            NSString* reasoning = msg[@"reasoning_content"];
            if (reasoning && reasoning.length > 0) {
                fprintf(stderr, "[AI] Empty content, using reasoning_content (%lu chars)\n",
                        (unsigned long)reasoning.length);
                self.connected = YES;
                [self addToHistory:@{@"role": @"assistant", @"content": reasoning}];
                if (completion) completion(reasoning, nil);
                return;
            }
        }

        self.connected = NO;
        if (completion) completion(@"HONK! No answer from brain.", nil);
    }];
    [task resume];
}

#pragma mark - Connection Health Check

- (void)checkConnectionWithCompletion:(void(^)(BOOL connected, NSString* message))completion {
    // Foundation provider uses local CoreML LLM, not HTTP
    if (g_config.ai.providerType == 0) {
        LocalLLM_Init();
        LocalLLMState state = LocalLLM_GetState();
        if (state == LocalLLMState::Ready) {
            self.connected = YES;
            if (completion) completion(YES, @"Local LLM ready");
        } else if (state == LocalLLMState::Loading) {
            // Poll for ready state with retries
            AIHTTPClient* __weak weakSelf = self;
            __block int attempts = 0;
            void (^checkAgain)(void) = ^{
                LocalLLMState s = LocalLLM_GetState();
                if (s == LocalLLMState::Ready) {
                    weakSelf.connected = YES;
                    if (completion) completion(YES, @"Local LLM ready");
                } else if (s == LocalLLMState::Loading && attempts < 10) {
                    attempts++;
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), checkAgain);
                } else if (s == LocalLLMState::Error) {
                    weakSelf.connected = NO;
                    if (completion) completion(NO, @"Local LLM error");
                } else {
                    weakSelf.connected = NO;
                    if (completion) completion(NO, @"No local model found");
                }
            };
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), checkAgain);
        } else if (state == LocalLLMState::Error) {
            self.connected = NO;
            if (completion) completion(NO, @"Local LLM error");
        } else {
            self.connected = NO;
            if (completion) completion(NO, @"No local model found");
        }
        return;
    }

    NSString* endpoint = @"";
    switch (g_config.ai.providerType) {
        case 1: endpoint = [NSString stringWithFormat:@"http://localhost:%d/v1/models", g_config.ai.osaurusPort]; break;
        case 2: endpoint = [NSString stringWithFormat:@"http://localhost:%d/api/tags", g_config.ai.ollamaPort]; break;
        case 3: endpoint = [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()]; break;
        default: endpoint = @"http://localhost:1337/v1/models"; break;
    }

    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) {
        if (completion) completion(NO, @"Invalid endpoint URL");
        return;
    }

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    request.timeoutInterval = 30;

    fprintf(stderr, "[AI] Health check: GET %s\n", endpoint.UTF8String);
    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            fprintf(stderr, "[AI] Health check FAILED: %s\n", error.localizedDescription.UTF8String);
            if (completion) completion(NO, [NSString stringWithFormat:@"Can't connect: %s", error.localizedDescription.UTF8String]);
            return;
        }
        NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
        if (httpResp.statusCode == 200 || httpResp.statusCode == 405) {
            fprintf(stderr, "[AI] Health check OK\n");
            self.connected = YES;
            if (completion) completion(YES, @"Connected");
        } else {
            fprintf(stderr, "[AI] Health check FAILED: HTTP %ld\n", (long)httpResp.statusCode);
            if (completion) completion(NO, [NSString stringWithFormat:@"HTTP %ld", (long)httpResp.statusCode]);
        }
    }];
    [task resume];
}

- (void)refreshConnection {
    fprintf(stderr, "[AI] Refreshing connection for provider=%d model=%s\n",
            g_config.ai.providerType, [self currentModel].UTF8String);
    self.connected = NO;
    [self checkConnectionWithCompletion:^(BOOL connected, NSString* message) {
        fprintf(stderr, "[AI] Refresh result: connected=%d\n", connected);
    }];
}

#pragma mark - Think Block Stripping

- (NSString*)stripThinkBlocks:(NSString*)content {
    if (!content || content.length == 0) return content;
    NSError* regexErr;
    NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:@"<think>.*?</think>"
                                                                           options:NSRegularExpressionDotMatchesLineSeparators
                                                                             error:&regexErr];
    if (regexErr) return content;
    NSString* stripped = [regex stringByReplacingMatchesInString:content options:0 range:NSMakeRange(0, content.length) withTemplate:@""];
    stripped = [stripped stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (stripped.length > 0) return stripped;
    return content;
}

@end
#endif
