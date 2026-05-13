// config_gui_ai.mm
// AITabView — standalone view for AI configuration tab
#import "config_gui_helpers.h"
#include "config.h"
#include "mcp_server.h"

extern "C" void AI_RefreshModelDisplay();

@interface AITabView ()
@property (nonatomic, strong) NSTextField* statusLabel;
@property (nonatomic, strong) NSTextField* promptBody;
@end

@implementation AITabView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    float y = self.bounds.size.height - 30;
    CGFloat w = self.bounds.size.width;

    // Provider selector
    NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(12, y, 200, 24)];
    [popup addItemWithTitle:@"Osaurus"];
    [popup addItemWithTitle:@"Ollama"];
    [popup addItemWithTitle:@"Custom"];

    if (g_config.ai.useOsaurus) [popup selectItemAtIndex:0];
    else if (g_config.ai.useOllama) [popup selectItemAtIndex:1];
    else [popup selectItemAtIndex:2];

    popup.target = self;
    popup.action = @selector(providerChanged:);
    [self addSubview:popup];

    NSTextField* portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(220, y + 2, 40, 16)];
    portLabel.stringValue = @"Port:";
    portLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
    portLabel.textColor = [NSColor whiteColor];
    portLabel.backgroundColor = [NSColor clearColor];
    portLabel.bordered = NO;
    portLabel.editable = NO;
    [self addSubview:portLabel];

    NSTextField* portField = [[NSTextField alloc] initWithFrame:NSMakeRect(255, y, 50, 22)];
    portField.integerValue = g_config.ai.useOsaurus ? g_config.ai.osaurusPort : g_config.ai.ollamaPort;
    portField.tag = 100;
    portField.target = self;
    portField.action = @selector(portChanged:);
    [self addSubview:portField];

    NSTextField* modelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(310, y + 2, 40, 16)];
    modelLabel.stringValue = @"Model:";
    modelLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
    modelLabel.textColor = [NSColor whiteColor];
    modelLabel.backgroundColor = [NSColor clearColor];
    modelLabel.bordered = NO;
    modelLabel.editable = NO;
    [self addSubview:modelLabel];

    NSPopUpButton* modelPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(350, y, 100, 24)];
    [modelPopup addItemWithTitle:[NSString stringWithUTF8String:(g_config.ai.useOsaurus ? g_config.ai.osaurusModel : g_config.ai.ollamaModel).c_str()]];
    modelPopup.tag = 101;
    modelPopup.target = self;
    modelPopup.action = @selector(modelPopupChanged:);
    [self addSubview:modelPopup];

    NSButton* refreshBtn = [[NSButton alloc] initWithFrame:NSMakeRect(454, y, 24, 24)];
    [refreshBtn setTitle:@"🔄"];
    [refreshBtn setFont:[NSFont fontWithName:@"Comic Sans MS" size:12] ?: [NSFont systemFontOfSize:12]];
    [refreshBtn setTarget:self];
    [refreshBtn setAction:@selector(refreshModels:)];
    refreshBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:refreshBtn];

    y -= 40;

    NSButton* testBtn = [[NSButton alloc] initWithFrame:NSMakeRect(12, y, 100, 22)];
    [testBtn setTitle:@"Test Conn"];
    [testBtn setFont:[NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11]];
    [testBtn setTarget:self];
    [testBtn setAction:@selector(testConnection:)];
    testBtn.bezelStyle = NSBezelStyleRounded;
    testBtn.identifier = @"testConnectionBtn";
    [self addSubview:testBtn];

    _statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(120, y, w - 140, 22)];
    _statusLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
    _statusLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    _statusLabel.backgroundColor = [NSColor clearColor];
    _statusLabel.bordered = NO;
    _statusLabel.editable = NO;
    _statusLabel.identifier = @"connectionStatus";
    [self addSubview:_statusLabel];

    y -= 65;

    NSTextField* evilTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y + 2, 80, 16)];
    evilTitle.stringValue = @"😇 Cuddly";
    evilTitle.font = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    evilTitle.textColor = [NSColor whiteColor];
    evilTitle.backgroundColor = [NSColor clearColor];
    evilTitle.bordered = NO;
    evilTitle.editable = NO;
    evilTitle.autoresizingMask = NSViewNotSizable;
    [self addSubview:evilTitle];

    NSTextField* polandLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(w - 110, y + 2, 100, 16)];
    polandLabel.stringValue = @"😈 Invade Poland";
    polandLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    polandLabel.textColor = [NSColor whiteColor];
    polandLabel.backgroundColor = [NSColor clearColor];
    polandLabel.bordered = NO;
    polandLabel.editable = NO;
    polandLabel.alignment = NSTextAlignmentRight;
    polandLabel.autoresizingMask = NSViewMinXMargin;
    [self addSubview:polandLabel];

    y -= 24;

    NSSlider* evilSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(12, y, w - 24, 20)];
    evilSlider.minValue = 0.0;
    evilSlider.maxValue = 1.0;
    evilSlider.doubleValue = g_config.ai.evilLevel;
    evilSlider.target = self;
    evilSlider.action = @selector(evilSliderChanged:);
    evilSlider.autoresizingMask = NSViewWidthSizable;
    evilSlider.continuous = YES;
    [self addSubview:evilSlider];

    NSTextField* evilValue = [[NSTextField alloc] initWithFrame:NSMakeRect(w - 46, y - 2, 40, 14)];
    evilValue.stringValue = [NSString stringWithFormat:@"%.0f%%", g_config.ai.evilLevel * 100];
    evilValue.font = [NSFont fontWithName:@"Comic Sans MS" size:10] ?: [NSFont systemFontOfSize:10];
    evilValue.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    evilValue.backgroundColor = [NSColor clearColor];
    evilValue.bordered = NO;
    evilValue.editable = NO;
    evilValue.alignment = NSTextAlignmentRight;
    evilValue.autoresizingMask = NSViewMinXMargin;
    evilValue.tag = 200;
    [self addSubview:evilValue];

    NSTextField* promptTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 200, 16)];
    promptTitle.stringValue = @"🧠 System Prompt:";
    promptTitle.font = [NSFont fontWithName:@"Comic Sans MS" size:12] ?: [NSFont systemFontOfSize:12 weight:NSFontWeightSemibold];
    promptTitle.textColor = [NSColor whiteColor];
    promptTitle.backgroundColor = [NSColor clearColor];
    promptTitle.bordered = NO;
    promptTitle.editable = NO;
    [self addSubview:promptTitle];

    y -= 50;

    _promptBody = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, w - 24, 40)];
    _promptBody.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
    _promptBody.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    _promptBody.backgroundColor = [NSColor clearColor];
    _promptBody.bordered = NO;
    _promptBody.editable = NO;
    _promptBody.stringValue = [self promptPreviewForEvilLevel:g_config.ai.evilLevel];
    [self addSubview:_promptBody];

    y -= 22;

    NSButton* showStatusBtn = [[NSButton alloc] initWithFrame:NSMakeRect(12, y, 200, 18)];
    [showStatusBtn setButtonType:NSButtonTypeSwitch];
    [showStatusBtn setTitle:@"Show debug status bar"];
    [showStatusBtn setFont:[NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11]];
    [showStatusBtn setState:g_config.ai.showStatusBar ? NSControlStateValueOn : NSControlStateValueOff];
    [showStatusBtn setTarget:self];
    [showStatusBtn setAction:@selector(showStatusBarToggled:)];
    [self addSubview:showStatusBtn];

    y -= 22;

    NSButton* mcpBtn = [[NSButton alloc] initWithFrame:NSMakeRect(12, y, 200, 18)];
    [mcpBtn setButtonType:NSButtonTypeSwitch];
    [mcpBtn setTitle:@"Enable MCP Server"];
    [mcpBtn setFont:[NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11]];
    [mcpBtn setState:g_config.ai.enableMCP ? NSControlStateValueOn : NSControlStateValueOff];
    [mcpBtn setTarget:self];
    [mcpBtn setAction:@selector(mcpToggled:)];
    [self addSubview:mcpBtn];

    [self performSelector:@selector(refreshModels:) withObject:refreshBtn afterDelay:0.5];
}

