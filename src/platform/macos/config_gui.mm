// config_gui.mm
// Config GUI - macOS preferences window for behavior settings
// ===========================
#import <Cocoa/Cocoa.h>
#include "config.h"
#include <string>

static bool s_getBoolForKey(const std::string& key) {
    if (key == "behaviors.fun.ball") return g_config.behaviors.fun.ball;
    if (key == "behaviors.fun.breadCrumbs") return g_config.behaviors.fun.breadCrumbs;
    if (key == "behaviors.fun.hats") return g_config.behaviors.fun.hats;
    if (key == "behaviors.fun.rainbow") return g_config.behaviors.fun.rainbow;
    if (key == "behaviors.fun.acid") return g_config.behaviors.fun.acid;
    if (key == "behaviors.fun.anger") return g_config.behaviors.fun.anger;
    if (key == "behaviors.control.honcker") return g_config.behaviors.control.honcker;
    if (key == "behaviors.control.jail") return g_config.behaviors.control.jail;
    if (key == "behaviors.control.portals") return g_config.behaviors.control.portals;
    if (key == "behaviors.control.drag") return g_config.behaviors.control.drag;
    if (key == "behaviors.control.banish") return g_config.behaviors.control.banish;
    if (key == "behaviors.info.nametag") return g_config.behaviors.info.nametag;
    if (key == "behaviors.info.presence") return g_config.behaviors.info.presence;
    if (key == "behaviors.info.configGUI") return g_config.behaviors.info.configGUI;
    if (key == "behaviors.info.gooseManager") return g_config.behaviors.info.gooseManager;
    if (key == "behaviors.systems.health") return g_config.behaviors.systems.health;
    if (key == "behaviors.systems.ai") return g_config.behaviors.systems.ai;
    if (key == "behaviors.systems.pomodoro") return g_config.behaviors.systems.pomodoro;
    return false;
}

void s_setFloatValue(const std::string& key, float value) {
    if (key == "behaviors.fun.ball.size") g_config.behaviors.ball.size = value;
    else if (key == "behaviors.fun.breadCrumbs.max") g_config.behaviors.breadCrumbs.maxCrumbs = (int)value;
    else if (key == "behaviors.fun.hats.size") g_config.behaviors.hats.sizeX = value;
    else if (key == "behaviors.fun.rainbow.speed") g_config.behaviors.rainbow.hueSpeed = value;
    else if (key == "behaviors.fun.acid.speed") g_config.behaviors.acid.spinSpeed = value;
    else if (key == "behaviors.fun.anger.max") g_config.behaviors.anger.maxAnger = value;
    else if (key == "behaviors.control.honcker.cooldown") g_config.behaviors.honcker.cooldown = value;
    else if (key == "behaviors.control.jail.size") g_config.behaviors.jail.size = value;
    else if (key == "behaviors.control.portals.width") g_config.portal.width = value;
    else if (key == "behaviors.control.drag.radius") g_config.behaviors.drag.radius = value;
    else if (key == "behaviors.control.banish.duration") g_config.behaviors.banish.duration = value;
    else if (key == "behaviors.info.nametag.size") g_config.behaviors.nametag.size = value;
    else if (key == "behaviors.info.presence.interval") g_config.behaviors.presence.interval = value;
    else if (key == "behaviors.info.gooseManager.taskInterval") g_config.behaviors.gooseManager.taskInterval = value;
    else if (key == "behaviors.systems.health.opacity") g_config.behaviors.health.opacity = value;
    else if (key == "behaviors.systems.pomodoro.workDuration") g_config.behaviors.pomodoro.workMinutes = (int)value;
    else if (key == "behaviors.systems.pomodoro.breakDuration") g_config.behaviors.pomodoro.breakMinutes = (int)value;
}

void s_setFloatValue(const std::string& key, float value);
void s_setBoolValue(const std::string& key, bool value);

@interface BehaviorRowView : NSView
@property (nonatomic, strong) NSButton* toggle;
@property (nonatomic, strong) NSTextField* iconLabel;
@property (nonatomic, strong) NSTextField* nameLabel;
@property (nonatomic, strong) NSTextField* descLabel;
@property (nonatomic, strong) NSButton* detailBtn;
@property (nonatomic, copy) NSString* configKey;
@property (nonatomic, weak) id target;
@property (nonatomic) SEL detailAction;
- (void)toggled:(id)sender;
- (void)openDetail:(id)sender;
+ (NSString*)iconForConfigKey:(NSString*)key;
@end

@implementation BehaviorRowView

