#import "config_gui_helpers.h"
#include "config.h"
#include "mcp_server.h"
#include "ai_text_meme.h"
#include "local_llm.h"

extern "C" void AI_RefreshModelDisplay();

// --- Layout constants ---
static constexpr float kSectionTopY = 35.0f;
static constexpr float kSectionGap = 28.0f;
static constexpr float kControlGap = 32.0f;
static constexpr float kMarginX = 24.0f;
static constexpr float kProviderPopupWidth = 110.0f;
static constexpr float kControlHeight = 24.0f;
static constexpr float kControlHeightSmall = 20.0f;
static constexpr float kPortLabelX = 140.0f;
static constexpr float kPortLabelYOffset = 2.0f;
static constexpr float kPortLabelWidth = 36.0f;
static constexpr float kPortFieldX = 176.0f;
static constexpr float kPortFieldWidth = 50.0f;
static constexpr float kModelLabelX = 236.0f;
static constexpr float kModelLabelWidth = 44.0f;
static constexpr float kModelPopupX = 280.0f;
static constexpr float kModelPopupWidth = 160.0f;
static constexpr float kRefreshBtnX = 444.0f;
static constexpr float kRefreshBtnSize = 24.0f;
static constexpr float kTestBtnWidth = 100.0f;
static constexpr float kTestBtnHeight = 22.0f;
static constexpr float kStatusLabelX = 130.0f;
static constexpr float kStatusLabelYOffset = 2.0f;
static constexpr float kSectionTitleHeight = 16.0f;
static constexpr float kSectionTitleWidth = 200.0f;
static constexpr float kEvilTitleWidth = 80.0f;
static constexpr float kPolandLabelWidth = 120.0f;
static constexpr float kPolandLabelYOffset = 0.0f;
static constexpr float kSliderHeight = 20.0f;
static constexpr float kEvilValueWidth = 40.0f;
static constexpr float kEvilValueHeight = 14.0f;
static constexpr float kEvilValueYOffset = -18.0f;
static constexpr float kPromptTitleHeight = 16.0f;
static constexpr float kPromptBodyHeight = 48.0f;
static constexpr float kPromptBodyCornerRadius = 4.0f;
static constexpr float kTempLabelWidth = 120.0f;
static constexpr float kTempValueWidth = 40.0f;
static constexpr float kTempValueHeight = 14.0f;
static constexpr float kToggleHeight = 18.0f;
static constexpr float kTextMemeBtnWidth = 220.0f;
static constexpr float kAutoSaveBtnWidth = 260.0f;
static constexpr float kMcpBtnWidth = 160.0f;
static constexpr float kMcpPortLabelX = 180.0f;
static constexpr float kMcpPortLabelWidth = 40.0f;
static constexpr float kMcpPortFieldX = 224.0f;
static constexpr float kMcpPortFieldYOffset = -2.0f;
static constexpr float kMcpPortFieldWidth = 70.0f;
static constexpr float kShowStatusBtnWidth = 200.0f;
static constexpr float kEndpointFieldHeight = 22.0f;
static constexpr float kCustomModelFieldHeight = 22.0f;
static constexpr float kCustomModelFieldYOffset = -28.0f;
static constexpr float kCustomSectionYDrop = 56.0f;
static constexpr float kPostCustomYGap = 10.0f;
static constexpr float kPostTestYGap = 40.0f;
static constexpr float kPostSectionYGap = 26.0f;
static constexpr float kPostSectionYGapLarge = 28.0f;
static constexpr float kPostSliderYGap = 10.0f;
static constexpr float kPostToggleYGap = 26.0f;
static constexpr float kPostAutoSaveYGap = 10.0f;
static constexpr float kEvilSliderYGap = 20.0f;
static constexpr float kEvilPromptYGap = 54.0f;
static constexpr float kEvilPostPromptYGap = 10.0f;
static constexpr float kSectionWidth = 200.0f;
static constexpr float kPortFieldTag = 100;
static constexpr float kModelPopupTag = 101;
static constexpr float kEvilValueTag = 200;
static constexpr float kTempValueTag = 301;
static constexpr float kMinMcpPort = 1024;
static constexpr float kMaxMcpPort = 65535;
static constexpr float kDefaultMcpPort = 31072;
static constexpr float kEvilMin = 0.0f;
static constexpr float kEvilMax = 1.0f;
static constexpr float kTempMin = 0.0f;
static constexpr float kTempMax = 2.0f;
static constexpr float kTestTimeout = 5.0f;
static constexpr float kModelRefreshDelay = 0.5f;
static constexpr float kNameFontSize = 12.0f;
static constexpr float kLabelFontSize = 11.0f;
static constexpr float kSmallFontSize = 10.0f;
static constexpr float kBtnFontSize = 11.0f;
static constexpr float kTitleFontSize = 12.0f;
static constexpr float kSectionTitleFontSize = 12.0f;
static constexpr float kPromptFontSize = 11.0f;
static constexpr float kRefreshBtnFontSize = 12.0f;

