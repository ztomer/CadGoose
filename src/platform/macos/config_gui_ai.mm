// config_gui_ai.mm
// AITabView — standalone view for AI configuration tab
#import "config_gui_helpers.h"
#include "config.h"
#include "mcp_server.h"

extern "C" void AI_RefreshModelDisplay();

@interface AITabView ()
@property (nonatomic, strong) NSTextField* statusLabel;
@property (nonatomic, strong) NSTextField* promptBody;
@property (nonatomic, strong) NSTextField* endpointField;
@property (nonatomic, strong) NSTextField* customModelField;
@property (nonatomic, strong) NSTextField* portLabel;
@property (nonatomic, strong) NSTextField* portField;
@property (nonatomic, strong) NSTextField* modelLabel;
@property (nonatomic, strong) NSPopUpButton* modelPopup;
@property (nonatomic, strong) NSButton* refreshBtn;
@end

@implementation AITabView

- (NSInteger)currentProvider {
    return g_config.ai.providerType;
}

- (void)setProvider:(NSInteger)idx {
    g_config.ai.providerType = (int)idx;
}

- (int)currentPort {
    switch ([self currentProvider]) {
        case 0: return g_config.ai.osaurusPort;
        case 1: return g_config.ai.ollamaPort;
        default: return g_config.ai.customPort;
    }
}

- (void)setPort:(int)port {
    switch ([self currentProvider]) {
        case 0: g_config.ai.osaurusPort = port; break;
        case 1: g_config.ai.ollamaPort = port; break;
        default: g_config.ai.customPort = port; break;
    }
}