+ (NSString*)iconForConfigKey:(NSString*)key {
    if ([key hasSuffix:@"ball"]) return @"⚽";
    if ([key hasSuffix:@"breadCrumbs"]) return @"🍞";
    if ([key hasSuffix:@"hats"]) return @"🎩";
    if ([key hasSuffix:@"rainbow"]) return @"🌈";
    if ([key hasSuffix:@"acid"]) return @"🧪";
    if ([key hasSuffix:@"anger"]) return @"😠";
    if ([key hasSuffix:@"honcker"]) return @"📯";
    if ([key hasSuffix:@"jail"]) return @"🔒";
    if ([key hasSuffix:@"portals"]) return @"🌀";
    if ([key hasSuffix:@"drag"]) return @"🖱️";
    if ([key hasSuffix:@"banish"]) return @"👻";
    if ([key hasSuffix:@"nametag"]) return @"🏷️";
    if ([key hasSuffix:@"health"]) return @"❤️";
    if ([key hasSuffix:@"ai"]) return @"🤖";
    if ([key hasSuffix:@"pomodoro"]) return @"⏰";
    return @"🦆";
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _toggle = [[NSButton alloc] initWithFrame:NSMakeRect(8, 6, 20, 20)];
        _toggle.buttonType = NSButtonTypeSwitch;
        _toggle.title = @"";
        _toggle.target = self;
        _toggle.action = @selector(toggled:);
        [self addSubview:_toggle];

        _iconLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(34, 4, 24, 22)];
        _iconLabel.font = [NSFont systemFontOfSize:16];
        _iconLabel.backgroundColor = [NSColor clearColor];
        _iconLabel.bordered = NO;
        _iconLabel.editable = NO;
        _iconLabel.alignment = NSTextAlignmentCenter;
        [self addSubview:_iconLabel];

        _nameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(64, 7, 180, 18)];
        _nameLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:14] ?: [NSFont boldSystemFontOfSize:14];
        _nameLabel.textColor = [NSColor labelColor];
        _nameLabel.backgroundColor = [NSColor clearColor];
        _nameLabel.bordered = NO;
        _nameLabel.editable = NO;
        [self addSubview:_nameLabel];

        _descLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(250, 8, 360, 14)];
        _descLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
        _descLabel.textColor = [NSColor secondaryLabelColor];
        _descLabel.backgroundColor = [NSColor clearColor];
        _descLabel.bordered = NO;
        _descLabel.editable = NO;
        _descLabel.lineBreakMode = NSLineBreakByTruncatingTail;
        [self addSubview:_descLabel];

        _detailBtn = [[NSButton alloc] initWithFrame:NSMakeRect(630, 6, 24, 24)];
        [_detailBtn setTitle:@"⚙️"];
        [_detailBtn setBezelStyle:NSBezelStyleRounded];
        _detailBtn.target = self;
        _detailBtn.action = @selector(openDetail:);
        [self addSubview:_detailBtn];
    }
    return self;
}

- (void)setEnabled:(BOOL)enabled {
    _toggle.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)toggled:(id)sender {
    if (_configKey) {
        std::string key = std::string([_configKey UTF8String]);
        bool val = ((NSButton*)sender).state == NSControlStateValueOn;
        s_setBoolValue(key, val);
    }
}

- (void)openDetail:(id)sender {
    if (_target && _detailAction) {
        [_target performSelector:_detailAction withObject:self.configKey];
    }
}

@end

static NSMutableArray* g_configItemsForAccess = nil;

#define DETAIL_WIDTH 680
#define DETAIL_HEIGHT 380

// Detail panel for individual behavior settings
@interface BehaviorDetailView : NSView
@property (nonatomic, strong) NSTextField* titleLabel;
@property (nonatomic, strong) NSView* contentView;
@property (nonatomic, strong) NSButton* closeBtn;
@property (nonatomic, copy) NSString* configKey;
- (void)configureForBehavior:(NSString*)key;
@end

@implementation BehaviorDetailView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.hidden = YES;
        // Header area
        _titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(14, frame.size.height - 28, frame.size.width - 60, 20)];
        _titleLabel.font = [NSFont boldSystemFontOfSize:15];
        _titleLabel.textColor = [NSColor labelColor];
        _titleLabel.backgroundColor = [NSColor clearColor];
        _titleLabel.bordered = NO;
        _titleLabel.editable = NO;
        _titleLabel.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
        [self addSubview:_titleLabel];

        _closeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width - 28, frame.size.height - 28, 20, 20)];
        _closeBtn.title = @"✕";
        _closeBtn.bezelStyle = NSBezelStyleRounded;
        _closeBtn.target = self;
        _closeBtn.action = @selector(closeDetail:);
        _closeBtn.autoresizingMask = NSViewMinXMargin | NSViewMinYMargin;
        [self addSubview:_closeBtn];

        _contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height - 36)];
        _contentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [self addSubview:_contentView];
    }
    return self;
}

