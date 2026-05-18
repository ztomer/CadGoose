// ai_http_client.mm
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
        sysPrompt = [NSString stringWithFormat:@"Output JSON now.\n\n%@", sysPrompt];
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

    fprintf(stderr, "[AI] POST %s model=%s temp=%.1f max_tokens=%d turn=%d%s\n",
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
