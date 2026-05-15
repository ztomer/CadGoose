// ===========================
// behavior_ai.cpp
// AI Behavior - Chat with goose via OpenAI
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "ai_mcp_bridge.h"
#include <string>
#include <vector>
#include <cstring>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <sys/socket.h>
#import <sys/un.h>


#pragma mark - Foundation Fallback

static NSString* s_fallbackResponseForMessage(NSString* message, NSString* gooseName) {
    @autoreleasepool {
        if (message.length == 0) return @"HONK! Silence speaks volumes... HONK!";

        NSLinguisticTagger* tagger = [[NSLinguisticTagger alloc] initWithTagSchemes:@[NSLinguisticTagSchemeLexicalClass] options:0];
        [tagger setString:message];

        __block NSMutableSet* keywords = [NSMutableSet set];
        [tagger enumerateTagsInRange:NSMakeRange(0, message.length)
                              scheme:NSLinguisticTagSchemeLexicalClass
                             options:NSLinguisticTaggerOmitWhitespace | NSLinguisticTaggerOmitPunctuation | NSLinguisticTaggerOmitOther
                          usingBlock:^(NSString* _Nullable tag, NSRange tokenRange, NSRange sentenceRange, BOOL* _Nonnull stop) {
            NSString* word = [[message substringWithRange:tokenRange] lowercaseString];
            if ([tag isEqualToString:NSLinguisticTagNoun] || [tag isEqualToString:NSLinguisticTagVerb] || [tag isEqualToString:NSLinguisticTagAdjective] || [tag isEqualToString:NSLinguisticTagAdverb] || [tag isEqualToString:NSLinguisticTagPronoun] || [tag isEqualToString:NSLinguisticTagInterjection] || [tag isEqualToString:@"OtherWord"]) {
                [keywords addObject:word];
            }
        }];

        if ([keywords containsObject:@"honk"]) return @"HONK! HONK HONK! 🦆";
        if ([keywords containsObject:@"sad"] || [keywords containsObject:@"mad"] || [keywords containsObject:@"angry"])
            return @"HONK! Why so mad? Have some bread crumbs. 🍞";
        if ([keywords containsObject:@"happy"] || [keywords containsObject:@"love"] || [keywords containsObject:@"great"])
            return @"HONK! You seem happy! Me too! HONK! 🎉";
        if ([keywords intersectsSet:[NSSet setWithObjects:@"food", @"bread", @"seed", @"feed", @"eat", nil]])
            return @"HONK! FOOD?! Where?! HONK HONK! 🍞";
        if ([keywords containsObject:@"name"]) return [NSString stringWithFormat:@"HONK! I'm %@, a goose! HONK!", gooseName ?: @"a goose"];
        if ([keywords containsObject:@"goose"]) return @"HONK! Yes, I'm a goose! The best goose! HONK!";
        if ([keywords containsObject:@"bye"] || [keywords containsObject:@"goodbye"])
            return @"HONK! Bye bye! Don't forget to feed the geese! 🦆";
        if ([keywords containsObject:@"hello"] || [keywords containsObject:@"hi"] || [keywords containsObject:@"hey"])
            return @"HONK! Hello to you too! 🦆";
        if ([keywords containsObject:@"help"] || [keywords containsObject:@"what"])
            return @"HONK! I'm a goose. I walk, I honk, I steal things. What else do you need to know?";
        if ([keywords containsObject:@"dance"] || [keywords containsObject:@"spin"])
            return @"HONK! *goose does a little dance* HONK HONK! 🕺";
        if ([keywords containsObject:@"sing"] || [keywords containsObject:@"song"])
            return @"HONK HONK HOOOOONK! *a beautiful goose song* 🎶";

        static NSArray* defaults = @[
            @"HONK! 🦆",
            @"HONK HONK!",
            @"🦆 The goose acknowledges your presence. HONK!",
            @"HONK! I'm a goose, what did you expect?",
            @"*goose tilts head* HONK?",
            @"HONK! That's nice. Anyway, HONK!",
            @"HONK! I don't understand but I support you!",
            @"🦆 HONK! Have you tried turning it off and on again?",
        ];
        return defaults[arc4random_uniform((uint32_t)defaults.count)];
    }
}