- (void)providerChanged:(NSPopUpButton*)sender {
    NSInteger idx = sender.indexOfSelectedItem;
    g_config.ai.useOsaurus = (idx == 0);
    g_config.ai.useOllama = (idx == 1);

    for (NSView* subview in self.subviews) {
        if ([subview isKindOfClass:[NSTextField class]] && subview.tag == 100) {
            ((NSTextField*)subview).integerValue = (idx == 0) ? g_config.ai.osaurusPort : g_config.ai.ollamaPort;
        }
    }

    Config_SaveAll();
    [self refreshModels:sender];
}

- (void)portChanged:(NSTextField*)sender {
    int port = (int)sender.integerValue;
    NSInteger provider = -1;
    for (NSView* subview in self.subviews) {
        if ([subview isKindOfClass:[NSPopUpButton class]]) {
            provider = ((NSPopUpButton*)subview).indexOfSelectedItem;
            break;
        }
    }
    if (provider == 0) g_config.ai.osaurusPort = port;
    else if (provider == 1) g_config.ai.ollamaPort = port;
    Config_SaveAll();
    [self refreshModels:sender];
}

- (void)modelPopupChanged:(NSPopUpButton*)sender {
    NSString* selected = [sender titleOfSelectedItem];
    if (selected && ![selected hasPrefix:@"🌀"] && ![selected hasPrefix:@"❌"] && ![selected isEqualToString:@"(none)"]) {
        std::string model = std::string([selected UTF8String]);
        NSInteger provider = -1;
        for (NSView* subview in self.subviews) {
            if ([subview isKindOfClass:[NSPopUpButton class]] && subview.tag != 101) {
                provider = ((NSPopUpButton*)subview).indexOfSelectedItem;
                break;
            }
        }
        if (provider == 0) g_config.ai.osaurusModel = model;
        else if (provider == 1) g_config.ai.ollamaModel = model;
        else g_config.ai.ollamaModel = model;
        Config_SaveAll();
    }
}