- (void)configureForBehavior:(NSString*)key {
    _configKey = key;

    // Clear previous content
    for (NSView* subview in _contentView.subviews) {
        [subview removeFromSuperview];
    }

    self.hidden = NO;
    _titleLabel.stringValue = [NSString stringWithFormat:@"Settings for %@", [key lastPathComponent]];

    float y = _contentView.bounds.size.height - 40;

    if ([key isEqualToString:@"behaviors.fun.ball"]) {
        _titleLabel.stringValue = @"Ball Behavior";
        [self addSliderWithLabel:@"Ball Size" min:5.0f max:50.0f value:g_config.behaviors.ball.size atY:y key:@"behaviors.fun.ball.size"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.breadCrumbs"]) {
        _titleLabel.stringValue = @"Breadcrumbs Behavior";
        [self addSliderWithLabel:@"Max Crumbs" min:10.0f max:200.0f value:g_config.behaviors.breadCrumbs.maxCrumbs atY:y key:@"behaviors.fun.breadCrumbs.max"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.hats"]) {
        _titleLabel.stringValue = @"Hats Behavior";
        [self addSliderWithLabel:@"Hat Size" min:0.01f max:0.5f value:g_config.behaviors.hats.sizeX atY:y key:@"behaviors.fun.hats.size"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.rainbow"]) {
        _titleLabel.stringValue = @"Rainbow Behavior";
        [self addSliderWithLabel:@"Hue Speed" min:0.1f max:5.0f value:g_config.behaviors.rainbow.hueSpeed atY:y key:@"behaviors.fun.rainbow.speed"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.acid"]) {
        _titleLabel.stringValue = @"Acid Behavior";
        [self addSliderWithLabel:@"Spin Speed" min:0.1f max:5.0f value:g_config.behaviors.acid.spinSpeed atY:y key:@"behaviors.fun.acid.speed"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.anger"]) {
        _titleLabel.stringValue = @"Anger Behavior";
        [self addSliderWithLabel:@"Max Anger" min:0.0f max:200.0f value:g_config.behaviors.anger.maxAnger atY:y key:@"behaviors.fun.anger.max"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.honcker"]) {
        _titleLabel.stringValue = @"Honcker Behavior";
        [self addInstructionLabel:@"🦆 Press F to honk at cursor location" atY:y];
        y -= 25;
        [self addSliderWithLabel:@"Honk Cooldown" min:0.1f max:10.0f value:g_config.behaviors.honcker.cooldown atY:y key:@"behaviors.control.honcker.cooldown"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.jail"]) {
        _titleLabel.stringValue = @"Jail Behavior";
        [self addInstructionLabel:@"🔒 O = set cursor as jail position\n   P = toggle jail on/off" atY:y];
        y -= 42;
        [self addSliderWithLabel:@"Jail Size" min:50.0f max:300.0f value:g_config.behaviors.jail.size atY:y key:@"behaviors.control.jail.size"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.portals"]) {
        _titleLabel.stringValue = @"Portal Behavior";
        [self addInstructionLabel:@"🌀 P+1 = place portal A\n   P+2 = place portal B\n   P+0 = toggle portals" atY:y];
        y -= 60;
        [self addSliderWithLabel:@"Portal Width" min:30.0f max:200.0f value:g_config.portal.width atY:y key:@"behaviors.control.portals.width"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.drag"]) {
        _titleLabel.stringValue = @"Drag Behavior";
        [self addInstructionLabel:@"🖱️ Click and drag the goose" atY:y];
        y -= 25;
        [self addSliderWithLabel:@"Drag Radius" min:50.0f max:300.0f value:g_config.behaviors.drag.radius atY:y key:@"behaviors.control.drag.radius"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.banish"]) {
        _titleLabel.stringValue = @"Banish Behavior";
        [self addInstructionLabel:@"👻 Ctrl+Alt+Middle Click to banish\n   Respawns after the duration" atY:y];
        y -= 42;
        [self addSliderWithLabel:@"Duration (s)" min:1.0f max:60.0f value:g_config.behaviors.banish.duration atY:y key:@"behaviors.control.banish.duration"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.info.nametag"]) {
        _titleLabel.stringValue = @"Nametag Behavior";
        [self addSliderWithLabel:@"Font Size" min:8.0f max:40.0f value:g_config.behaviors.nametag.size atY:y key:@"behaviors.info.nametag.size"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.systems.health"]) {
        _titleLabel.stringValue = @"Health System";
        [self addSliderWithLabel:@"Opacity" min:0.2f max:1.0f value:g_config.behaviors.health.opacity atY:y key:@"behaviors.systems.health.opacity"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.systems.ai"]) {
        _titleLabel.stringValue = @"AI Chat";
        [self addInstructionLabel:@"🤖 Chat with your goose using local AI" atY:y];
        y -= 25;
        [self addProviderSelectorAtY:y];
        y -= 120;
        [self addPromptDisplayAtY:y];
        y -= 60;
    } else if ([key isEqualToString:@"behaviors.systems.pomodoro"]) {
        _titleLabel.stringValue = @"Pomodoro Timer";
        [self addSliderWithLabel:@"Work (min)" min:1.0f max:60.0f value:g_config.behaviors.pomodoro.workMinutes atY:y key:@"behaviors.systems.pomodoro.workDuration"];
        y -= 35;
        [self addSliderWithLabel:@"Break (min)" min:1.0f max:30.0f value:g_config.behaviors.pomodoro.breakMinutes atY:y key:@"behaviors.systems.pomodoro.breakDuration"];
        y -= 35;
    } else {
        NSTextField* desc = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, DETAIL_WIDTH - 24, 30)];
        desc.font = [NSFont systemFontOfSize:12];
        desc.textColor = [NSColor secondaryLabelColor];
        desc.backgroundColor = [NSColor clearColor];
        desc.bordered = NO;
        desc.editable = NO;
        desc.stringValue = [NSString stringWithFormat:@"No settings for %@", [key lastPathComponent]];
        [_contentView addSubview:desc];
    }
}

- (void)addInstructionLabel:(NSString*)text atY:(float)y {
    NSTextField* label = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, DETAIL_WIDTH - 24, 40)];
    label.font = [NSFont fontWithName:@"Comic Sans MS" size:12] ?: [NSFont systemFontOfSize:12];
    label.textColor = [NSColor secondaryLabelColor];
    label.backgroundColor = [NSColor clearColor];
    label.bordered = NO;
    label.editable = NO;
    [_contentView addSubview:label];

    // Set multi-line text by matching origin and size
    NSMutableParagraphStyle* para = [[NSMutableParagraphStyle alloc] init];
    para.lineSpacing = 2;
    NSDictionary* attrs = @{NSFontAttributeName: label.font ?: [NSFont systemFontOfSize:12],
                            NSParagraphStyleAttributeName: para};
    label.attributedStringValue = [[NSAttributedString alloc] initWithString:text attributes:attrs];
}

- (void)addSliderWithLabel:(NSString*)label min:(float)min max:(float)max value:(float)value atY:(float)y key:(NSString*)key {
    NSTextField* labelField = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 150, 16)];
    labelField.font = [NSFont systemFontOfSize:12];
    labelField.textColor = [NSColor labelColor];
    labelField.backgroundColor = [NSColor clearColor];
    labelField.bordered = NO;
    labelField.editable = NO;
    labelField.stringValue = label;
    [_contentView addSubview:labelField];

    NSSlider* slider = [[NSSlider alloc] initWithFrame:NSMakeRect(170, y, 300, 20)];
    slider.minValue = min;
    slider.maxValue = max;
    slider.doubleValue = value;
    slider.identifier = key;
    slider.target = self;
    slider.action = @selector(sliderChanged:);
    [_contentView addSubview:slider];

    NSTextField* valueField = [[NSTextField alloc] initWithFrame:NSMakeRect(490, y, 80, 16)];
    valueField.font = [NSFont systemFontOfSize:11];
    valueField.textColor = [NSColor secondaryLabelColor];
    valueField.backgroundColor = [NSColor clearColor];
    valueField.bordered = NO;
    valueField.editable = YES;
    valueField.stringValue = [NSString stringWithFormat:@"%.2f", value];
    valueField.identifier = key;
    valueField.target = self;
    valueField.action = @selector(valueFieldChanged:);
    [_contentView addSubview:valueField];
}

- (void)addProviderSelectorAtY:(float)y {
    NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(12, y, 200, 24)];
    [popup addItemWithTitle:@"Osaurus"];
    [popup addItemWithTitle:@"Ollama"];
    [popup addItemWithTitle:@"Custom"];

    if (g_config.ai.useOsaurus) [popup selectItemAtIndex:0];
    else if (g_config.ai.useOllama) [popup selectItemAtIndex:1];
    else [popup selectItemAtIndex:2];

    popup.target = self;
    popup.action = @selector(providerChanged:);
    [_contentView addSubview:popup];

    NSTextField* portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(220, y + 2, 40, 16)];
    portLabel.stringValue = @"Port:";
    portLabel.font = [NSFont systemFontOfSize:11];
    portLabel.textColor = [NSColor labelColor];
    portLabel.backgroundColor = [NSColor clearColor];
    portLabel.bordered = NO;
    portLabel.editable = NO;
    [_contentView addSubview:portLabel];

    NSTextField* portField = [[NSTextField alloc] initWithFrame:NSMakeRect(260, y, 60, 22)];
    portField.integerValue = g_config.ai.useOsaurus ? g_config.ai.osaurusPort : g_config.ai.ollamaPort;
    portField.tag = 100;
    portField.target = self;
    portField.action = @selector(portChanged:);
    [_contentView addSubview:portField];

    NSTextField* modelLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(340, y + 2, 50, 16)];
    modelLabel.stringValue = @"Model:";
    modelLabel.font = [NSFont systemFontOfSize:11];
    modelLabel.textColor = [NSColor labelColor];
    modelLabel.backgroundColor = [NSColor clearColor];
    modelLabel.bordered = NO;
    modelLabel.editable = NO;
    [_contentView addSubview:modelLabel];

    NSTextField* modelField = [[NSTextField alloc] initWithFrame:NSMakeRect(395, y, 140, 22)];
    modelField.stringValue = [NSString stringWithUTF8String:(g_config.ai.useOsaurus ? g_config.ai.osaurusModel : g_config.ai.ollamaModel).c_str()];
    modelField.tag = 101;
    modelField.target = self;
    modelField.action = @selector(modelChanged:);
    [_contentView addSubview:modelField];
}

- (void)addPromptDisplayAtY:(float)y {
    NSTextField* promptTitle = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 200, 16)];
    promptTitle.stringValue = @"🧠 System Prompt:";
    promptTitle.font = [NSFont fontWithName:@"Comic Sans MS" size:12] ?: [NSFont boldSystemFontOfSize:12];
    promptTitle.textColor = [NSColor labelColor];
    promptTitle.backgroundColor = [NSColor clearColor];
    promptTitle.bordered = NO;
    promptTitle.editable = NO;
    [_contentView addSubview:promptTitle];

    // Build the prompt text from the goose persona
    NSString* promptText = @"You are a mischievous Canadian goose. You live on\nthe user's desktop, steal things, and honk a lot.\nKeep responses short, sarcastic, and goose-like.";
    NSTextField* promptBody = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y - 45, DETAIL_WIDTH - 24, 40)];
    promptBody.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
    promptBody.textColor = [NSColor secondaryLabelColor];
    promptBody.backgroundColor = [NSColor clearColor];
    promptBody.bordered = NO;
    promptBody.editable = NO;
    promptBody.stringValue = promptText;
    [_contentView addSubview:promptBody];
}