#pragma mark - AIHTTPClient

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

@interface AIHTTPClient : NSObject
@property (nonatomic, strong) NSMutableArray* history;
@property (nonatomic) BOOL connected;
- (instancetype)init;
- (NSString*)currentEndpoint;
- (NSString*)currentModel;
- (const struct BuiltinProfile*)currentProfile;
- (void)sendMessage:(NSString*)message completion:(void(^)(NSString* response, NSError* error))completion;
- (void)checkConnectionWithCompletion:(void(^)(BOOL connected, NSString* message))completion;
- (void)refreshConnection;
@end

#pragma mark - AIChatWindowController

// --- Layout constants ---
static constexpr float kChatWindowWidth = 420.0f;
static constexpr float kChatWindowHeight = 360.0f;
static constexpr float kAppBarHeight = 38.0f;
static constexpr float kAppBarY = kChatWindowHeight - kAppBarHeight - 0.0f;
static constexpr float kSeparatorHeight = 1.0f;
static constexpr float kGoosePopupLeftMargin = 70.0f;
static constexpr float kGoosePopupRightReserved = 70.0f;
static constexpr float kGoosePopupHeight = 24.0f;
static constexpr float kGoosePopupY = 7.0f;
static constexpr float kPinButtonRightMargin = 30.0f;
static constexpr float kPinButtonY = 9.0f;
static constexpr float kPinButtonSize = 20.0f;
static constexpr float kPinButtonFontSize = 12.0f;
static constexpr float kScrollViewMarginX = 10.0f;
static constexpr float kInputAreaHeight = 46.0f;
static constexpr float kScrollViewY = kInputAreaHeight + 10.0f;
static constexpr float kScrollViewWidth = kChatWindowWidth - 20.0f;
static constexpr float kScrollViewHeight = kChatWindowHeight - kAppBarHeight - kInputAreaHeight - 28.0f;
static constexpr float kTextViewWidth = kScrollViewWidth;
static constexpr float kTextViewHeight = kScrollViewHeight + 6.0f;
static constexpr float kSpinnerX = 3.0f;
static constexpr float kSpinnerY = 2.0f;
static constexpr float kSpinnerSize = 12.0f;
static constexpr float kStatusBarMarginX = 18.0f;
static constexpr float kStatusBarY = 2.0f;
static constexpr float kStatusBarRightMargin = 30.0f;
static constexpr float kStatusBarHeight = 12.0f;
static constexpr float kStatusBarFontSize = 10.0f;
static constexpr float kInputFieldMarginX = 12.0f;
static constexpr float kInputFieldY = 16.0f;
static constexpr float kInputFieldWidth = kChatWindowWidth - 2 * kInputFieldMarginX - 58.0f;
static constexpr float kInputFieldHeight = 30.0f;
static constexpr float kSendButtonX = kInputFieldMarginX + kInputFieldWidth + 6.0f;
static constexpr float kSendButtonY = kInputFieldY;
static constexpr float kSendButtonWidth = 46.0f;
static constexpr float kSendButtonHeight = kInputFieldHeight;
static constexpr float kSendButtonFontSize = 16.0f;
static constexpr float kChatFontSize = 13.0f;
static constexpr float kChatFontWeight = 13.0f;

@interface AIChatWindowController : NSWindowController <NSWindowDelegate>
@property (nonatomic, copy) NSString* gooseName;
@property (nonatomic, strong) NSTextField* inputField;
@property (nonatomic, strong) NSTextView* chatView;
@property (nonatomic, strong) NSButton* sendButton;
@property (nonatomic, strong) NSButton* pinButton;
@property (nonatomic, strong) NSPopUpButton* goosePopup;
@property (nonatomic, strong) NSTextField* statusBar;
@property (nonatomic, strong) NSProgressIndicator* spinner;
@property (nonatomic, strong) AIHTTPClient* httpClient;
- (void)sendMessage:(id)sender;
- (void)appendResponse:(NSString*)response;
- (void)togglePin:(id)sender;
- (void)gooseSelected:(id)sender;
- (void)populateGoosePopup;
- (void)updateModelDisplay;
@end

@implementation AIChatWindowController

