// ===========================
// behavior_ai.cpp
// AI Behavior - Chat with goose via OpenAI
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <string>
#include <vector>
#include <cstring>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>

#pragma mark - AIHTTPClient

@interface AIHTTPClient : NSObject
@property (nonatomic, copy) NSString* endpoint;
@property (nonatomic, copy) NSString* model;
@property (nonatomic, strong) NSMutableArray* history;
- (instancetype)initWithEndpoint:(NSString*)endpoint model:(NSString*)model;
- (void)sendMessage:(NSString*)message completion:(void(^)(NSString* response, NSError* error))completion;
@end

#pragma mark - AIChatWindowController

@interface AIChatWindowController : NSWindowController
@property (nonatomic, copy) NSString* gooseName;
@property (nonatomic, strong) NSTextField* inputField;
@property (nonatomic, strong) NSTextView* chatView;
@property (nonatomic, strong) NSButton* sendButton;
@property (nonatomic, strong) AIHTTPClient* httpClient;
- (void)sendMessage:(id)sender;
- (void)appendResponse:(NSString*)response;
@end

@implementation AIChatWindowController

- (instancetype)initWithGooseName:(NSString*)name {
    self.gooseName = name;

    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 400, 300)
                                             styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    self.window.title = [NSString stringWithFormat:@"Chat with %@", name];
    [self.window center];

    NSView* contentView = self.window.contentView;
    [contentView setWantsLayer:YES];

    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 50, 380, 200)];
    scrollView.hasVerticalScroller = YES;

    NSTextView* textView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 380, 200)];
    textView.editable = NO;
    textView.font = [NSFont systemFontOfSize:13];
    textView.string = [NSString stringWithFormat:@"%@: HONK! What do you want?\n", name];
    self.chatView = textView;
    scrollView.documentView = textView;
    [contentView addSubview:scrollView];

    self.inputField = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 20, 310, 24)];
    self.inputField.placeholderString = @"Type your message...";
    [contentView addSubview:self.inputField];

    self.sendButton = [[NSButton alloc] initWithFrame:NSMakeRect(330, 18, 60, 28)];
    [self.sendButton setTitle:@"Send"];
    [self.sendButton setTarget:self];
    [self.sendButton setAction:@selector(sendMessage:)];
    [contentView addSubview:self.sendButton];

    NSString* endpoint;
    NSString* model;

    if (g_config.ai.useOsaurus) {
        endpoint = [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.osaurusPort];
        model = [NSString stringWithUTF8String:g_config.ai.osaurusModel.c_str()];
    } else if (g_config.ai.useOllama) {
        endpoint = [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.ollamaPort];
        model = [NSString stringWithUTF8String:g_config.ai.ollamaModel.c_str()];
    } else {
        endpoint = @"http://localhost:1337/v1/chat/completions";
        model = @"llama3";
    }

    self.httpClient = [[AIHTTPClient alloc] initWithEndpoint:endpoint model:model];

    return self;
}

- (void)sendMessage:(id)sender {
    NSString* message = self.inputField.stringValue;
    if (message.length == 0) return;

    NSString* chatText = self.chatView.string;
    chatText = [chatText stringByAppendingFormat:@"You: %@\n", message];
    self.chatView.string = chatText;
    self.chatView.string = [self.chatView.string stringByAppendingString:@"Goose: HONK...\n"];

    self.sendButton.enabled = NO;

    __weak AIChatWindowController* weakSelf = self;
    [self.httpClient sendMessage:message completion:^(NSString* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            AIChatWindowController* strong = weakSelf;
            if (!strong) return;
            strong.sendButton.enabled = YES;
            if (error) {
                [strong appendResponse:@"HONK! Something went wrong."];
            } else {
                [strong appendResponse:response ? response : @"HONK! No response."];
            }
        });
    }];

    self.inputField.stringValue = @"";
}

- (void)appendResponse:(NSString*)response {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSString* current = self.chatView.string;
        NSRange honkRange = [current rangeOfString:@"Goose: HONK..."];
        if (honkRange.location != NSNotFound) {
            NSString* before = [current substringToIndex:honkRange.location];
            self.chatView.string = [before stringByAppendingFormat:@"Goose: %@\n\n", response];
        }
    });
}

