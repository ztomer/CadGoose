#import "config_gui_helpers.h"
#include "config.h"
#include "behaviors/ai_prompt_builder.h"
#include "config_gui_ai_layout.h"

// Category: builds the "Provider & Network" section of the AI tab.
// Extracted from AITabView.setupUI along its section seams; layout constants
// remain in config_gui_ai.mm.

@implementation AITabView (Sections)

- (float)setupConnectionSectionWithY:(float)y
                             width:(CGFloat)w {
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
    self.portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kPortLabelX, y + kPortLabelYOffset, kPortLabelWidth, kSectionTitleHeight)];
    self.portLabel.stringValue = @"Port:";
    self.portLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    self.portLabel.textColor = [NSColor whiteColor];
    self.portLabel.backgroundColor = [NSColor clearColor];
    self.portLabel.bordered = NO; self.portLabel.editable = NO;
    [self addSubview:self.portLabel];

    self.portField = [[NSTextField alloc] initWithFrame:NSMakeRect(kPortFieldX, y + kPortLabelYOffset, kPortFieldWidth, kControlHeightSmall)];
    self.portField.integerValue = [self currentPort];
    self.portField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    self.portField.bezelStyle = NSTextFieldRoundedBezel;
    self.portField.tag = kPortFieldTag;
    self.portField.target = self;
    self.portField.action = @selector(portChanged:);
    [self addSubview:self.portField];

    // Model popup
    self.modelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kModelLabelX, y + kPortLabelYOffset, kModelLabelWidth, kSectionTitleHeight)];
    self.modelLabel.stringValue = @"Model:";
    self.modelLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    self.modelLabel.textColor = [NSColor whiteColor];
    self.modelLabel.backgroundColor = [NSColor clearColor];
    self.modelLabel.bordered = NO; self.modelLabel.editable = NO;
    [self addSubview:self.modelLabel];

    self.modelPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(kModelPopupX, y, kModelPopupWidth, kControlHeight)];
    [self.modelPopup addItemWithTitle:[self currentModelName]];
    self.modelPopup.tag = kModelPopupTag;
    self.modelPopup.target = self;
    self.modelPopup.action = @selector(modelPopupChanged:);
    [self addSubview:self.modelPopup];

    self.refreshBtn = [[NSButton alloc] initWithFrame:NSMakeRect(kRefreshBtnX, y, kRefreshBtnSize, kRefreshBtnSize)];
    [self.refreshBtn setTitle:@"Refresh"];
    [self.refreshBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kRefreshBtnFontSize] ?: [NSFont systemFontOfSize:kRefreshBtnFontSize]];
    [self.refreshBtn setTarget:self];
    [self.refreshBtn setAction:@selector(refreshModels:)];
    self.refreshBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:self.refreshBtn];

    // Test Connection button (same row as refresh)
    NSButton* testBtn = [[NSButton alloc] initWithFrame:NSMakeRect(kRefreshBtnX + kRefreshBtnSize + 8, y, kTestBtnWidth, kControlHeight)];
    [testBtn setTitle:@"Test Conn"];
    [testBtn setFont:[NSFont fontWithName:@"Maple Mono" size:kBtnFontSize] ?: [NSFont systemFontOfSize:kBtnFontSize]];
    [testBtn setTarget:self];
    [testBtn setAction:@selector(testConnection:)];
    testBtn.bezelStyle = NSBezelStyleRounded;
    testBtn.identifier = @"testConnectionBtn";
    [self addSubview:testBtn];

    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y - kControlHeight - 4, w - marginX*2, kSectionTitleHeight)];
    self.statusLabel.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    self.statusLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    self.statusLabel.backgroundColor = [NSColor clearColor];
    self.statusLabel.bordered = NO; self.statusLabel.editable = NO;
    self.statusLabel.identifier = @"connectionStatus";
    [self addSubview:self.statusLabel];

    y -= kControlGap;

    // Custom endpoint URL field
    self.endpointField = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kEndpointFieldHeight)];
    self.endpointField.stringValue = [NSString stringWithUTF8String:g_config.ai.customEndpoint.c_str()];
    self.endpointField.placeholderString = @"http://localhost:1337/v1/chat/completions";
    self.endpointField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    self.endpointField.bezelStyle = NSTextFieldRoundedBezel;
    self.endpointField.target = self;
    self.endpointField.action = @selector(endpointChanged:);
    self.endpointField.hidden = (prov != 2 && prov != 3);
    self.endpointField.autoresizingMask = NSViewWidthSizable;
    [self addSubview:self.endpointField];

    // Custom model name field
    self.customModelField = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y + kCustomModelFieldYOffset, w - marginX*2, kCustomModelFieldHeight)];
    self.customModelField.stringValue = [NSString stringWithUTF8String:g_config.ai.customModel.c_str()];
    self.customModelField.placeholderString = @"Model name";
    self.customModelField.font = [NSFont fontWithName:@"Maple Mono" size:kLabelFontSize] ?: [NSFont systemFontOfSize:kLabelFontSize];
    self.customModelField.bezelStyle = NSTextFieldRoundedBezel;
    self.customModelField.target = self;
    self.customModelField.action = @selector(customModelChanged:);
    self.customModelField.hidden = (prov != 2 && prov != 3);
    self.customModelField.autoresizingMask = NSViewWidthSizable;
    [self addSubview:self.customModelField];

    if (prov == 2 || prov == 3) y -= kCustomSectionYDrop;

    y -= kPostCustomYGap;

    // Extra gap for Foundation provider to clear the provider row (24px control height + 10px gap - 10px already in kPostCustomYGap)
    if (prov == 0) y -= 24;

    // Foundation explainer note — in Connection section, shown only for Foundation provider
    self.foundationNote = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kFoundationNoteHeight)];
    self.foundationNote.stringValue = FoundationPersonaCapNote();
    self.foundationNote.font = [NSFont fontWithName:@"Maple Mono" size:kSmallFontSize] ?: [NSFont systemFontOfSize:kSmallFontSize];
    self.foundationNote.textColor = [NSColor systemOrangeColor];
    self.foundationNote.backgroundColor = [NSColor clearColor];
    self.foundationNote.bordered = NO; self.foundationNote.editable = NO;
    self.foundationNote.usesSingleLineMode = NO;
    self.foundationNote.lineBreakMode = NSLineBreakByWordWrapping;
    self.foundationNote.cell.wraps = YES;
    self.foundationNote.cell.truncatesLastVisibleLine = NO;
    self.foundationNote.autoresizingMask = NSViewWidthSizable;
    self.foundationNote.hidden = (prov != 0);
    [self addSubview:self.foundationNote];
    if (prov == 0) y -= kPostNoteGap;  // was kFoundationNoteHeight + kPostNoteGap
    return y;
}

@end
