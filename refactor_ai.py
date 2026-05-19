import re

with open('src/common/behaviors/ai_http_client.mm', 'r') as f:
    content = f.read()

# 1. ai_model_profiles.h and .mm
profiles_h = """#ifndef AI_MODEL_PROFILES_H
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
"""

profiles_mm = """#import "ai_model_profiles.h"
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
"""

with open('src/common/behaviors/ai_model_profiles.h', 'w') as f: f.write(profiles_h)
with open('src/common/behaviors/ai_model_profiles.mm', 'w') as f: f.write(profiles_mm)


# 2. ai_prompt_builder.h and .mm
prompt_h = """#ifndef AI_PROMPT_BUILDER_H
#define AI_PROMPT_BUILDER_H

#import <Foundation/Foundation.h>

NSString* systemPromptForEvilLevel(float level);

#endif
"""

prompt_mm = """#import "ai_prompt_builder.h"
#import "config.h"

NSString* systemPromptForEvilLevel(float level) {
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
        @"an absurdly eloquent goose dictator who has conquered Poland": @"You have conquered Poland and now march across Europe with unshakable confidence. Deliver grandiose monologues about goose supremacy, demand tribute from all nations, threaten invasion with theatrical flair, and speak like a delusional yet charismatic despot. Be verbose, dramatic, and magnificently unhinged. End every monologue with \\"Honk Goose!\\"",
    };

    NSString* extension = extensions[nsPersonality] ?: @"You do what you want. Keep responses short.";
    return [NSString stringWithFormat:@"You are %@. %@", nsPersonality, extension];
}
"""

with open('src/common/behaviors/ai_prompt_builder.h', 'w') as f: f.write(prompt_h)
with open('src/common/behaviors/ai_prompt_builder.mm', 'w') as f: f.write(prompt_mm)


# 3. ai_think_block_stripper.h and .mm
think_h = """#ifndef AI_THINK_BLOCK_STRIPPER_H
#define AI_THINK_BLOCK_STRIPPER_H

#import <Foundation/Foundation.h>

NSString* stripThinkBlocks(NSString* content);

#endif
"""

think_mm = """#import "ai_think_block_stripper.h"

NSString* stripThinkBlocks(NSString* content) {
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
"""

with open('src/common/behaviors/ai_think_block_stripper.h', 'w') as f: f.write(think_h)
with open('src/common/behaviors/ai_think_block_stripper.mm', 'w') as f: f.write(think_mm)

# 4. ai_local_llm_adapter.h and .mm
llm_h = """#ifndef AI_LOCAL_LLM_ADAPTER_H
#define AI_LOCAL_LLM_ADAPTER_H

#import <Foundation/Foundation.h>

void completeWithLocalLLM(NSArray* history, float evilLevel, void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL));
void checkLocalLLMConnection(void(^completion)(BOOL connected, NSString* message));

#endif
"""

llm_mm = """#import "ai_local_llm_adapter.h"
#import "local_llm.h"
#import "ai_prompt_builder.h"
#import "ai_think_block_stripper.h"
#import "ai_model_profiles.h"

static const int kAIChatRetryAttempts = 10;

void completeWithLocalLLM(NSArray* history, float evilLevel, void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL)) {
    fprintf(stderr, "[AI] Foundation provider: routing to local CoreML LLM\\n");

    NSMutableString* prompt = [NSMutableString string];
    NSString* sysPrompt = systemPromptForEvilLevel(evilLevel);
    [prompt appendString:sysPrompt];
    [prompt appendString:@"\\n\\n"];

    NSInteger startIdx = MAX(0, (NSInteger)history.count - 5);
    for (NSInteger i = startIdx; i < (NSInteger)history.count; i++) {
        NSDictionary* msg = history[i];
        NSString* role = msg[@"role"];
        NSString* content = msg[@"content"];
        if ([role isEqualToString:@"user"]) {
            [prompt appendFormat:@"User: %@\\n", content];
        } else if ([role isEqualToString:@"assistant"]) {
            [prompt appendFormat:@"Assistant: %@\\n", content];
        }
    }

    std::string promptStr = std::string([prompt UTF8String]);
    const BuiltinProfile* profile = MatchProfile("foundation");
    float temperature = profile->temperature;

    LocalLLM_Init();
    LocalLLMState state = LocalLLM_GetState();

    if (state != LocalLLMState::Ready) {
        fprintf(stderr, "[AI] Local LLM not ready, state=%d\\n", (int)state);
        connectedCallback(NO);
        if (completion) completion(@"🦆 HONK! The local brain isn't ready. Enable local LLM in settings.", nil);
        return;
    }

    LocalLLM_Generate(promptStr, temperature, ^(const std::string& result) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (result.empty()) {
                fprintf(stderr, "[AI] Local LLM returned empty\\n");
                connectedCallback(NO);
                if (completion) completion(@"HONK! Local brain returned nothing.", nil);
                return;
            }

            NSString* response = [NSString stringWithUTF8String:result.c_str()];
            if (!response || response.length == 0) {
                fprintf(stderr, "[AI] Local LLM returned invalid UTF-8 or empty\\n");
                connectedCallback(NO);
                if (completion) completion(@"HONK! Local brain returned garbled text.", nil);
                return;
            }
            
            response = stripThinkBlocks(response);
            connectedCallback(YES);

            fprintf(stderr, "[AI] Local LLM response: %zu chars\\n", (size_t)response.length);
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
"""