- (NSString*)currentModelName {
    switch ([self currentProvider]) {
        case 0: return [NSString stringWithUTF8String:g_config.ai.osaurusModel.c_str()];
        case 1: return [NSString stringWithUTF8String:g_config.ai.ollamaModel.c_str()];
        default: return [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
    }
}

- (void)setModelName:(NSString*)name {
    std::string s = std::string([name UTF8String]);
    switch ([self currentProvider]) {
        case 0: g_config.ai.osaurusModel = s; break;
        case 1: g_config.ai.ollamaModel = s; break;
        default: g_config.ai.customModel = s; break;
    }
}

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
    NSInteger prov = [self currentProvider];

    // Provider selector
    NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(12, y, 120, 24)];
    [popup addItemWithTitle:@"Osaurus"];
    [popup addItemWithTitle:@"Ollama"];
    [popup addItemWithTitle:@"Custom"];
    [popup selectItemAtIndex:prov];
    popup.target = self;
    popup.action = @selector(providerChanged:);
    [self addSubview:popup];

    // Port
    _portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(140, y + 2, 36, 16)];
    _portLabel.stringValue = @"Port:";
    _portLabel.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
    _portLabel.textColor = [NSColor whiteColor];
    _portLabel.backgroundColor = [NSColor clearColor];
    _portLabel.bordered = NO;
    _portLabel.editable = NO;
    [self addSubview:_portLabel];

    _portField = [[NSTextField alloc] initWithFrame:NSMakeRect(176, y, 50, 22)];
    _portField.integerValue = [self currentPort];
    _portField.tag = 100;
    _portField.target = self;
    _portField.action = @selector(portChanged:);
    [self addSubview:_portField];

    // Model label + popup
    _modelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(236, y + 2, 44, 16)];
    _modelLabel.stringValue = @"Model:";
    _modelLabel.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
    _modelLabel.textColor = [NSColor whiteColor];
    _modelLabel.backgroundColor = [NSColor clearColor];
    _modelLabel.bordered = NO;
    _modelLabel.editable = NO;
    [self addSubview:_modelLabel];

    _modelPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(280, y, 160, 24)];
    [_modelPopup addItemWithTitle:[self currentModelName]];
    _modelPopup.tag = 101;
    _modelPopup.target = self;
    _modelPopup.action = @selector(modelPopupChanged:);
    [self addSubview:_modelPopup];

    _refreshBtn = [[NSButton alloc] initWithFrame:NSMakeRect(444, y, 24, 24)];
    [_refreshBtn setTitle:@"🔄"];
    [_refreshBtn setFont:[NSFont fontWithName:@"Maple Mono" size:12] ?: [NSFont systemFontOfSize:12]];
    [_refreshBtn setTarget:self];
    [_refreshBtn setAction:@selector(refreshModels:)];
    _refreshBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:_refreshBtn];

    y -= 30;

    // Custom endpoint URL field (only shown for Custom provider)
    _endpointField = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, w - 24, 22)];
    _endpointField.stringValue = [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
    _endpointField.placeholderString = @"http://localhost:1337/v1/chat/completions";
    _endpointField.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
    _endpointField.bezelStyle = NSTextFieldRoundedBezel;
    _endpointField.target = self;
    _endpointField.action = @selector(endpointChanged:);
    _endpointField.hidden = (prov != 2);
    _endpointField.autoresizingMask = NSViewWidthSizable;
    [self addSubview:_endpointField];

    // Custom model name field (only shown for Custom provider)
    _customModelField = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y - 26, w - 24, 22)];
    _customModelField.stringValue = [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
    _customModelField.placeholderString = @"Model name";
    _customModelField.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
    _customModelField.bezelStyle = NSTextFieldRoundedBezel;
    _customModelField.target = self;
    _customModelField.action = @selector(customModelChanged:);
    _customModelField.hidden = (prov != 2);
    _customModelField.autoresizingMask = NSViewWidthSizable;
    [self addSubview:_customModelField];

    if (prov == 2) y -= 56;

    y -= 10;

    // Test Connection + status
    NSButton* testBtn = [[NSButton alloc] initWithFrame:NSMakeRect(12, y, 100, 22)];
    [testBtn setTitle:@"Test Conn"];
    [testBtn setFont:[NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11]];
    [testBtn setTarget:self];
    [testBtn setAction:@selector(testConnection:)];
    testBtn.bezelStyle = NSBezelStyleRounded;
    testBtn.identifier = @"testConnectionBtn";
    [self addSubview:testBtn];

    _statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(120, y, w - 140, 22)];
    _statusLabel.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
    _statusLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    _statusLabel.backgroundColor = [NSColor clearColor];
    _statusLabel.bordered = NO;
    _statusLabel.editable = NO;
    _statusLabel.identifier = @"connectionStatus";
    [self addSubview:_statusLabel];

    y -= 65;

    // Evil slider
    NSTextField* evilTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y + 2, 80, 16)];
    evilTitle.stringValue = @"😇 Cuddly";
    evilTitle.font = [NSFont fontWithName:@"Maple Mono" size:13] ?: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    evilTitle.textColor = [NSColor whiteColor];
    evilTitle.backgroundColor = [NSColor clearColor];
    evilTitle.bordered = NO;
    evilTitle.editable = NO;
    evilTitle.autoresizingMask = NSViewNotSizable;
    [self addSubview:evilTitle];

    NSTextField* polandLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(w - 110, y + 2, 100, 16)];
    polandLabel.stringValue = @"😈 Invade Poland";
    polandLabel.font = [NSFont fontWithName:@"Maple Mono" size:13] ?: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
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
    evilValue.font = [NSFont fontWithName:@"Maple Mono" size:10] ?: [NSFont systemFontOfSize:10];
    evilValue.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    evilValue.backgroundColor = [NSColor clearColor];
    evilValue.bordered = NO;
    evilValue.editable = NO;
    evilValue.alignment = NSTextAlignmentRight;
    evilValue.autoresizingMask = NSViewMinXMargin;
    evilValue.tag = 200;
    [self addSubview:evilValue];

    y -= 24;

    NSTextField* promptTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 200, 16)];
    promptTitle.stringValue = @"🧠 System Prompt:";
    promptTitle.font = [NSFont fontWithName:@"Maple Mono" size:12] ?: [NSFont systemFontOfSize:12 weight:NSFontWeightSemibold];
    promptTitle.textColor = [NSColor whiteColor];
    promptTitle.backgroundColor = [NSColor clearColor];
    promptTitle.bordered = NO;
    promptTitle.editable = NO;
    [self addSubview:promptTitle];

    y -= 50;

    _promptBody = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, w - 24, 40)];
    _promptBody.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
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
    [showStatusBtn setFont:[NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11]];
    [showStatusBtn setState:g_config.ai.showStatusBar ? NSControlStateValueOn : NSControlStateValueOff];
    [showStatusBtn setTarget:self];
    [showStatusBtn setAction:@selector(showStatusBarToggled:)];
    [self addSubview:showStatusBtn];

    y -= 22;

    // MCP Port field
    NSTextField* mcpPortLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(24, y + 2, 40, 16)];
    mcpPortLabel.stringValue = @"Port:";
    mcpPortLabel.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
    mcpPortLabel.textColor = [NSColor colorWithWhite:0.75 alpha:1.0];
    mcpPortLabel.backgroundColor = [NSColor clearColor];
    mcpPortLabel.bordered = NO;
    mcpPortLabel.editable = NO;
    mcpPortLabel.alignment = NSTextAlignmentRight;
    [self addSubview:mcpPortLabel];

    NSTextField* mcpPortField = [[NSTextField alloc] initWithFrame:NSMakeRect(66, y, 70, 22)];
    mcpPortField.stringValue = [NSString stringWithFormat:@"%d", g_config.ai.mcpPort];
    mcpPortField.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
    mcpPortField.bezelStyle = NSTextFieldRoundedBezel;
    mcpPortField.target = self;
    mcpPortField.action = @selector(mcpPortChanged:);
    mcpPortField.autoresizingMask = NSViewMaxXMargin;
    [self addSubview:mcpPortField];

    y -= 22;

    NSButton* mcpBtn = [[NSButton alloc] initWithFrame:NSMakeRect(12, y, 200, 18)];
    [mcpBtn setButtonType:NSButtonTypeSwitch];
    [mcpBtn setTitle:@"Enable MCP Server"];
    [mcpBtn setFont:[NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11]];
    [mcpBtn setState:g_config.ai.enableMCP ? NSControlStateValueOn : NSControlStateValueOff];
    [mcpBtn setTarget:self];
    [mcpBtn setAction:@selector(mcpToggled:)];
    [self addSubview:mcpBtn];



    [self performSelector:@selector(refreshModels:) withObject:_refreshBtn afterDelay:0.5];
}
- (void)updateCustomVisibility {
    NSInteger prov = [self currentProvider];
    BOOL isCustom = (prov == 2);
    _endpointField.hidden = !isCustom;
    _customModelField.hidden = !isCustom;
    _portLabel.hidden = isCustom;
    _portField.hidden = isCustom;
    _modelLabel.hidden = isCustom;
    _modelPopup.hidden = isCustom;
    _refreshBtn.hidden = isCustom;
}

