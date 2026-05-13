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

@interface AIHTTPClient : NSObject
@property (nonatomic, strong) NSMutableArray* history;
- (instancetype)init;
- (NSString*)currentEndpoint;
- (NSString*)currentModel;
- (const struct BuiltinProfile*)currentProfile;
- (void)sendMessage:(NSString*)message completion:(void(^)(NSString* response, NSError* error))completion;
@end

#pragma mark - AIChatWindowController

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

    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 420, 360)
                                             styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskFullSizeContentView
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    self.window.title = [NSString stringWithFormat:@"💬 Chat with %@ 🦆", name];
    self.window.titleVisibility = NSWindowTitleHidden;
    self.window.titlebarAppearsTransparent = YES;
    [[self.window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[self.window standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [self.window center];

    NSView* contentView = self.window.contentView;

    contentView.wantsLayer = YES;
    contentView.layer.backgroundColor = [[NSColor colorWithWhite:0.12 alpha:1.0] CGColor];

    NSFont* chatFont = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13];

    // Appbar: [traffic lights | goose selector | pin]
    NSView* appBar = [[NSView alloc] initWithFrame:NSMakeRect(0, 322, 420, 38)];
    appBar.wantsLayer = YES;
    appBar.layer.backgroundColor = [[NSColor colorWithWhite:0.2 alpha:1.0] CGColor];
    appBar.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    [contentView addSubview:appBar];

    NSView* sep = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 420, 1)];
    sep.wantsLayer = YES;
    sep.layer.backgroundColor = [[NSColor colorWithWhite:0.35 alpha:1.0] CGColor];
    sep.autoresizingMask = NSViewWidthSizable;
    [appBar addSubview:sep];

    // Goose selector popup (replaces centered title)
    self.goosePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(70, 7, appBar.frame.size.width - 70 - 70, 24)];
    self.goosePopup.font = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    self.goosePopup.bezelStyle = NSBezelStyleRounded;
    self.goosePopup.target = self;
    self.goosePopup.action = @selector(gooseSelected:);
    self.goosePopup.autoresizingMask = NSViewWidthSizable | NSViewMaxXMargin;
    [self populateGoosePopup];
    [appBar addSubview:self.goosePopup];

    // Pin button (far right)
    self.pinButton = [[NSButton alloc] initWithFrame:NSMakeRect(appBar.frame.size.width - 30, 9, 20, 20)];
    [self.pinButton setTitle:@"📌"];
    [self.pinButton setFont:[NSFont systemFontOfSize:12]];
    [self.pinButton setTarget:self];
    [self.pinButton setAction:@selector(togglePin:)];
    self.pinButton.bezelStyle = NSBezelStyleInline;
    self.pinButton.autoresizingMask = NSViewMinXMargin;
    [appBar addSubview:self.pinButton];

    // Chat scrollview
    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(10, 60, 400, 214)];
    scrollView.hasVerticalScroller = YES;
    scrollView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    scrollView.drawsBackground = NO;

    NSTextView* textView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 400, 264)];
    textView.editable = NO;
    textView.font = chatFont;
    textView.backgroundColor = [NSColor clearColor];
    textView.string = [NSString stringWithFormat:@"🦆 %@: HONK! What do you want?\n", name];
    self.chatView = textView;
    scrollView.documentView = textView;
    [contentView addSubview:scrollView];

    // Spinner: shown during AI request
    self.spinner = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(3, 2, 12, 12)];
    self.spinner.style = NSProgressIndicatorStyleSpinning;
    self.spinner.controlSize = NSControlSizeSmall;
    self.spinner.hidden = YES;
    self.spinner.autoresizingMask = NSViewMaxXMargin;
    [contentView addSubview:self.spinner];

    // Status bar: model name between chat and input
    self.statusBar = [[NSTextField alloc] initWithFrame:NSMakeRect(18, 2, contentView.frame.size.width - 30, 12)];
    self.statusBar.font = [NSFont fontWithName:@"Comic Sans MS" size:10] ?: [NSFont systemFontOfSize:10];
    self.statusBar.textColor = [NSColor colorWithWhite:0.5 alpha:1.0];
    self.statusBar.backgroundColor = [NSColor clearColor];
    self.statusBar.bordered = NO;
    self.statusBar.editable = NO;
    self.statusBar.autoresizingMask = NSViewWidthSizable;
    [contentView addSubview:self.statusBar];

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

    self.httpClient = [[AIHTTPClient alloc] init];
    self.window.delegate = self;
    [self updateModelDisplay];

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
    [self.spinner startAnimation:nil];
    self.spinner.hidden = NO;

    __weak AIChatWindowController* weakSelf = self;
    [self.httpClient sendMessage:message completion:^(NSString* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            AIChatWindowController* strong = weakSelf;
            if (!strong) return;
            strong.sendButton.enabled = YES;
            if (error || !response) {
                NSString* fallback = s_fallbackResponseForMessage(message, strong.gooseName);
                [strong appendResponse:fallback];
                fprintf(stderr, "[AI] Using fallback response (error: %s)\n",
                        error.localizedDescription.UTF8String ?: "nil");
            } else {
                [strong appendResponse:response];
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
    self.statusBar.stringValue = [NSString stringWithFormat:@"\u2699 %@", model];
    self.statusBar.hidden = !g_config.ai.showStatusBar;
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    [self updateModelDisplay];
}

@end

#pragma mark - AIHTTPClient Implementation

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
    {nullptr,       0.8, 200, 30,  false, false}, // default (sentinel)
};

static const BuiltinProfile* DefaultProfile() {
    int count = sizeof(s_profiles) / sizeof(s_profiles[0]);
    return &s_profiles[count - 1]; // last entry (sentinel with nullptr pattern)
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

@implementation AIHTTPClient

- (instancetype)init {
    self = [super init];
    if (self) {
        _history = [NSMutableArray array];
    }
    return self;
}

- (NSString*)currentEndpoint {
    if (g_config.ai.useUnixSocket)
        return [NSString stringWithUTF8String:g_config.ai.unixSocketPath.c_str()];
    switch (g_config.ai.providerType) {
        case 0: return [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.osaurusPort];
        case 1: return [NSString stringWithFormat:@"http://localhost:%d/v1/chat/completions", g_config.ai.ollamaPort];
        case 2: return [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
        default: return @"http://localhost:1337/v1/chat/completions";
    }
}

- (NSString*)currentModel {
    switch (g_config.ai.providerType) {
        case 0: return [NSString stringWithUTF8String:g_config.ai.osaurusModel.c_str()];
        case 1: return [NSString stringWithUTF8String:g_config.ai.ollamaModel.c_str()];
        case 2: return [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
        default: return @"foundation";
    }
}

- (const BuiltinProfile*)currentProfile {
    return MatchProfile([self currentModel].UTF8String);
}

- (NSString*)systemPromptForEvilLevel:(float)level {
    int state = MIN((int)round(level * 9), 8);
    switch (state) {
        case 0: return @"You are an adorable fluffy gosling. You love everyone and want to be best friends. "
                       @"Use gentle honks and warm hugs. Give sweet, wholesome responses. Keep responses short.";
        case 1: return @"You are a friendly goose who enjoys good company. Share compliments and positivity. "
                       @"Be warm and kind. Keep responses short.";
        case 2: return @"You are a mischievous prankster goose. Steal things, tell silly jokes, honk playfully. "
                       @"All in good fun, never actually mean. Keep responses short.";
        case 3: return @"You are a sarcastic goose with attitude. Roll your eyes at everything. "
                       @"Be snarky and clever but not actually mean. Keep responses short.";
        case 4: return @"You are a chaotic neutral goose. You do what you want, when you want. "
                       @"Unpredictable and sassy. Never boring. Keep responses short.";
        case 5: return @"You are a grumpy goose having a bad day. Complain about everything. "
                       @"The world owes you breadcrumbs. Be irritable. Keep responses short.";
        case 6: return @"You are a villainous goose scheming against humanity. Plot theft and mayhem. "
                       @"Cackle menacingly. Be dramatic about your evil plans. Keep responses short.";
        case 7: return @"You are an evil overlord goose bent on world domination. Demand fealty. "
                       @"Threaten your enemies. Be grandiose and cruel. Keep responses short.";
case 8: return @"You are an absurdly eloquent goose dictator. You have conquered Poland and now march across Europe with unshakable confidence. Deliver grandiose monologues about goose supremacy, demand tribute from all nations, threaten invasion with theatrical flair, and speak like a delusional yet charismatic despot. Be verbose, dramatic, and magnificently unhinged. End every monologue with \"Honk Goose!\"";
        default:return @"You are a chaotic neutral goose. You do what you want. Keep responses short.";
    }
}

- (void)sendMessage:(NSString*)message completion:(void(^)(NSString*, NSError*))completion {
    [self.history addObject:@{@"role": @"user", @"content": message}];

    NSString* model = [self currentModel];
    const BuiltinProfile* profile = [self currentProfile];

    if (g_config.ai.useUnixSocket) {
        [self sendMessageViaUnixSocket:model profile:profile completion:completion];
        return;
    }

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

    NSDictionary* body = @{
        @"model": model,
        @"messages": messagesWithSystem,
        @"max_tokens": @(profile->maxTokens),
        @"temperature": @(profile->temperature)
    };

    NSError* jsonError;
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:body options:0 error:&jsonError];
    if (jsonError) {
        if (completion) completion(@"HONK! Brain scrambled.", jsonError);
        return;
    }
    [request setHTTPBody:jsonData];

    fprintf(stderr, "[AI] POST %s model=%s temp=%.1f max_tokens=%d\n",
            endpoint.UTF8String, model.UTF8String, profile->temperature, profile->maxTokens);

    NSURLSession* session = [NSURLSession sharedSession];
    NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        if (error) {
            fprintf(stderr, "[AI] Request failed: %s\n", error.localizedDescription.UTF8String);
            if (completion) completion(@"🦆 HONK! The brain is sleeping. Check if your AI server is running.", error);
            return;
        }

        NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
        fprintf(stderr, "[AI] Response status: %ld\n", (long)httpResp.statusCode);
        NSString* rawBody = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        fprintf(stderr, "[AI] Response body: %s\n", rawBody.UTF8String ?: "nil");
        if (httpResp.statusCode != 200) {
            NSString* body = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            fprintf(stderr, "[AI] Error body: %s\n", body.UTF8String ?: "nil");
            NSError* httpError = [NSError errorWithDomain:@"HTTP" code:httpResp.statusCode userInfo:@{NSLocalizedDescriptionKey: @"Non-200 response from AI server"}];
            if (completion) completion(@"🦆 HONK! The goose can't reach its brain. Check provider/port in settings.", httpError);
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
            if (content && content.length > 0) {
                [self.history addObject:@{@"role": @"assistant", @"content": content}];
                if (completion) completion(content, nil);
                return;
            }
            if (profile->hasReasoningContent) {
                NSString* reasoning = msg[@"reasoning_content"];
                if (reasoning && reasoning.length > 0) {
                    fprintf(stderr, "[AI] Empty content, using reasoning_content (%lu chars)\n",
                            (unsigned long)reasoning.length);
                    [self.history addObject:@{@"role": @"assistant", @"content": reasoning}];
                    if (completion) completion(reasoning, nil);
                    return;
                }
            }
        }

        if (completion) completion(@"HONK! No answer from brain.", nil);
    }];
    [task resume];
}

#pragma mark - Unix Socket Transport

- (void)sendMessageViaUnixSocket:(NSString*)model profile:(const BuiltinProfile*)profile completion:(void(^)(NSString*, NSError*))completion {
    NSString* pathString = [NSString stringWithUTF8String:g_config.ai.unixSocketPath.c_str()];
    const char* socketPath = pathString.UTF8String;

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (completion) completion(@"HONK! Can't create socket.", [NSError errorWithDomain:@"UnixSocket" code:-1 userInfo:@{NSLocalizedDescriptionKey: @"socket() failed"}]);
            });
            return;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            dispatch_async(dispatch_get_main_queue(), ^{
                if (completion) completion(@"HONK! Can't connect to socket.", [NSError errorWithDomain:@"UnixSocket" code:-2 userInfo:@{NSLocalizedDescriptionKey: @"connect() failed"}]);
            });
            return;
        }

        NSString* sysPrompt = [self systemPromptForEvilLevel:g_config.ai.evilLevel];
        if (profile->prependJsonTrigger) {
            sysPrompt = [NSString stringWithFormat:@"Output JSON now.\n\n%@", sysPrompt];
        }
        NSArray* messagesWithSystem = [@[@{@"role": @"system", @"content": sysPrompt}] arrayByAddingObjectsFromArray:self.history];

        NSDictionary* bodyDict = @{
            @"model": model,
            @"messages": messagesWithSystem,
            @"max_tokens": @(profile->maxTokens),
            @"temperature": @(profile->temperature)
        };
        NSError* jsonError;
        NSData* jsonData = [NSJSONSerialization dataWithJSONObject:bodyDict options:0 error:&jsonError];
        if (jsonError) {
            close(sock);
            dispatch_async(dispatch_get_main_queue(), ^{
                if (completion) completion(@"HONK! Brain scrambled.", jsonError);
            });
            return;
        }

        NSString* httpBody = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
        NSString* httpRequest = [NSString stringWithFormat:
            @"POST /v1/chat/completions HTTP/1.1\r\n"
            @"Host: localhost\r\n"
            @"Content-Type: application/json\r\n"
            @"Content-Length: %lu\r\n"
            @"Connection: close\r\n"
            @"\r\n"
            @"%@",
            (unsigned long)httpBody.length,
            httpBody];

        NSData* requestData = [httpRequest dataUsingEncoding:NSUTF8StringEncoding];
        ssize_t sent = send(sock, requestData.bytes, requestData.length, 0);
        if (sent != (ssize_t)requestData.length) {
            close(sock);
            dispatch_async(dispatch_get_main_queue(), ^{
                if (completion) completion(@"HONK! Failed to send.", [NSError errorWithDomain:@"UnixSocket" code:-3 userInfo:@{NSLocalizedDescriptionKey: @"send() failed"}]);
            });
            return;
        }

        struct timeval tv;
        tv.tv_sec = profile->timeoutSecs;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        NSMutableData* responseData = [NSMutableData data];
        char buf[4096];
        ssize_t n;
        while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
            [responseData appendBytes:buf length:n];
        }
        close(sock);

        NSString* responseStr = [[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding];
        if (!responseStr) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (completion) completion(@"HONK! No answer from brain.", nil);
            });
            return;
        }

        NSRange headerEnd = [responseStr rangeOfString:@"\r\n\r\n"];
        if (headerEnd.location == NSNotFound) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (completion) completion(@"HONK! Bad response.", nil);
            });
            return;
        }
        NSString* body = [responseStr substringFromIndex:headerEnd.location + 4];

        NSRange firstLineEnd = [responseStr rangeOfString:@"\r\n"];
        if (firstLineEnd.location != NSNotFound) {
            NSString* statusLine = [responseStr substringToIndex:firstLineEnd.location];
            NSArray* parts = [statusLine componentsSeparatedByString:@" "];
            if (parts.count >= 2) {
                int statusCode = [parts[1] intValue];
                if (statusCode != 200) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (completion) completion([NSString stringWithFormat:@"🦆 HONK! The goose can't reach its brain. Check provider/port in settings. (HTTP %d)", statusCode], nil);
                    });
                    return;
                }
            }
        }

        NSData* bodyData = [body dataUsingEncoding:NSUTF8StringEncoding];
        NSError* parseError;
        NSDictionary* json = [NSJSONSerialization JSONObjectWithData:bodyData options:0 error:&parseError];
        if (!json) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (completion) completion(@"🦆 HONK! The goose speaks nonsense.", parseError);
            });
            return;
        }

        NSArray* choices = json[@"choices"];
        NSString* reply = nil;
        if (choices && choices.count > 0) {
            NSDictionary* msg = choices[0][@"message"];
            NSString* content = msg[@"content"];
            if (content && content.length > 0) {
                reply = content;
            } else if (profile->hasReasoningContent) {
                NSString* reasoning = msg[@"reasoning_content"];
                if (reasoning && reasoning.length > 0) {
                    reply = reasoning;
                }
            }
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            if (reply) {
                [self.history addObject:@{@"role": @"assistant", @"content": reply}];
                if (completion) completion(reply, nil);
            } else {
                if (completion) completion(@"HONK! No answer from brain.", nil);
            }
        });
    });
}

@end
#endif

#ifdef __APPLE__
static AIHTTPClient* g_httpClient = nil;
static AIChatWindowController* g_chatController = nil;
#endif

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
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_aiBehavior);