- (instancetype)initWithGooseName:(NSString*)name {
    self.gooseName = name;

    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, kChatWindowWidth, kChatWindowHeight)
                                             styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskFullSizeContentView
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    self.window.title = [NSString stringWithFormat:@"💬 Chat with %@ 🦆", name];
    self.window.titleVisibility = NSWindowTitleHidden;
    self.window.titlebarAppearsTransparent = YES;
    self.window.backgroundColor = [NSColor clearColor];
    self.window.opaque = NO;
    [[self.window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[self.window standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [self.window center];

    NSView* contentView = self.window.contentView;

    // Apple-tier liquid glass background
    NSVisualEffectView* visualEffectView = [[NSVisualEffectView alloc] initWithFrame:contentView.bounds];
    visualEffectView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    visualEffectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    visualEffectView.material = NSVisualEffectMaterialUnderWindowBackground;
    visualEffectView.state = NSVisualEffectStateActive;
    [contentView addSubview:visualEffectView];

    NSFont* chatFont = [NSFont fontWithName:@"Maple Mono" size:kChatFontSize] ?: [NSFont systemFontOfSize:kChatFontSize];

    // Appbar: [traffic lights | goose selector | pin] with liquid glass
    NSView* appBar = [[NSView alloc] initWithFrame:NSMakeRect(0, kAppBarY, kChatWindowWidth, kAppBarHeight)];
    appBar.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    [contentView addSubview:appBar];

    // Liquid glass effect for appbar
    NSVisualEffectView* appBarGlass = [[NSVisualEffectView alloc] initWithFrame:appBar.bounds];
    appBarGlass.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    appBarGlass.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    appBarGlass.material = NSVisualEffectMaterialTitlebar;
    appBarGlass.state = NSVisualEffectStateActive;
    [appBar addSubview:appBarGlass];

    NSView* sep = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, kChatWindowWidth, kSeparatorHeight)];
    sep.wantsLayer = YES;
    sep.layer.backgroundColor = [[NSColor separatorColor] CGColor];
    sep.autoresizingMask = NSViewWidthSizable;
    [appBar addSubview:sep];

    // Goose selector popup (replaces centered title)
    self.goosePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(kGoosePopupLeftMargin, kGoosePopupY, appBar.frame.size.width - kGoosePopupLeftMargin - kGoosePopupRightReserved, kGoosePopupHeight)];
    self.goosePopup.font = [NSFont fontWithName:@"Maple Mono" size:kChatFontSize] ?: [NSFont systemFontOfSize:kChatFontSize weight:NSFontWeightSemibold];
    self.goosePopup.bezelStyle = NSBezelStyleRounded;
    self.goosePopup.target = self;
    self.goosePopup.action = @selector(gooseSelected:);
    self.goosePopup.autoresizingMask = NSViewWidthSizable | NSViewMaxXMargin;
    [self populateGoosePopup];
    [appBar addSubview:self.goosePopup];

    // Pin button (far right)
    self.pinButton = [[NSButton alloc] initWithFrame:NSMakeRect(appBar.frame.size.width - kPinButtonRightMargin, kPinButtonY, kPinButtonSize, kPinButtonSize)];
    [self.pinButton setTitle:@"📌"];
    [self.pinButton setFont:[NSFont systemFontOfSize:kPinButtonFontSize]];
    [self.pinButton setTarget:self];
    [self.pinButton setAction:@selector(togglePin:)];
    self.pinButton.bezelStyle = NSBezelStyleInline;
    self.pinButton.autoresizingMask = NSViewMinXMargin;
    [appBar addSubview:self.pinButton];

    // Chat scrollview - fills space between input and appbar (no empty gap)
    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(kScrollViewMarginX, kScrollViewY, kScrollViewWidth, kScrollViewHeight)];
    scrollView.hasVerticalScroller = YES;
    scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scrollView.drawsBackground = NO;

    NSTextView* textView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, kTextViewWidth, kTextViewHeight)];
    textView.editable = NO;
    textView.font = chatFont;
    textView.backgroundColor = [NSColor clearColor];
    textView.string = [NSString stringWithFormat:@"🦆 %@: HONK! What do you want?\n", name];
    self.chatView = textView;
    scrollView.documentView = textView;
    [contentView addSubview:scrollView];

    // Spinner: shown during AI request
    self.spinner = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(kSpinnerX, kSpinnerY, kSpinnerSize, kSpinnerSize)];
    self.spinner.style = NSProgressIndicatorStyleSpinning;
    self.spinner.controlSize = NSControlSizeSmall;
    self.spinner.hidden = YES;
    self.spinner.autoresizingMask = NSViewMaxXMargin;
    [contentView addSubview:self.spinner];

    // Status bar: model name between chat and input
    self.statusBar = [[NSTextField alloc] initWithFrame:NSMakeRect(kStatusBarMarginX, kStatusBarY, contentView.frame.size.width - kStatusBarMarginX - kStatusBarRightMargin, kStatusBarHeight)];
    self.statusBar.font = [NSFont fontWithName:@"Maple Mono" size:kStatusBarFontSize] ?: [NSFont systemFontOfSize:kStatusBarFontSize];
    self.statusBar.textColor = [NSColor colorWithWhite:0.5 alpha:1.0];
    self.statusBar.backgroundColor = [NSColor clearColor];
    self.statusBar.bordered = NO;
    self.statusBar.editable = NO;
    self.statusBar.autoresizingMask = NSViewWidthSizable;
    [contentView addSubview:self.statusBar];

    self.inputField = [[NSTextField alloc] initWithFrame:NSMakeRect(kInputFieldMarginX, kInputFieldY, kInputFieldWidth, kInputFieldHeight)];
    self.inputField.placeholderString = @"Type your message...";
    self.inputField.font = [NSFont fontWithName:@"Maple Mono" size:kChatFontSize] ?: [NSFont systemFontOfSize:kChatFontSize];
    [self.inputField setTarget:self];
    [self.inputField setAction:@selector(sendMessage:)];
    self.inputField.bezelStyle = NSTextFieldRoundedBezel;
    self.inputField.controlSize = NSControlSizeLarge;
    self.inputField.autoresizingMask = NSViewWidthSizable;
    [contentView addSubview:self.inputField];

    self.sendButton = [[NSButton alloc] initWithFrame:NSMakeRect(kSendButtonX, kSendButtonY, kSendButtonWidth, kSendButtonHeight)];
    [self.sendButton setTitle:@"🪿"];
    [self.sendButton setFont:[NSFont systemFontOfSize:kSendButtonFontSize]];
    [self.sendButton setTarget:self];
    [self.sendButton setAction:@selector(sendMessage:)];
    self.sendButton.bezelStyle = NSBezelStyleRounded;
    self.sendButton.controlSize = NSControlSizeLarge;
    self.sendButton.autoresizingMask = NSViewMinXMargin;
    [contentView addSubview:self.sendButton];

    self.httpClient = [[AIHTTPClient alloc] init];
    self.window.delegate = self;
    [self updateModelDisplay];
    [self checkConnection];

    return self;
}