- (void)sliderChanged:(NSSlider*)sender {
    float value = (float)sender.doubleValue;
    NSString* keyStr = sender.identifier;
    if (keyStr) {
        s_setFloatValue(std::string([keyStr UTF8String]), value);
        // Update matching value field
        for (NSView* subview in _contentView.subviews) {
            if ([subview isKindOfClass:[NSTextField class]] && ![subview isEqualTo:sender] && [((NSTextField*)subview).identifier isEqualToString:keyStr]) {
                ((NSTextField*)subview).stringValue = [NSString stringWithFormat:@"%.2f", value];
                break;
            }
        }
    }
}

- (void)valueFieldChanged:(NSTextField*)sender {
    float value = (float)sender.doubleValue;
    NSString* keyStr = sender.identifier;
    if (keyStr) {
        s_setFloatValue(std::string([keyStr UTF8String]), value);
        // Update matching slider
        for (NSView* subview in _contentView.subviews) {
            if ([subview isKindOfClass:[NSSlider class]] && [((NSSlider*)subview).identifier isEqualToString:keyStr]) {
                ((NSSlider*)subview).doubleValue = value;
                break;
            }
        }
    }
}

- (void)providerChanged:(NSPopUpButton*)sender {
    NSInteger idx = sender.indexOfSelectedItem;
    g_config.ai.useOsaurus = (idx == 0);
    g_config.ai.useOllama = (idx == 1);

    // Update port and model fields for selected provider
    for (NSView* subview in _contentView.subviews) {
        if ([subview isKindOfClass:[NSTextField class]] && subview.tag == 100) {
            ((NSTextField*)subview).integerValue = (idx == 0) ? g_config.ai.osaurusPort : g_config.ai.ollamaPort;
        }
        if ([subview isKindOfClass:[NSTextField class]] && subview.tag == 101) {
            ((NSTextField*)subview).stringValue = [NSString stringWithUTF8String:(idx == 0 ? g_config.ai.osaurusModel : g_config.ai.ollamaModel).c_str()];
        }
    }
}