@end

#pragma mark - AIHTTPClient Implementation

@implementation AIHTTPClient

- (instancetype)initWithEndpoint:(NSString*)endpoint model:(NSString*)model {
    self = [super init];
    if (self) {
        _endpoint = endpoint;
        _model = model;
        _history = [NSMutableArray array];
    }
    return self;
}

- (void)sendMessage:(NSString*)message completion:(void(^)(NSString*, NSError*))completion {
    [self.history addObject:@{@"role": @"user", @"content": message}];

    NSURL* url = [NSURL URLWithString:self.endpoint];
    if (!url) {
        if (completion) completion(@"HONK! Can't reach the brain.", nil);
        return;
    }

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    [request setHTTPMethod:@"POST"];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];

    NSDictionary* body = @{
        @"model": self.model,
        @"messages": self.history,
        @"max_tokens": @(200),
        @"temperature": @(0.8)
    };

    NSError* jsonError;
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:body options:0 error:&jsonError];
    if (jsonError) {
        if (completion) completion(@"HONK! Brain scrambled.", jsonError);
        return;
    }
    [request setHTTPBody:jsonData];

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            if (completion) completion(@"HONK! Something went wrong.", error);
            return;
        }

        NSError* parseError;
        NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseError];
        if (parseError) {
            if (completion) completion(@"HONK! Can't understand.", parseError);
            return;
        }

        NSArray* choices = json[@"choices"];
        if (choices && choices.count > 0) {
            NSDictionary* msg = choices[0][@"message"];
            NSString* content = msg[@"content"];
            if (content) {
                [self.history addObject:@{@"role": @"assistant", @"content": content}];
                if (completion) completion(content, nil);
                return;
            }
        }

        if (completion) completion(@"HONK! No answer from brain.", nil);
    }];
    [task resume];
}

@end

static AIHTTPClient* g_httpClient = nil;

static AIChatWindowController* g_chatController = nil;

extern "C" void AI_ShowChatWindow(const char* gooseName) {
    dispatch_async(dispatch_get_main_queue(), ^{
        NSString* name = [NSString stringWithUTF8String:gooseName ? gooseName : "Goose"];

        if (g_chatController) {
            [g_chatController.window makeKeyAndOrderFront:nil];
        } else {
            g_chatController = [[AIChatWindowController alloc] initWithGooseName:name];
            [g_chatController.window makeKeyAndOrderFront:nil];
        }

        [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
    });
}
#endif

static bool s_enabled = true;
static bool s_chatOpen = false;
static std::string s_pendingQuestion = "";
static std::string s_lastResponse = "";
static double s_lastResponseTime = 0.0f;

struct AIState : public BehaviorState {
    std::vector<std::string> conversationHistory;
    double lastQuestionTime = 0;
    bool awaitingResponse = false;

    void Reset() override {
        conversationHistory.clear();
        lastQuestionTime = 0;
        awaitingResponse = false;
    }
};

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AIState>(ctx.goose->id, "ai");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.systems.ai) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AIState>(goose->id, "ai");

    if (s_chatOpen && !s_pendingQuestion.empty() && !state->awaitingResponse) {
        state->awaitingResponse = true;
        state->lastQuestionTime = time;
        s_lastResponse = "HONK! I'm thinking...";
        s_lastResponseTime = time;
        state->awaitingResponse = false;
        s_pendingQuestion = "";
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.systems.ai) return;
}

void AI_OpenChat(const char* gooseName) {
    s_chatOpen = true;
    AI_ShowChatWindow(gooseName);
}

void AI_SendMessage(const char* message) {
    s_pendingQuestion = message;
}

void AI_CloseChat() {
    s_chatOpen = false;
    s_pendingQuestion = "";
}

static Behavior g_aiBehavior = {
    .id = "ai",
    .name = "AI",
    .description = "Chat with the goose using AI. Enable and click goose to open chat.",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_aiBehavior);