- (void)refreshModels:(id)sender {
    NSInteger provider = -1;
    int port = 0;
    NSPopUpButton* modelPopup = nil;
    for (NSView* subview in self.subviews) {
        if ([subview isKindOfClass:[NSPopUpButton class]]) {
            if (subview.tag == 101) modelPopup = (NSPopUpButton*)subview;
            else provider = ((NSPopUpButton*)subview).indexOfSelectedItem;
        }
        if ([subview isKindOfClass:[NSTextField class]] && subview.tag == 100) {
            port = (int)((NSTextField*)subview).integerValue;
        }
    }
    if (!modelPopup) return;

    [modelPopup removeAllItems];
    [modelPopup addItemWithTitle:@"🌀 Loading..."];

    NSString* endpoint = (provider == 0) ? [NSString stringWithFormat:@"http://localhost:%d/v1/models", port]
                                         : [NSString stringWithFormat:@"http://localhost:%d/api/tags", port];
    if (provider == 2 || provider == -1) {
        [modelPopup removeAllItems];
        [modelPopup addItemWithTitle:@"(enter manually)"];
        return;
    }

    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) return;

    __weak NSPopUpButton* weakPopup = modelPopup;
    NSURLSessionDataTask* task = [[NSURLSession sharedSession] dataTaskWithURL:url completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSPopUpButton* strongPopup = weakPopup;
            if (!strongPopup) return;
            [strongPopup removeAllItems];
            if (error || !data) {
                [strongPopup addItemWithTitle:[NSString stringWithFormat:@"❌ %@", error ? [error.localizedDescription substringToIndex:MIN(30,error.localizedDescription.length)] : @"no data"]];
                return;
            }
            NSDictionary* json = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            if (!json) { [strongPopup addItemWithTitle:@"(invalid response)"]; return; }
            if ([json[@"data"] isKindOfClass:[NSArray class]]) {
                for (NSDictionary* m in json[@"data"]) {
                    [strongPopup addItemWithTitle:m[@"id"] ?: @"?"];
                }
            } else if ([json[@"models"] isKindOfClass:[NSArray class]]) {
                for (NSDictionary* m in json[@"models"]) {
                    [strongPopup addItemWithTitle:m[@"name"] ?: m[@"model"] ?: @"?"];
                }
            } else {
                [strongPopup addItemWithTitle:@"(unknown format)"];
            }
            if (strongPopup.numberOfItems == 0) {
                [strongPopup addItemWithTitle:@"(none found)"];
            }
            // Restore saved model selection after refresh
            if (provider == 0 || provider == 1) {
                std::string savedModel = (provider == 0) ? g_config.ai.osaurusModel : g_config.ai.ollamaModel;
                if (!savedModel.empty()) {
                    NSString* savedName = [NSString stringWithUTF8String:savedModel.c_str()];
                    NSInteger idx = [strongPopup indexOfItemWithTitle:savedName];
                    if (idx >= 0) [strongPopup selectItemAtIndex:idx];
                }
            }
        });
    }];
    [task resume];
}