- (void)portChanged:(NSTextField*)sender {
    int port = (int)sender.integerValue;
    NSInteger provider = -1;
    for (NSView* subview in _contentView.subviews) {
        if ([subview isKindOfClass:[NSPopUpButton class]]) {
            provider = ((NSPopUpButton*)subview).indexOfSelectedItem;
            break;
        }
    }
    if (provider == 0) g_config.ai.osaurusPort = port;
    else if (provider == 1) g_config.ai.ollamaPort = port;
}

- (void)modelChanged:(NSTextField*)sender {
    std::string model = std::string([sender.stringValue UTF8String]);
    NSInteger provider = -1;
    for (NSView* subview in _contentView.subviews) {
        if ([subview isKindOfClass:[NSPopUpButton class]]) {
            provider = ((NSPopUpButton*)subview).indexOfSelectedItem;
            break;
        }
    }
    if (provider == 0) g_config.ai.osaurusModel = model;
    else if (provider == 1) g_config.ai.ollamaModel = model;
}

@end

@interface ConfigGUIWindowController : NSWindowController <NSTableViewDelegate, NSTableViewDataSource>
@property (nonatomic, strong) NSTableView* behaviorsTable;
@property (nonatomic, strong) NSMutableArray* configItems;
@property (nonatomic, strong) BehaviorDetailView* detailView;
@property (nonatomic, strong) NSView* separatorLine;
@property (nonatomic) NSWindow* parentWindow;
+ (NSMutableArray*)configItemsForAccess;
@end

