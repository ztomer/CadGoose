#import "config_gui_helpers.h"
#include "config.h"
#include "mcp_server.h"
#include "ai_text_meme.h"
#include "local_llm.h"
#include "behaviors/ai_prompt_builder.h"

#include "config_gui_ai_layout.h"

extern "C" void AI_RefreshModelDisplay();

// --- Layout constants ---

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

    y = [self setupConnectionSectionWithY:y width:w];


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
    evilTitle.stringValue = @"Cuddly";
    evilTitle.font = [NSFont fontWithName:@"Maple Mono" size:kNameFontSize] ?: [NSFont systemFontOfSize:kNameFontSize];
    evilTitle.textColor = [NSColor whiteColor];
    evilTitle.backgroundColor = [NSColor clearColor];
    evilTitle.bordered = NO; evilTitle.editable = NO;
    evilTitle.autoresizingMask = NSViewNotSizable;
    [self addSubview:evilTitle];

    NSTextField* polandLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(w - marginX - kPolandLabelWidth, y + kPolandLabelYOffset, kPolandLabelWidth, kSectionTitleHeight)];
    polandLabel.stringValue = @"Invade Poland";
    polandLabel.font = [NSFont fontWithName:@"Maple Mono" size:kNameFontSize] ?: [NSFont systemFontOfSize:kNameFontSize];
    polandLabel.textColor = [NSColor whiteColor];
    polandLabel.backgroundColor = [NSColor clearColor];
    polandLabel.bordered = NO; polandLabel.editable = NO;
    polandLabel.alignment = NSTextAlignmentRight;
    polandLabel.autoresizingMask = NSViewMinXMargin;
    [self addSubview:polandLabel];

    y -= kEvilSliderYGap;

    // Evil slider: centered between "Cuddly" (left) and "Invade Poland" (right) labels
    // Left label ends at: marginX + kEvilTitleWidth + 8 (gap)
    // Right label starts at: w - marginX - kPolandLabelWidth - 8 (gap)
    // Value label takes kEvilValueWidth + 8 (gap) on the right
    CGFloat sliderLeftX = marginX + kEvilTitleWidth + 8;
    CGFloat sliderRightX = w - marginX - kPolandLabelWidth - 8 - kEvilValueWidth - 8;
    CGFloat sliderW = sliderRightX - sliderLeftX;
    _personalitySliderWidth = sliderW;
    _personalitySliderLeftX = sliderLeftX;
    NSSlider* evilSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(sliderLeftX, y, sliderW, kSliderHeight)];
    evilSlider.minValue = kEvilMin;
    evilSlider.maxValue = kEvilMax;
    evilSlider.doubleValue = g_config.ai.evilLevel;
    evilSlider.target = self;
    evilSlider.action = @selector(evilSliderChanged:);
    evilSlider.autoresizingMask = NSViewWidthSizable;
    evilSlider.continuous = YES;
    [self addSubview:evilSlider];

    NSTextField* evilValue = [[NSTextField alloc] initWithFrame:NSMakeRect(sliderRightX + 8, y + kEvilValueYOffset, kEvilValueWidth, kEvilValueHeight)];
    evilValue.stringValue = [NSString stringWithFormat:@"%.0f%%", g_config.ai.evilLevel * 100];
    evilValue.font = [NSFont fontWithName:@"Maple Mono" size:kSmallFontSize] ?: [NSFont systemFontOfSize:kSmallFontSize];
    evilValue.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    evilValue.backgroundColor = [NSColor clearColor];
    evilValue.bordered = NO; evilValue.editable = NO;
    evilValue.alignment = NSTextAlignmentLeft;
    evilValue.autoresizingMask = NSViewMinXMargin;
    evilValue.tag = kEvilValueTag;
    [self addSubview:evilValue];

    y -= kPostSliderYGap;
    y -= kEvilSliderYGap;

    // Temperature slider: same size and position as personality (evil) slider
    NSSlider* tempSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(_personalitySliderLeftX, y, _personalitySliderWidth, kSliderHeight)];
    tempSlider.minValue = kTempMin;
    tempSlider.maxValue = kTempMax;
    tempSlider.doubleValue = g_config.ai.textMemeTemperature;
    tempSlider.target = self;
    tempSlider.action = @selector(textMemeTempChanged:);
    tempSlider.autoresizingMask = NSViewWidthSizable;
    tempSlider.continuous = YES;
    [self addSubview:tempSlider];

    // Temp label: left of slider, aligned with "Cuddly" (marginX), vertically centered on slider
    NSTextField* tempLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y + 3, kTempLabelWidth, kSectionTitleHeight)];
    tempLabel.stringValue = @"Temperature";
    tempLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    tempLabel.textColor = [NSColor whiteColor];
    tempLabel.backgroundColor = [NSColor clearColor];
    tempLabel.bordered = NO; tempLabel.editable = NO;
    tempLabel.alignment = NSTextAlignmentLeft;
    [self addSubview:tempLabel];

    // Temp value: right of slider, vertically centered on slider
    NSTextField* tempValue = [[NSTextField alloc] initWithFrame:NSMakeRect(_personalitySliderLeftX + _personalitySliderWidth + 8, y + 3, kTempValueWidth, kTempValueHeight)];
    tempValue.stringValue = [NSString stringWithFormat:@"%.1f", g_config.ai.textMemeTemperature];
    tempValue.font = [NSFont fontWithName:@"Maple Mono" size:kSmallFontSize] ?: [NSFont systemFontOfSize:kSmallFontSize];
    tempValue.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    tempValue.backgroundColor = [NSColor clearColor];
    tempValue.bordered = NO; tempValue.editable = NO;
    tempValue.alignment = NSTextAlignmentLeft;
    tempValue.autoresizingMask = NSViewMinXMargin;
    tempValue.tag = kTempValueTag;
    [self addSubview:tempValue];
    tempSlider.minValue = kTempMin;
    tempSlider.maxValue = kTempMax;
    tempSlider.doubleValue = g_config.ai.textMemeTemperature;
    tempSlider.target = self;
    tempSlider.action = @selector(textMemeTempChanged:);
    tempSlider.autoresizingMask = NSViewWidthSizable;
    tempSlider.continuous = YES;
    [self addSubview:tempSlider];

    y -= kPostToggleYGap;

    NSTextField* promptTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kSectionTitleWidth, kPromptTitleHeight)];
    promptTitle.stringValue = @"System Prompt Preview:";
    promptTitle.font = [NSFont fontWithName:@"Maple Mono" size:kPromptFontSize] ?: [NSFont systemFontOfSize:kPromptFontSize weight:NSFontWeightSemibold];
    promptTitle.textColor = [NSColor colorWithWhite:0.7 alpha:1.0];
    promptTitle.backgroundColor = [NSColor clearColor];
    promptTitle.bordered = NO; promptTitle.editable = NO;
    [self addSubview:promptTitle];

    y -= kEvilPromptYGap;

    // Glass panel behind prompt preview
    NSVisualEffectView* glass = [[NSVisualEffectView alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kPromptBodyHeight)];
    glass.material = NSVisualEffectMaterialHUDWindow;
    glass.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    glass.state = NSVisualEffectStateActive;
    glass.wantsLayer = YES;
    glass.layer.cornerRadius = kPromptBodyCornerRadius;
    glass.layer.masksToBounds = YES;
    [self addSubview:glass];

    _promptBody = [[NSTextView alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kPromptBodyHeight)];
    _promptBody.font = [NSFont fontWithName:@"Maple Mono" size:kPromptFontSize] ?: [NSFont systemFontOfSize:kPromptFontSize];
    _promptBody.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    _promptBody.backgroundColor = [NSColor clearColor];
    _promptBody.wantsLayer = YES;
    _promptBody.layer.cornerRadius = kPromptBodyCornerRadius;
    _promptBody.layer.masksToBounds = YES;
    _promptBody.editable = NO;
    _promptBody.selectable = YES;
    _promptBody.drawsBackground = NO;
    _promptBody.string = [self promptPreviewForEvilLevel:g_config.ai.evilLevel];
    [self addSubview:_promptBody];

    y -= kEvilPostPromptYGap;
    y -= 20;  // extra gap to clear glass panel

    // --- SECTION: TEXT MEME ---
    NSTextField* section3 = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, kSectionTitleWidth, kSectionTitleHeight)];
    section3.stringValue = @"TEXT MEME";
    section3.font = [NSFont fontWithName:@"Maple Mono" size:kSectionTitleFontSize] ?: [NSFont systemFontOfSize:kSectionTitleFontSize weight:NSFontWeightBold];
    section3.textColor = [NSColor colorWithWhite:0.6 alpha:1.0];
    section3.backgroundColor = [NSColor clearColor];
    section3.bordered = NO; section3.editable = NO;
    [self addSubview:section3];

    y -= kPostSectionYGap;

    // Text meme toggle: label left, NSSwitch right
    NSTextField* textMemeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2 - 44 - 8, kToggleHeight)];
    textMemeLabel.stringValue = @"Generate text memes via AI";
    textMemeLabel.font = [NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize];
    textMemeLabel.textColor = [NSColor whiteColor];
    textMemeLabel.backgroundColor = [NSColor clearColor];
    textMemeLabel.bordered = NO; textMemeLabel.editable = NO;
    [self addSubview:textMemeLabel];

    NSSwitch* textMemeSwitch = [[NSSwitch alloc] initWithFrame:NSMakeRect(w - marginX - 44, y, 44, kToggleHeight)];
    textMemeSwitch.state = g_config.ai.textMemeEnabled ? NSControlStateValueOn : NSControlStateValueOff;
    textMemeSwitch.target = self;
    textMemeSwitch.action = @selector(textMemeToggled:);
    [self addSubview:textMemeSwitch];

    y -= kPostToggleYGap;

    // Auto-save toggle: label left, NSSwitch right
    NSTextField* autoSaveLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2 - 44 - 8, kToggleHeight)];
    autoSaveLabel.stringValue = @"Auto-save generated texts";
    autoSaveLabel.font = [NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize];
    autoSaveLabel.textColor = [NSColor whiteColor];
    autoSaveLabel.backgroundColor = [NSColor clearColor];
    autoSaveLabel.bordered = NO; autoSaveLabel.editable = NO;
    [self addSubview:autoSaveLabel];

    NSSwitch* autoSaveSwitch = [[NSSwitch alloc] initWithFrame:NSMakeRect(w - marginX - 44, y, 44, kToggleHeight)];
    autoSaveSwitch.state = g_config.ai.textMemeAutoSave ? NSControlStateValueOn : NSControlStateValueOff;
    autoSaveSwitch.target = self;
    autoSaveSwitch.action = @selector(textMemeAutoSaveToggled:);
    [self addSubview:autoSaveSwitch];

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

    // MCP Server toggle: label left, Port field middle, NSSwitch right (aligned with other toggles)
    NSTextField* mcpLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, 92, kToggleHeight)];
    mcpLabel.stringValue = @"MCP server";
    mcpLabel.font = [NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize];
    mcpLabel.textColor = [NSColor whiteColor];
    mcpLabel.backgroundColor = [NSColor clearColor];
    mcpLabel.bordered = NO; mcpLabel.editable = NO;
    [self addSubview:mcpLabel];

    // Port field: directly to the right of "MCP server" label
    NSTextField* mcpPortLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX + 96, y + 2, 36, kSectionTitleHeight)];
    mcpPortLabel.stringValue = @"Port:";
    mcpPortLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    mcpPortLabel.textColor = [NSColor colorWithWhite:0.75 alpha:1.0];
    mcpPortLabel.backgroundColor = [NSColor clearColor];
    mcpPortLabel.bordered = NO; mcpPortLabel.editable = NO;
    mcpPortLabel.alignment = NSTextAlignmentRight;
    [self addSubview:mcpPortLabel];

    NSTextField* mcpPortField = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX + 136, y, 50.0f, kControlHeightSmall)];
    mcpPortField.stringValue = [NSString stringWithFormat:@"%d", g_config.ai.mcpPort];
    mcpPortField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    mcpPortField.bezelStyle = NSTextFieldRoundedBezel;
    mcpPortField.target = self;
    mcpPortField.action = @selector(mcpPortChanged:);
    [self addSubview:mcpPortField];

    NSSwitch* mcpSwitch = [[NSSwitch alloc] initWithFrame:NSMakeRect(w - marginX - 44, y, 44, kToggleHeight)];
    mcpSwitch.state = g_config.ai.enableMCP ? NSControlStateValueOn : NSControlStateValueOff;
    mcpSwitch.target = self;
    mcpSwitch.action = @selector(mcpToggled:);
    [self addSubview:mcpSwitch];

    y -= kPostToggleYGap;

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
    _foundationNote.hidden = !isFoundation;
}

@end