- (void)checkConnection {
    __weak AIChatWindowController* weakSelf = self;
    [self.httpClient checkConnectionWithCompletion:^(BOOL connected, NSString* message) {
        dispatch_async(dispatch_get_main_queue(), ^{
            AIChatWindowController* strong = weakSelf;
            if (!strong) return;
            if (!connected) {
                strong.statusBar.stringValue = [NSString stringWithFormat:@"⚠️ %@", message];
                fprintf(stderr, "[AI] Connection check failed: %s\n", message.UTF8String);
            } else {
                fprintf(stderr, "[AI] Connection OK\n");
                [strong updateModelDisplay];
            }
        });
    }];
}

- (void)sendMessage:(id)sender {
    NSString* message = self.inputField.stringValue;
    if (message.length == 0) return;

    // Update awaiting state
    auto* aiState = BehaviorStateManager::Instance().GetOrCreate<AIState>(0, "ai");
    aiState->awaitingResponse = true;

    NSString* chatText = self.chatView.string;
    chatText = [chatText stringByAppendingFormat:@"You: %@\n", message];
    self.chatView.string = chatText;
    self.chatView.string = [self.chatView.string stringByAppendingString:@"Goose: HONK...\n"];

    self.sendButton.enabled = NO;
    [self.spinner startAnimation:nil];
    self.spinner.hidden = NO;

    __weak AIChatWindowController* weakSelf = self;
    [self.httpClient sendMessage:message completion:^(NSString* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            AIChatWindowController* strong = weakSelf;
            if (!strong) return;
            auto* s = BehaviorStateManager::Instance().GetOrCreate<AIState>(0, "ai");
            s->awaitingResponse = false;
            strong.sendButton.enabled = YES;
            if (response && response.length > 0 && !error) {
                [strong appendResponse:response];
            } else {
                std::string cmdResponse;
                std::string msgStr([message UTF8String]);
                if (AI_TryMCPCommand(msgStr, cmdResponse)) {
                    NSString* mcpResponse = [NSString stringWithUTF8String:cmdResponse.c_str()];
                    [strong appendResponse:mcpResponse];
                    fprintf(stderr, "[AI] Handled as MCP command: %s\n", msgStr.c_str());
                } else if (response && response.length > 0) {
                    [strong appendResponse:response];
                    fprintf(stderr, "[AI] LLM error displayed: %s\n", response.UTF8String);
                } else {
                    NSString* fallback = s_fallbackResponseForMessage(message, strong.gooseName);
                    [strong appendResponse:fallback];
                    fprintf(stderr, "[AI] Using fallback response\n");
                }
            }
            [strong.spinner stopAnimation:nil];
            strong.spinner.hidden = YES;
            [strong updateModelDisplay];
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

- (void)togglePin:(id)sender {
    if (self.window.level == NSFloatingWindowLevel) {
        self.window.level = NSNormalWindowLevel;
        self.pinButton.title = @"📌";
    } else {
        self.window.level = NSFloatingWindowLevel;
        self.pinButton.title = @"📍";
    }
}

- (void)populateGoosePopup {
    [self.goosePopup removeAllItems];
    for (const auto& g : g_geese) {
        NSString* gName = [NSString stringWithUTF8String:g.name.c_str()];
        [self.goosePopup addItemWithTitle:gName];
        if ([gName isEqualToString:self.gooseName]) {
            [self.goosePopup selectItemAtIndex:self.goosePopup.numberOfItems - 1];
        }
    }
    if (self.goosePopup.numberOfItems == 0) {
        [self.goosePopup addItemWithTitle:self.gooseName ?: @"Goose"];
        [self.goosePopup selectItemAtIndex:0];
    }
}

- (void)gooseSelected:(id)sender {
    NSString* selected = [self.goosePopup titleOfSelectedItem];
    if (selected) {
        self.gooseName = selected;
    }
}

- (void)updateModelDisplay {
    NSString* model = [self.httpClient currentModel];
    NSString* dot = self.httpClient.connected ? @"●" : @"○";
    self.statusBar.stringValue = [NSString stringWithFormat:@"\u2699 %@  %@", model, dot];
    self.statusBar.textColor = self.httpClient.connected ? [NSColor systemGreenColor] : [NSColor systemRedColor];
    self.statusBar.hidden = !g_config.ai.showStatusBar;
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    [self.httpClient refreshConnection];
    [self updateModelDisplay];
}

@end
#endif

#ifdef __APPLE__
static AIHTTPClient* g_httpClient = nil;
static AIChatWindowController* g_chatController = nil;
#endif

extern "C" void AI_OpenChat(const char* gooseName) {
#ifdef __APPLE__
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
#else
    fprintf(stderr, "[AI] Chat UI not implemented on this platform\n");
#endif
}

extern "C" void AI_RefreshModelDisplay() {
#ifdef __APPLE__
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_chatController updateModelDisplay];
    });
#endif
}

extern "C" void AI_CloseChat() {
#ifdef __APPLE__
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_chatController) {
            [g_chatController.window close];
            g_chatController = nil;
        }
    });
#endif
}

extern "C" void AI_SendMessage(const char* message) {
#ifdef __APPLE__
    dispatch_async(dispatch_get_main_queue(), ^{
        if (g_chatController) {
            g_chatController.inputField.stringValue = [NSString stringWithUTF8String:message];
            [g_chatController sendMessage:nil];
        }
    });
#else
    fprintf(stderr, "[AI] Cannot send message: Chat UI not implemented\n");
#endif
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
    .configPtr = &g_config.behaviors.systems.ai,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_aiBehavior);