@implementation ConfigGUIWindowController

+ (NSMutableArray*)configItemsForAccess {
    return g_configItemsForAccess;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        g_configItemsForAccess = [NSMutableArray array];
        self.configItems = g_configItemsForAccess;

        self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 700, 580)
                                                 styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
        self.window.title = @"🇨🇦  Goose Config Dominion  🦆";
        self.window.minSize = NSMakeSize(700, 400);
        [self.window center];

        NSView* contentView = self.window.contentView;

        // Canadian flag glass backdrop
        NSVisualEffectView* glassView = [[NSVisualEffectView alloc] initWithFrame:contentView.bounds];
        glassView.material = NSVisualEffectMaterialDark;
        glassView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        glassView.state = NSVisualEffectStateActive;
        glassView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [contentView addSubview:glassView positioned:NSWindowBelow relativeTo:nil];

        // Maple leaf background label
        NSTextField* flagBg = [[NSTextField alloc] initWithFrame:NSMakeRect(200, 150, 300, 300)];
        flagBg.stringValue = @"🍁";
        flagBg.font = [NSFont systemFontOfSize:200];
        flagBg.textColor = [NSColor colorWithWhite:1.0 alpha:0.08];
        flagBg.backgroundColor = [NSColor clearColor];
        flagBg.bordered = NO;
        flagBg.editable = NO;
        flagBg.autoresizingMask = NSViewMinXMargin | NSViewMaxXMargin | NSViewMinYMargin | NSViewMaxYMargin;
        [contentView addSubview:flagBg];

        // Header
        NSView* headerView = [[NSView alloc] initWithFrame:NSMakeRect(0, 532, 700, 48)];
        headerView.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;

        NSTextField* titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, 14, 500, 24)];
        titleLabel.stringValue = @"🍁  BEHAVIOR COMMAND CENTRE  🦆";
        titleLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:18] ?: [NSFont boldSystemFontOfSize:18];
        titleLabel.textColor = [NSColor colorWithRed:0.9 green:0.1 blue:0.1 alpha:1.0];
        titleLabel.backgroundColor = [NSColor clearColor];
        titleLabel.bordered = NO;
        titleLabel.editable = NO;
        [headerView addSubview:titleLabel];

        NSButton* toggleAllBtn = [[NSButton alloc] initWithFrame:NSMakeRect(500, 12, 108, 24)];
        [toggleAllBtn setTitle:@"Toggle All"];
        [contentView addSubview:headerView];

        // Detail view area (offscreen until showDetailForBehavior expands window)
        _detailView = [[BehaviorDetailView alloc] initWithFrame:NSMakeRect(700, 0, DETAIL_WIDTH, 580)];
        _detailView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [contentView addSubview:_detailView];

        // Separator between table and detail
        NSView* separator = [[NSView alloc] initWithFrame:NSMakeRect(699, 0, 1, 529)];
        separator.autoresizingMask = NSViewHeightSizable | NSViewMaxXMargin;
        self.separatorLine = separator;
        [contentView addSubview:separator];

        // Scrollable table (fixed width, doesn't stretch when window expands)
        NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 700, 528)];
        scrollView.hasVerticalScroller = YES;
        scrollView.hasHorizontalScroller = NO;
        scrollView.borderType = NSNoBorder;
        scrollView.drawsBackground = NO;
        scrollView.autoresizingMask = NSViewHeightSizable | NSViewMaxXMargin;

        NSTableView* tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 700, 528)];
        tableView.headerView = nil;
        tableView.rowHeight = 44;
        tableView.delegate = self;
        tableView.dataSource = self;
        tableView.selectionHighlightStyle = NSTableViewSelectionHighlightStyleNone;
        tableView.backgroundColor = [NSColor clearColor];
        tableView.intercellSpacing = NSMakeSize(0, 0);

        NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"main"];
        column.width = 700;
        column.minWidth = 400;
        [tableView addTableColumn:column];

        self.behaviorsTable = tableView;

        scrollView.documentView = tableView;
        [contentView addSubview:scrollView];

        // Bottom bar
        NSView* bottomBar = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 700, 40)];
        bottomBar.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
        [contentView addSubview:bottomBar];

        [self loadConfigItems];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(detailPanelClosed:) name:@"DetailPanelClosed" object:nil];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)detailPanelClosed:(NSNotification*)note {
    [self.detailView setHidden:YES];
    NSRect frame = self.window.frame;
    frame.size.width = 700;
    [self.window setFrame:frame display:YES animate:YES];
}