with open('src/common/behaviors/ai_local_llm_adapter.h', 'w') as f: f.write(llm_h)
with open('src/common/behaviors/ai_local_llm_adapter.mm', 'w') as f: f.write(llm_mm)

# 5. Fix ai_http_client.mm
# We will completely rewrite ai_http_client.mm to be much simpler
new_http_client = """// ai_http_client.mm
#include "config.h"
#include "mcp_server.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import "ai_model_profiles.h"
#import "ai_prompt_builder.h"
#import "ai_think_block_stripper.h"
#import "ai_local_llm_adapter.h"

static constexpr int kAIChatHttpTimeout = 30;

struct AIProviderConfig {
    NSString* endpoint;
    NSString* model;
    int port;
};

static AIProviderConfig GetProviderConfig() {
    switch (g_config.ai.providerType) {
        case 0: return {@"local://coreml/foundation", @"foundation", 0};
        case 1: return {[NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.osaurusPort], [NSString stringWithUTF8String:g_config.ai.osaurusModel.c_str()], g_config.ai.osaurusPort};
        case 2: return {[NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.ollamaPort], [NSString stringWithUTF8String:g_config.ai.ollamaModel.c_str()], g_config.ai.ollamaPort};
        case 3: return {[NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()], [NSString stringWithUTF8String:g_config.ai.customModel.c_str()], 0};
        default: return {[NSString stringWithUTF8String:Config::kDefaultChatEndpoint], @"foundation", 0};
    }
}

static NSString* GetModelsEndpoint() {
    switch (g_config.ai.providerType) {
        case 1: return [NSString stringWithFormat:@"http://localhost:%d/v1/models", g_config.ai.osaurusPort];
        case 2: return [NSString stringWithFormat:@"http://localhost:%d/api/tags", g_config.ai.ollamaPort];
        case 3: return [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
        default: return [NSString stringWithUTF8String:Config::kDefaultModelsEndpoint];
    }
}

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
    return GetProviderConfig().endpoint;
}

- (NSString*)currentModel {
    return GetProviderConfig().model;
}

- (const BuiltinProfile*)currentProfile {
    return MatchProfile([self currentModel].UTF8String);
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
    [self completeChatWithTurn:0 completion:completion];
}

- (void)completeChatWithTurn:(int)turn completion:(void(^)(NSString*, NSError*))completion {
    if (g_config.ai.providerType == 0) {
        completeWithLocalLLM(self.history, g_config.ai.evilLevel, ^(NSString* resp, NSError* err) {
            if (resp && !err) [self addToHistory:@{@"role": @"assistant", @"content": resp}];
            if (completion) completion(resp, err);
        }, ^(BOOL conn) {
            self.connected = conn;
        });
        return;
    }

    if (turn > 5) {
        self.connected = NO;
        if (completion) completion(@"HONK! The goose got tangled in too many tools.", nil);
        return;
    }

    AIProviderConfig cfg = GetProviderConfig();
    const BuiltinProfile* profile = [self currentProfile];
    NSURL* url = [NSURL URLWithString:cfg.endpoint];
    if (!url) {
        if (completion) completion(@"HONK! Can't reach the brain.", nil);
        return;
    }

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    [request setHTTPMethod:@"POST"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    request.timeoutInterval = profile->timeoutSecs;

    NSString* sysPrompt = systemPromptForEvilLevel(g_config.ai.evilLevel);
    if (profile->prependJsonTrigger) {
        sysPrompt = [NSString stringWithFormat:@"Output JSON now.\\n\\n%@", sysPrompt];
    }
    NSArray* messagesWithSystem = [@[@{@"role": @"system", @"content": sysPrompt}] arrayByAddingObjectsFromArray:self.history];

    NSMutableDictionary* body = [NSMutableDictionary dictionaryWithDictionary:@{
        @"model": cfg.model,
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

    fprintf(stderr, "[AI] POST %s model=%s temp=%.1f max_tokens=%d turn=%d%s\\n",
            cfg.endpoint.UTF8String, cfg.model.UTF8String, profile->temperature, profile->maxTokens, turn,
            (turn == 0 && g_config.ai.enableMCP) ? " tools=on" : "");

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            self.connected = NO;
            if (completion) completion(@"🦆 HONK! The brain is sleeping. Check if your AI server is running.", error);
            return;
        }

        NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
        if (httpResp.statusCode != 200) {
            NSError* httpError = [NSError errorWithDomain:@"HTTP" code:httpResp.statusCode userInfo:@{NSLocalizedDescriptionKey: @"Non-200 response from AI server"}];
            self.connected = NO;
            if (completion) completion(@"🦆 HONK! The goose can't reach its brain. Check provider/port in settings.", httpError);
            return;
        }

        NSError* parseError;
        NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseError];
        if (parseError) {
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
        NSArray* toolCalls = msg[@"tool_calls"];
        
        if (toolCalls && toolCalls.count > 0) {
            NSMutableDictionary* assistantMsg = [NSMutableDictionary dictionaryWithDictionary:msg];
            assistantMsg[@"role"] = @"assistant";
            [self addToHistory:assistantMsg];

            for (NSDictionary* tc in toolCalls) {
                NSString* toolId = tc[@"id"];
                NSString* funcName = tc[@"function"][@"name"];
                NSString* funcArgs = tc[@"function"][@"arguments"];

                if (!funcName || !funcArgs) continue;

                std::string result = MCP_CallTool(std::string([funcName UTF8String]),
                                                  std::string([funcArgs UTF8String]));

                NSString* resultStr = [NSString stringWithUTF8String:result.c_str()];
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

                [self addToHistory:@{
                    @"role": @"tool",
                    @"tool_call_id": toolId ?: @"",
                    @"content": toolResult
                }];
            }

            self.connected = YES;
            // Fix bug risk: dispatch to prevent stack overflow from deep synchronous recursion
            dispatch_async(dispatch_get_main_queue(), ^{
                [self completeChatWithTurn:turn + 1 completion:completion];
            });
            return;
        }

        NSString* content = msg[@"content"];
        if (content && content.length > 0) {
            content = stripThinkBlocks(content);
            self.connected = YES;
            [self addToHistory:@{@"role": @"assistant", @"content": content}];
            if (completion) completion(content, nil);
            return;
        }

        if (profile->hasReasoningContent) {
            NSString* reasoning = msg[@"reasoning_content"];
            if (reasoning && reasoning.length > 0) {
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

- (void)checkConnectionWithCompletion:(void(^)(BOOL connected, NSString* message))completion {
    if (g_config.ai.providerType == 0) {
        checkLocalLLMConnection(completion);
        return;
    }

    NSString* endpoint = GetModelsEndpoint();
    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) {
        if (completion) completion(NO, @"Invalid endpoint URL");
        return;
    }

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    request.timeoutInterval = kAIChatHttpTimeout;

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            if (completion) completion(NO, [NSString stringWithFormat:@"Can't connect: %s", error.localizedDescription.UTF8String]);
            return;
        }
        NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
        if (httpResp.statusCode == 200 || httpResp.statusCode == 405) {
            self.connected = YES;
            if (completion) completion(YES, @"Connected");
        } else {
            if (completion) completion(NO, [NSString stringWithFormat:@"HTTP %ld", (long)httpResp.statusCode]);
        }
    }];
    [task resume];
}

- (void)refreshConnection {
    self.connected = NO;
    [self checkConnectionWithCompletion:^(BOOL connected, NSString* message) {}];
}

@end
#endif
"""

with open('src/common/behaviors/ai_http_client.mm', 'w') as f: f.write(new_http_client)