- (void)providerChanged:(NSPopUpButton*)sender {
    NSInteger idx = sender.indexOfSelectedItem;
    [self setProvider:idx];
    _portField.integerValue = [self currentPort];
    [_modelPopup removeAllItems];
    [_modelPopup addItemWithTitle:[self currentModelName]];
    [self updateCustomVisibility];
    Config_SaveAll();
    [self refreshModels:sender];
}

- (void)portChanged:(NSTextField*)sender {
    [self setPort:(int)sender.integerValue];
    Config_SaveAll();
    [self refreshModels:sender];
}

- (void)modelPopupChanged:(NSPopUpButton*)sender {
    NSString* selected = [sender titleOfSelectedItem];
    if (selected && ![selected hasPrefix:@"🌀"] && ![selected hasPrefix:@"❌"] && ![selected isEqualToString:@"(none)"]) {
        [self setModelName:selected];
        Config_SaveAll();
    }
}

- (void)endpointChanged:(NSTextField*)sender {
    g_config.ai.customEndpoint = std::string([sender.stringValue UTF8String]);
    Config_SaveAll();
}

- (void)customModelChanged:(NSTextField*)sender {
    g_config.ai.customModel = std::string([sender.stringValue UTF8String]);
    Config_SaveAll();
}

- (void)refreshModels:(id)sender {
    NSInteger prov = [self currentProvider];
    if (prov == 2) return; // no model list for custom

    int port = [self currentPort];

    [_modelPopup removeAllItems];
    [_modelPopup addItemWithTitle:@"🌀 Loading..."];

    NSString* endpoint = (prov == 0) ? [NSString stringWithFormat:@"http://localhost:%d/v1/models", port]
                                     : [NSString stringWithFormat:@"http://localhost:%d/api/tags", port];
    NSURL* url = [NSURL URLWithString:endpoint];
    if (!url) return;

    __weak NSPopUpButton* weakPopup = _modelPopup;
    __weak AITabView* weakSelf = self;
    NSURLSessionDataTask* task = [[NSURLSession sharedSession] dataTaskWithURL:url completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSPopUpButton* strongPopup = weakPopup;
            AITabView* strongSelf = weakSelf;
            if (!strongPopup || !strongSelf) return;
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
            // Restore saved model selection
            if (prov == 0 || prov == 1) {
                std::string savedModel = (prov == 0) ? g_config.ai.osaurusModel : g_config.ai.ollamaModel;
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
- (NSString*)modelsEndpointForTest {
    NSInteger prov = [self currentProvider];
    if (prov == 2) return [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
    return (prov == 0) ? [NSString stringWithFormat:@"http://localhost:%d/v1/models", [self currentPort]]
                       : [NSString stringWithFormat:@"http://localhost:%d/api/tags", [self currentPort]];
}

- (void)testConnection:(id)sender {
    if (!_statusLabel) return;

    _statusLabel.stringValue = @"🌀 Testing...";
    _statusLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];

    NSString* endpoint = [self modelsEndpointForTest];
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
        MCP_StartHTTPServer();
    } else {
        MCP_StopHTTPServer();
        MCP_StopInternalServer();
    }
    Config_SaveAll();
}

- (void)mcpPortChanged:(NSTextField*)sender {
    int port = [sender.stringValue intValue];
    if (port < 1024 || port > 65535) port = 31072;
    g_config.ai.mcpPort = port;
    Config_SaveAll();
}

@end