- (void)loadConfigItems {
    if (!self.configItems) self.configItems = [NSMutableArray array];
    [self.configItems removeAllObjects];

    [self.configItems addObject:@{@"name": @"FUN", @"type": @"header"}];
    [self addRow:@"Ball" key:@"behaviors.fun.ball" desc:@"Pushable balls that bounce around"];
    [self addRow:@"Breadcrumbs" key:@"behaviors.fun.breadCrumbs" desc:@"Leave a trail of breadcrumbs"];
    [self addRow:@"Hats" key:@"behaviors.fun.hats" desc:@"Put various hats on geese"];
    [self addRow:@"Rainbow" key:@"behaviors.fun.rainbow" desc:@"Cycle colors on all geese"];
    [self addRow:@"Acid" key:@"behaviors.fun.acid" desc:@"Geese spin and honk rapidly"];
    [self addRow:@"Anger" key:@"behaviors.fun.anger" desc:@"Geese get angry and punch things"];

    [self.configItems addObject:@{@"name": @"CONTROL", @"type": @"header"}];
    [self addRow:@"Honcker" key:@"behaviors.control.honcker" desc:@"Press F to honk at cursor"];
    [self addRow:@"Jail" key:@"behaviors.control.jail" desc:@"Set traps with O, trigger with P"];
    [self addRow:@"Portals" key:@"behaviors.control.portals" desc:@"Create portals with P+1/2, teleport with P+0"];
    [self addRow:@"Drag" key:@"behaviors.control.drag" desc:@"Click and drag geese around"];
    [self addRow:@"Banish" key:@"behaviors.control.banish" desc:@"Banish goose to another dimension"];

    [self.configItems addObject:@{@"name": @"INFO", @"type": @"header"}];
    [self addRow:@"Nametag" key:@"behaviors.info.nametag" desc:@"Show goose name above head"];

    [self.configItems addObject:@{@"name": @"SYSTEMS", @"type": @"header"}];
    [self addRow:@"Health" key:@"behaviors.systems.health" desc:@"Health bar system for geese"];
    [self addRow:@"AI Chat" key:@"behaviors.systems.ai" desc:@"Chat with goose using local AI"];
    [self addRow:@"Pomodoro" key:@"behaviors.systems.pomodoro" desc:@"Pomodoro timer behavior"];

    if (self.behaviorsTable) {
        [self.behaviorsTable reloadData];
    }
}

- (void)addRow:(NSString*)label key:(NSString*)key desc:(NSString*)desc {
    [self.configItems addObject:@{@"key": key, @"label": label, @"desc": desc, @"type": @"behavior"}];
}

- (void)closeWindow:(id)sender {
    [self.window close];
}

- (void)reloadConfig:(id)sender {
    Config_Init();
    [self loadConfigItems];
    [self.behaviorsTable reloadData];
}

- (void)toggleAll:(id)sender {
    bool anyOff = false;
    for (NSDictionary* item in self.configItems) {
        if (![item[@"type"] isEqualToString:@"behavior"]) continue;
        NSString* key = item[@"key"];
        if (!s_getBoolForKey(std::string([key UTF8String]))) { anyOff = true; break; }
    }
    for (NSDictionary* item in self.configItems) {
        if (![item[@"type"] isEqualToString:@"behavior"]) continue;
        NSString* key = item[@"key"];
        std::string k = std::string([key UTF8String]);
        s_setBoolValue(k, anyOff);
    }
    [self.behaviorsTable reloadData];
}