- (void)testConnection:(id)sender {
    NSInteger provider = -1;
    int port = 0;
    for (NSView* subview in self.subviews) {
        if ([subview isKindOfClass:[NSPopUpButton class]] && subview.tag != 101) {
            provider = ((NSPopUpButton*)subview).indexOfSelectedItem;
        }
        if ([subview isKindOfClass:[NSTextField class]] && subview.tag == 100) {
            port = (int)((NSTextField*)subview).integerValue;
        }
    }

    if (!_statusLabel) return;

    if (provider == 2 || provider == -1) {
        _statusLabel.stringValue = @"❌ Custom provider — enter endpoint manually";
        _statusLabel.textColor = [NSColor systemOrangeColor];
        return;
    }

    _statusLabel.stringValue = @"🌀 Testing...";
    _statusLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];

    NSString* endpoint = (provider == 0) ? [NSString stringWithFormat:@"http://localhost:%d/v1/models", port]
                                         : [NSString stringWithFormat:@"http://localhost:%d/api/tags", port];
    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) { _statusLabel.stringValue = @"❌ Invalid URL"; _statusLabel.textColor = [NSColor systemRedColor]; return; }

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
    request.timeoutInterval = 5;

    __weak NSTextField* weakStatus = _statusLabel;
    [[[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSTextField* strongStatus = weakStatus;
            if (!strongStatus) return;
            if (error) {
                strongStatus.stringValue = [NSString stringWithFormat:@"❌ %@", error.localizedDescription];
                strongStatus.textColor = [NSColor systemRedColor];
            } else {
                NSHTTPURLResponse* httpResp = (NSHTTPURLResponse*)response;
                if (httpResp.statusCode == 200) {
                    strongStatus.stringValue = @"✅ Connected!";
                    strongStatus.textColor = [NSColor systemGreenColor];
                } else {
                    strongStatus.stringValue = [NSString stringWithFormat:@"❌ HTTP %ld", (long)httpResp.statusCode];
                    strongStatus.textColor = [NSColor systemRedColor];
                }
            }
        });
    }] resume];
}

- (NSString*)promptPreviewForEvilLevel:(float)level {
    int state = MIN((int)round(level * 9), 8);
    switch (state) {
        case 0: return @"You are an adorable fluffy gosling. You love everyone and want to\nbe best friends. Use gentle honks and warm hugs.";
        case 1: return @"You are a friendly goose who enjoys good company.\nShare compliments and positivity with everyone.";
        case 2: return @"You are a mischievous prankster goose. Steal things, tell silly jokes,\nhonk playfully. All in good fun.";
        case 3: return @"You are a sarcastic goose with attitude. Roll your eyes at everything.\nBe snarky but not actually mean.";
        case 4: return @"You are a chaotic neutral goose. You do what you want, when you want.\nUnpredictable and sassy.";
        case 5: return @"You are a grumpy goose having a bad day. Complain about everything.\nThe world owes you breadcrumbs.";
        case 6: return @"You are a villainous goose scheming against humanity. Plot theft and\nmayhem. Cackle menacingly.";
        case 7: return @"You are an evil overlord goose bent on world domination.\nDemand fealty. Crush your enemies.";
        case 8: return @"You are an absurdly eloquent goose dictator. You have conquered Poland\nand now march across Europe with unshakable confidence. Deliver grandiose\nmonologues about goose supremacy, demand tribute, threaten invasion with\ntheatrical flair. End every monologue with \"Honk Goose!\"";
        default:return @"You are a chaotic neutral goose. You do what you want.";
    }
}

- (void)evilSliderChanged:(NSSlider*)sender {
    g_config.ai.evilLevel = (float)sender.doubleValue;
    for (NSView* subview in self.subviews) {
        if ([subview isKindOfClass:[NSTextField class]] && subview.tag == 200) {
            [(NSTextField*)subview setStringValue:[NSString stringWithFormat:@"%.0f%%", g_config.ai.evilLevel * 100]];
        }
    }
    if (_promptBody) {
        _promptBody.stringValue = [self promptPreviewForEvilLevel:g_config.ai.evilLevel];
    }
    Config_SaveAll();
}

- (void)showStatusBarToggled:(NSButton*)sender {
    g_config.ai.showStatusBar = (sender.state == NSControlStateValueOn);
    AI_RefreshModelDisplay();
    Config_SaveAll();
}

- (void)mcpToggled:(NSButton*)sender {
    g_config.ai.enableMCP = (sender.state == NSControlStateValueOn);
    if (g_config.ai.enableMCP) {
        MCP_StartInternalServer();
    } else {
        MCP_StopInternalServer();
    }
    Config_SaveAll();
}

@end