@implementation AITabView

- (NSInteger)currentProvider {
    return g_config.ai.providerType;
}

- (void)setProvider:(NSInteger)idx {
    g_config.ai.providerType = (int)idx;
}

- (int)currentPort {
     switch ([self currentProvider]) {
         case 0: return 0;
         case 1: return g_config.ai.osaurusPort;
         case 2: return g_config.ai.ollamaPort;
         case 3: return g_config.ai.customPort;
         default: return 0;
     }
 }

- (void)setPort:(int)port {
     switch ([self currentProvider]) {
         case 0: break;
         case 1: g_config.ai.osaurusPort = port; break;
         case 2: g_config.ai.ollamaPort = port; break;
         case 3: g_config.ai.customPort = port; break;
     }
 }

- (NSString*)currentModelName {
    switch ([self currentProvider]) {
        case 0: return @"foundation";
        case 1: return [NSString stringWithUTF8String:g_config.ai.osaurusModel.c_str()];
        case 2: return [NSString stringWithUTF8String:g_config.ai.ollamaModel.c_str()];
        case 3: return [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
        default: return @"foundation";
    }
}

- (void)setModelName:(NSString*)name {
    std::string s = std::string([name UTF8String]);
    switch ([self currentProvider]) {
        case 0: break;
        case 1: g_config.ai.osaurusModel = s; break;
        case 2: g_config.ai.ollamaModel = s; break;
        case 3: g_config.ai.customModel = s; break;
        default: break;
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
    float y = self.bounds.size.height - kSectionTopY;
    CGFloat w = self.bounds.size.width;
    NSInteger prov = [self currentProvider];
    float marginX = kMarginX;

    // --- SECTION: Provider & Network ---
    NSTextField* section1 = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kSectionTitleWidth, kSectionTitleHeight)];
    section1.stringValue = @"CONNECTION";
    section1.font = [NSFont fontWithName:@"Maple Mono" size:kSectionTitleFontSize] ?: [NSFont systemFontOfSize:kSectionTitleFontSize weight:NSFontWeightBold];
    section1.textColor = [NSColor colorWithWhite:0.6 alpha:1.0];
    section1.backgroundColor = [NSColor clearColor];
    section1.bordered = NO; section1.editable = NO;
    [self addSubview:section1];
    
    y -= kSectionGap;

     // Provider selector
     NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(marginX, y, kProviderPopupWidth, kControlHeight)];
     [popup addItemWithTitle:@"Foundation (direct)"];
     [popup addItemWithTitle:@"Osaurus"];
     [popup addItemWithTitle:@"Ollama"];
     [popup addItemWithTitle:@"Custom"];
     [popup selectItemAtIndex:prov];
     popup.target = self;
     popup.action = @selector(providerChanged:);
     [self addSubview:popup];

    // Port
    _portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kPortLabelX, y + kPortLabelYOffset, kPortLabelWidth, kSectionTitleHeight)];
    _portLabel.stringValue = @"Port:";
    _portLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    _portLabel.textColor = [NSColor whiteColor];
    _portLabel.backgroundColor = [NSColor clearColor];
    _portLabel.bordered = NO; _portLabel.editable = NO;
    [self addSubview:_portLabel];

    _portField = [[NSTextField alloc] initWithFrame:NSMakeRect(kPortFieldX, y + kPortLabelYOffset, kPortFieldWidth, kControlHeightSmall)];
    _portField.integerValue = [self currentPort];
    _portField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    _portField.bezelStyle = NSTextFieldRoundedBezel;
    _portField.tag = kPortFieldTag;
    _portField.target = self;
    _portField.action = @selector(portChanged:);
    [self addSubview:_portField];

    // Model popup
    _modelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kModelLabelX, y + kPortLabelYOffset, kModelLabelWidth, kSectionTitleHeight)];
    _modelLabel.stringValue = @"Model:";
    _modelLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    _modelLabel.textColor = [NSColor whiteColor];
    _modelLabel.backgroundColor = [NSColor clearColor];
    _modelLabel.bordered = NO; _modelLabel.editable = NO;
    [self addSubview:_modelLabel];

    _modelPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(kModelPopupX, y, kModelPopupWidth, kControlHeight)];
    [_modelPopup addItemWithTitle:[self currentModelName]];
    _modelPopup.tag = kModelPopupTag;
    _modelPopup.target = self;
    _modelPopup.action = @selector(modelPopupChanged:);
    [self addSubview:_modelPopup];

    _refreshBtn = [[NSButton alloc] initWithFrame:NSMakeRect(kRefreshBtnX, y, kRefreshBtnSize, kRefreshBtnSize)];
    [_refreshBtn setTitle:@"\U0001F504"];
    [_refreshBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kRefreshBtnFontSize] ?: [NSFont systemFontOfSize:kRefreshBtnFontSize]];
    [_refreshBtn setTarget:self];
    [_refreshBtn setAction:@selector(refreshModels:)];
    _refreshBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:_refreshBtn];

    y -= kControlGap;

    // Custom endpoint URL field
    _endpointField = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kEndpointFieldHeight)];
    _endpointField.stringValue = [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
    _endpointField.placeholderString = @"http://localhost:1337/v1/chat/completions";
    _endpointField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    _endpointField.bezelStyle = NSTextFieldRoundedBezel;
    _endpointField.target = self;
    _endpointField.action = @selector(endpointChanged:);
    _endpointField.hidden = (prov != 2 && prov != 3);
    _endpointField.autoresizingMask = NSViewWidthSizable;
    [self addSubview:_endpointField];

    // Custom model name field
    _customModelField = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y + kCustomModelFieldYOffset, w - marginX*2, kCustomModelFieldHeight)];
    _customModelField.stringValue = [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
    _customModelField.placeholderString = @"Model name";
    _customModelField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    _customModelField.bezelStyle = NSTextFieldRoundedBezel;
    _customModelField.target = self;
    _customModelField.action = @selector(customModelChanged:);
    _customModelField.hidden = (prov != 2 && prov != 3);
    _customModelField.autoresizingMask = NSViewWidthSizable;
    [self addSubview:_customModelField];

    if (prov == 2 || prov == 3) y -= kCustomSectionYDrop;

    y -= kPostCustomYGap;

    // Test Connection + status
    NSButton* testBtn = [[NSButton alloc] initWithFrame:NSMakeRect(marginX, y, kTestBtnWidth, kTestBtnHeight)];
    [testBtn setTitle:@"Test Conn"];
    [testBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize]];
    [testBtn setTarget:self];
    [testBtn setAction:@selector(testConnection:)];
    testBtn.bezelStyle = NSBezelStyleRounded;
    testBtn.identifier = @"testConnectionBtn";
    [self addSubview:testBtn];

    _statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kStatusLabelX, y + kStatusLabelYOffset, w - 150, kSectionTitleHeight)];
    _statusLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    _statusLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    _statusLabel.backgroundColor = [NSColor clearColor];
    _statusLabel.bordered = NO; _statusLabel.editable = NO;
    _statusLabel.identifier = @"connectionStatus";
    [self addSubview:_statusLabel];

    y -= kPostTestYGap;
    
    // --- SECTION: Personality ---
    NSTextField* section2 = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kSectionTitleWidth, kSectionTitleHeight)];
    section2.stringValue = @"PERSONALITY";
    section2.font = [NSFont fontWithName:@"Maple Mono" size:kSectionTitleFontSize] ?: [NSFont systemFontOfSize:kSectionTitleFontSize weight:NSFontWeightBold];
    section2.textColor = [NSColor colorWithWhite:0.6 alpha:1.0];
    section2.backgroundColor = [NSColor clearColor];
    section2.bordered = NO; section2.editable = NO;
    [self addSubview:section2];

    y -= kPostSectionYGap;

    // Evil slider
    NSTextField* evilTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kEvilTitleWidth, kSectionTitleHeight)];
    evilTitle.stringValue = @"\U0001F607 Cuddly";
    evilTitle.font = [NSFont fontWithName:@"Maple Mono" size:kNameFontSize] ?: [NSFont systemFontOfSize:kNameFontSize];
    evilTitle.textColor = [NSColor whiteColor];
    evilTitle.backgroundColor = [NSColor clearColor];
    evilTitle.bordered = NO; evilTitle.editable = NO;
    evilTitle.autoresizingMask = NSViewNotSizable;
    [self addSubview:evilTitle];

    NSTextField* polandLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(w - marginX - kPolandLabelWidth, y + kPolandLabelYOffset, kPolandLabelWidth, kSectionTitleHeight)];
    polandLabel.stringValue = @"\U0001F608 Invade Poland";
    polandLabel.font = [NSFont fontWithName:@"Maple Mono" size:kNameFontSize] ?: [NSFont systemFontOfSize:kNameFontSize];
    polandLabel.textColor = [NSColor whiteColor];
    polandLabel.backgroundColor = [NSColor clearColor];
    polandLabel.bordered = NO; polandLabel.editable = NO;
    polandLabel.alignment = NSTextAlignmentRight;
    polandLabel.autoresizingMask = NSViewMinXMargin;
    [self addSubview:polandLabel];

    y -= kEvilSliderYGap;

    NSSlider* evilSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kSliderHeight)];
    evilSlider.minValue = kEvilMin;
    evilSlider.maxValue = kEvilMax;
    evilSlider.doubleValue = g_config.ai.evilLevel;
    evilSlider.target = self;
    evilSlider.action = @selector(evilSliderChanged:);
    evilSlider.autoresizingMask = NSViewWidthSizable;
    evilSlider.continuous = YES;
    [self addSubview:evilSlider];

    NSTextField* evilValue = [[NSTextField alloc] initWithFrame:NSMakeRect(w - marginX - kEvilValueWidth, y + kEvilValueYOffset, kEvilValueWidth, kEvilValueHeight)];
    evilValue.stringValue = [NSString stringWithFormat:@"%.0f%%", g_config.ai.evilLevel * 100];
    evilValue.font = [NSFont fontWithName:@"Maple Mono" size:kSmallFontSize] ?: [NSFont systemFontOfSize:kSmallFontSize];
    evilValue.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    evilValue.backgroundColor = [NSColor clearColor];
    evilValue.bordered = NO; evilValue.editable = NO;
    evilValue.alignment = NSTextAlignmentRight;
    evilValue.autoresizingMask = NSViewMinXMargin;
    evilValue.tag = kEvilValueTag;
    [self addSubview:evilValue];

    y -= kPostSliderYGap;

    NSTextField* promptTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kSectionTitleWidth, kPromptTitleHeight)];
    promptTitle.stringValue = @"\U0001F9E0 System Prompt Preview:";
    promptTitle.font = [NSFont fontWithName:@"Maple Mono" size:kPromptFontSize] ?: [NSFont systemFontOfSize:kPromptFontSize weight:NSFontWeightSemibold];
    promptTitle.textColor = [NSColor colorWithWhite:0.7 alpha:1.0];
    promptTitle.backgroundColor = [NSColor clearColor];
    promptTitle.bordered = NO; promptTitle.editable = NO;
    [self addSubview:promptTitle];

    y -= kEvilPromptYGap;

    _promptBody = [[NSTextView alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kPromptBodyHeight)];
    _promptBody.font = [NSFont fontWithName:@"Maple Mono" size:kPromptFontSize] ?: [NSFont systemFontOfSize:kPromptFontSize];
    _promptBody.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    _promptBody.backgroundColor = [NSColor colorWithWhite:0.1 alpha:0.4];
    _promptBody.wantsLayer = YES;
    _promptBody.layer.cornerRadius = kPromptBodyCornerRadius;
    _promptBody.editable = NO;
    _promptBody.selectable = YES;
    _promptBody.drawsBackground = NO;
    _promptBody.string = [self promptPreviewForEvilLevel:g_config.ai.evilLevel];
    [self addSubview:_promptBody];

    y -= kEvilPostPromptYGap;

    // --- SECTION: TEXT MEME ---
    NSTextField* section3 = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kSectionTitleWidth, kSectionTitleHeight)];
    section3.stringValue = @"TEXT MEME";
    section3.font = [NSFont fontWithName:@"Maple Mono" size:kSectionTitleFontSize] ?: [NSFont systemFontOfSize:kSectionTitleFontSize weight:NSFontWeightBold];
    section3.textColor = [NSColor colorWithWhite:0.6 alpha:1.0];
    section3.backgroundColor = [NSColor clearColor];
    section3.bordered = NO; section3.editable = NO;
    [self addSubview:section3];

    y -= kPostSectionYGap;

    NSButton* textMemeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(marginX, y, kTextMemeBtnWidth, kToggleHeight)];
    [textMemeBtn setButtonType:NSButtonTypeSwitch];
    [textMemeBtn setTitle:@"Generate text memes via AI"];
    [textMemeBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize]];
    [textMemeBtn setState:g_config.ai.textMemeEnabled ? NSControlStateValueOn : NSControlStateValueOff];
    [textMemeBtn setTarget:self];
    [textMemeBtn setAction:@selector(textMemeToggled:)];
    [self addSubview:textMemeBtn];

    y -= kPostToggleYGap;

    NSTextField* tempLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kTempLabelWidth, kSectionTitleHeight)];
    tempLabel.stringValue = @"Temperature";
    tempLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    tempLabel.textColor = [NSColor whiteColor];
    tempLabel.backgroundColor = [NSColor clearColor];
    tempLabel.bordered = NO; tempLabel.editable = NO;
    [self addSubview:tempLabel];

    NSTextField* tempValue = [[NSTextField alloc] initWithFrame:NSMakeRect(w - marginX - kTempValueWidth, y, kTempValueWidth, kTempValueHeight)];
    tempValue.stringValue = [NSString stringWithFormat:@"%.1f", g_config.ai.textMemeTemperature];
    tempValue.font = [NSFont fontWithName:@"Maple Mono" size:kSmallFontSize] ?: [NSFont systemFontOfSize:kSmallFontSize];
    tempValue.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    tempValue.backgroundColor = [NSColor clearColor];
    tempValue.bordered = NO; tempValue.editable = NO;
    tempValue.alignment = NSTextAlignmentRight;
    tempValue.autoresizingMask = NSViewMinXMargin;
    tempValue.tag = kTempValueTag;
    [self addSubview:tempValue];

    y -= kEvilSliderYGap;

    NSSlider* tempSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kSliderHeight)];
    tempSlider.minValue = kTempMin;
    tempSlider.maxValue = kTempMax;
    tempSlider.doubleValue = g_config.ai.textMemeTemperature;
    tempSlider.target = self;
    tempSlider.action = @selector(textMemeTempChanged:);
    tempSlider.autoresizingMask = NSViewWidthSizable;
    tempSlider.continuous = YES;
    [self addSubview:tempSlider];

    y -= kPostToggleYGap;

    NSButton* autoSaveBtn = [[NSButton alloc] initWithFrame:NSMakeRect(marginX, y, kAutoSaveBtnWidth, kToggleHeight)];
    [autoSaveBtn setButtonType:NSButtonTypeSwitch];
    [autoSaveBtn setTitle:@"Auto-save generated texts"];
    [autoSaveBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize]];
    [autoSaveBtn setState:g_config.ai.textMemeAutoSave ? NSControlStateValueOn : NSControlStateValueOff];
    [autoSaveBtn setTarget:self];
    [autoSaveBtn setAction:@selector(textMemeAutoSaveToggled:)];
    [self addSubview:autoSaveBtn];

    y -= kPostAutoSaveYGap;

    // --- SECTION: Advanced ---
    NSTextField* section4 = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kSectionTitleWidth, kSectionTitleHeight)];
    section4.stringValue = @"ADVANCED";
    section4.font = [NSFont fontWithName:@"Maple Mono" size:kSectionTitleFontSize] ?: [NSFont systemFontOfSize:kSectionTitleFontSize weight:NSFontWeightBold];
    section4.textColor = [NSColor colorWithWhite:0.6 alpha:1.0];
    section4.backgroundColor = [NSColor clearColor];
    section4.bordered = NO; section4.editable = NO;
    [self addSubview:section4];

    y -= kPostSectionYGapLarge;

    NSButton* showStatusBtn = [[NSButton alloc] initWithFrame:NSMakeRect(marginX, y, kShowStatusBtnWidth, kToggleHeight)];
    [showStatusBtn setButtonType:NSButtonTypeSwitch];
    [showStatusBtn setTitle:@"Show debug status bar"];
    [showStatusBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize]];
    [showStatusBtn setState:g_config.ai.showStatusBar ? NSControlStateValueOn : NSControlStateValueOff];
    [showStatusBtn setTarget:self];
    [showStatusBtn setAction:@selector(showStatusBarToggled:)];
    [self addSubview:showStatusBtn];

    y -= kPostToggleYGap;

    NSButton* mcpBtn = [[NSButton alloc] initWithFrame:NSMakeRect(marginX, y, kMcpBtnWidth, kToggleHeight)];
    [mcpBtn setButtonType:NSButtonTypeSwitch];
    [mcpBtn setTitle:@"Enable MCP Server"];
    [mcpBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize]];
    [mcpBtn setState:g_config.ai.enableMCP ? NSControlStateValueOn : NSControlStateValueOff];
    [mcpBtn setTarget:self];
    [mcpBtn setAction:@selector(mcpToggled:)];
    [self addSubview:mcpBtn];

    NSTextField* mcpPortLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kMcpPortLabelX, y, kMcpPortLabelWidth, kSectionTitleHeight)];
    mcpPortLabel.stringValue = @"Port:";
    mcpPortLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    mcpPortLabel.textColor = [NSColor colorWithWhite:0.75 alpha:1.0];
    mcpPortLabel.backgroundColor = [NSColor clearColor];
    mcpPortLabel.bordered = NO; mcpPortLabel.editable = NO;
    mcpPortLabel.alignment = NSTextAlignmentRight;
    [self addSubview:mcpPortLabel];

    NSTextField* mcpPortField = [[NSTextField alloc] initWithFrame:NSMakeRect(kMcpPortFieldX, y + kMcpPortFieldYOffset, kMcpPortFieldWidth, kControlHeightSmall)];
    mcpPortField.stringValue = [NSString stringWithFormat:@"%d", g_config.ai.mcpPort];
    mcpPortField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    mcpPortField.bezelStyle = NSTextFieldRoundedBezel;
    mcpPortField.target = self;
    mcpPortField.action = @selector(mcpPortChanged:);
    [self addSubview:mcpPortField];

    [self performSelector:@selector(refreshModels:) withObject:_refreshBtn afterDelay:kModelRefreshDelay];
}

- (void)updateCustomVisibility {
    NSInteger prov = [self currentProvider];
    BOOL isFoundation = (prov == 0);
    BOOL isOsaurus = (prov == 1);
    BOOL isOllama = (prov == 2);
    BOOL isCustom = (prov == 3);
    _portLabel.hidden = isFoundation;
    _portField.hidden = isFoundation;
    _modelLabel.hidden = !(isOsaurus || isOllama);
    _modelPopup.hidden = !(isOsaurus || isOllama);
    _endpointField.hidden = !isCustom;
    _customModelField.hidden = !isCustom;
    _refreshBtn.hidden = !(isOsaurus || isOllama);
}

@end