void s_setBoolValue(const std::string& key, bool value) {
    if (key == "behaviors.fun.ball") g_config.behaviors.fun.ball = value;
    else if (key == "behaviors.fun.breadCrumbs") g_config.behaviors.fun.breadCrumbs = value;
    else if (key == "behaviors.fun.hats") g_config.behaviors.fun.hats = value;
    else if (key == "behaviors.fun.rainbow") g_config.behaviors.fun.rainbow = value;
    else if (key == "behaviors.fun.acid") g_config.behaviors.fun.acid = value;
    else if (key == "behaviors.fun.anger") g_config.behaviors.fun.anger = value;
    else if (key == "behaviors.control.honcker") g_config.behaviors.control.honcker = value;
    else if (key == "behaviors.control.jail") g_config.behaviors.control.jail = value;
    else if (key == "behaviors.control.portals") g_config.behaviors.control.portals = value;
    else if (key == "behaviors.control.drag") g_config.behaviors.control.drag = value;
    else if (key == "behaviors.control.banish") g_config.behaviors.control.banish = value;
    else if (key == "behaviors.info.nametag") g_config.behaviors.info.nametag = value;
    else if (key == "behaviors.info.presence") g_config.behaviors.info.presence = value;
    else if (key == "behaviors.info.configGUI") g_config.behaviors.info.configGUI = value;
    else if (key == "behaviors.info.gooseManager") g_config.behaviors.info.gooseManager = value;
    else if (key == "behaviors.systems.health") g_config.behaviors.systems.health = value;
    else if (key == "behaviors.systems.ai") g_config.behaviors.systems.ai = value;
    else if (key == "behaviors.systems.pomodoro") g_config.behaviors.systems.pomodoro = value;
}

- (void)showDetailForBehavior:(NSString*)key {
    [_detailView configureForBehavior:key];

    NSRect frame = self.window.frame;
    CGFloat targetWidth = 700 + DETAIL_WIDTH;
    if (frame.size.width < targetWidth) {
        frame.size.width = targetWidth;
        [self.window setFrame:frame display:YES animate:YES];
    }
}

- (CGFloat)tableView:(NSTableView*)tableView heightOfRow:(NSInteger)row {
    NSDictionary* item = self.configItems[row];
    if ([item[@"type"] isEqualToString:@"header"]) return 28;
    return 44;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
    return self.configItems.count;
}

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    NSDictionary* item = self.configItems[row];
    NSString* type = item[@"type"];

    if ([type isEqualToString:@"header"]) {
        NSTextField* label = [tableView makeViewWithIdentifier:@"headerLabel" owner:self];
        if (!label) {
            label = [[NSTextField alloc] init];
            label.identifier = @"headerLabel";
            label.backgroundColor = [NSColor clearColor];
            label.bordered = NO;
            label.editable = NO;
            label.font = [NSFont fontWithName:@"Comic Sans MS" size:14] ?: [NSFont boldSystemFontOfSize:14];
            label.textColor = [NSColor colorWithRed:0.9 green:0.1 blue:0.1 alpha:1.0];
        }
        label.frame = NSMakeRect(12, 2, 700, 24);
        label.stringValue = [NSString stringWithFormat:@"🇨🇦  %@", item[@"name"]];
        return label;
    }

    if ([type isEqualToString:@"behavior"]) {
        BehaviorRowView* rowView = [tableView makeViewWithIdentifier:@"behaviorRow" owner:self];
        if (!rowView) {
            rowView = [[BehaviorRowView alloc] initWithFrame:NSMakeRect(0, 0, 700, 44)];
            rowView.identifier = @"behaviorRow";
        }

        rowView.configKey = item[@"key"];
        rowView.nameLabel.stringValue = item[@"label"];
        rowView.descLabel.stringValue = item[@"desc"] ?: @"";
        rowView.iconLabel.stringValue = [BehaviorRowView iconForConfigKey:item[@"key"]];
        rowView.target = self;
        rowView.detailAction = @selector(showDetailForBehavior:);
        rowView.detailBtn.target = rowView;
        rowView.detailBtn.action = @selector(openDetail:);

        std::string key = std::string([item[@"key"] UTF8String]);
        bool val = s_getBoolForKey(key);
        rowView.toggle.state = val ? NSControlStateValueOn : NSControlStateValueOff;

        return rowView;
    }

    return nil;
}

@end

static ConfigGUIWindowController* g_configGuiController = nil;

void ConfigGUI_ShowWindow() {
    if (!g_configGuiController) {
        g_configGuiController = [[ConfigGUIWindowController alloc] init];
    }
    [g_configGuiController.window makeKeyAndOrderFront:nil];
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

void ConfigGUI_CloseWindow() {
    if (g_configGuiController) {
        [g_configGuiController.window close];
    }
}