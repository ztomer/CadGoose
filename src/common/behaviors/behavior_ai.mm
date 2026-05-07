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

@interface AIChatWindowController : NSWindowController
@property (nonatomic, copy) NSString* gooseName;
@property (nonatomic, strong) NSTextField* inputField;
@property (nonatomic, strong) NSTextView* chatView;
@property (nonatomic, strong) NSButton* sendButton;
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

    return self;
}

- (void)sendMessage:(id)sender {
    NSString* message = self.inputField.stringValue;
    if (message.length == 0) return;

    NSString* chatText = self.chatView.string;
    chatText = [chatText stringByAppendingFormat:@"You: %@\n", message];
    self.chatView.string = chatText;

    self.inputField.stringValue = @"";
}

@end

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