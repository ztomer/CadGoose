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

    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 420, 360)
                                             styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    self.window.title = [NSString stringWithFormat:@"💬 Chat with %@ 🦆", name];
    self.window.titlebarAppearsTransparent = YES;
    [[self.window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[self.window standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [self.window center];

    NSView* contentView = self.window.contentView;

    // Vibrant glass background
    NSVisualEffectView* glass = [[NSVisualEffectView alloc] initWithFrame:contentView.bounds];
    glass.material = NSVisualEffectMaterialSidebar;
    glass.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    glass.state = NSVisualEffectStateActive;
    glass.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [contentView addSubview:glass positioned:NSWindowBelow relativeTo:nil];

    NSFont* chatFont = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13];

    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 60, 400, 260)];
    scrollView.hasVerticalScroller = YES;
    scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scrollView.drawsBackground = NO;

    NSTextView* textView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 400, 260)];
    textView.editable = NO;
    textView.font = chatFont;
    textView.backgroundColor = [NSColor clearColor];
    textView.string = [NSString stringWithFormat:@"🦆 %@: HONK! What do you want?\n", name];
    self.chatView = textView;
    scrollView.documentView = textView;
    [contentView addSubview:scrollView];

    self.inputField = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 18, 340, 28)];
    self.inputField.placeholderString = @"Type your message...";
    self.inputField.font = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13];
    [self.inputField setTarget:self];
    [self.inputField setAction:@selector(sendMessage:)];
    self.inputField.bezelStyle = NSTextFieldRoundedBezel;
    self.inputField.autoresizingMask = NSViewWidthSizable;
    [contentView addSubview:self.inputField];

    self.sendButton = [[NSButton alloc] initWithFrame:NSMakeRect(360, 16, 50, 28)];
    [self.sendButton setTitle:@"🪿"];
    [self.sendButton setFont:[NSFont systemFontOfSize:16]];
    [self.sendButton setTarget:self];
    [self.sendButton setAction:@selector(sendMessage:)];
    self.sendButton.bezelStyle = NSBezelStyleRounded;
    self.sendButton.autoresizingMask = NSViewMinXMargin;
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

    fprintf(stderr, "[AI] POST %s model=%s\n", self.endpoint.UTF8String, self.model.UTF8String);

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            fprintf(stderr, "[AI] Request failed: %s\n", error.localizedDescription.UTF8String);
            if (completion) completion(@"🦆 HONK! The brain is sleeping. Check if your AI server is running.", error);
            return;
        }

        NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
        fprintf(stderr, "[AI] Response status: %ld\n", (long)httpResp.statusCode);
        if (httpResp.statusCode != 200) {
            NSString* body = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            fprintf(stderr, "[AI] Error body: %s\n", body.UTF8String ?: "nil");
            if (completion) completion(@"🦆 HONK! The goose can't reach its brain. Check provider/port in settings.", nil);
            return;
        }

        NSError* parseError;
        NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseError];
        if (parseError) {
            NSString* body = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            fprintf(stderr, "[AI] JSON parse error: %s body: %s\n", parseError.localizedDescription.UTF8String, body.UTF8String ?: "nil");
            if (completion) completion(@"🦆 HONK! The goose speaks nonsense. Try a different model.", parseError);
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
#endif

static AIHTTPClient* g_httpClient = nil;
static AIChatWindowController* g_chatController = nil;

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

extern "C" void AI_OpenChat(const char* gooseName) {
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

extern "C" void AI_CloseChat() {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_chatController) {
            [g_chatController.window close];
            g_chatController = nil;
        }
    });
}

extern "C" void AI_SendMessage(const char* message) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_chatController) {
            g_chatController.inputField.stringValue = [NSString stringWithUTF8String:message];
            [g_chatController sendMessage:nil];
        }
    });
}

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AIState>(ctx.goose->id, "ai");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
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