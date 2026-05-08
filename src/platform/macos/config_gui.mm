// config_gui.mm
// Config GUI - macOS preferences window for behavior settings
// ===========================
#import <Cocoa/Cocoa.h>
#include "config.h"

@interface ConfigGUIWindowController : NSWindowController <NSWindowDelegate, NSTableViewDelegate, NSTableViewDataSource, NSTextFieldDelegate>
@property (nonatomic, strong) NSTableView* behaviorsTable;
@property (nonatomic, strong) NSMutableArray* configItems;
@end

@implementation ConfigGUIWindowController

- (instancetype)init {
    self.configItems = [NSMutableArray array];
    [self loadConfigItems];

    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 500, 400)
                                             styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    self.window.title = @"CadGoose Behavior Settings";
    self.window.delegate = self;
    [self.window center];

    NSView* contentView = self.window.contentView;
    [contentView setWantsLayer:YES];

    NSView* headerView = [[NSView alloc] initWithFrame:NSMakeRect(0, 360, 500, 40)];
    NSTextField* titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 10, 460, 20)];
    titleLabel.stringValue = @"Configure Behavior Settings";
    titleLabel.font = [NSFont boldSystemFontOfSize:14];
    titleLabel.textColor = [NSColor labelColor];
    titleLabel.backgroundColor = [NSColor clearColor];
    titleLabel.bordered = NO;
    titleLabel.editable = NO;
    [headerView addSubview:titleLabel];
    [contentView addSubview:headerView];

    NSView* separatorLine = [[NSView alloc] initWithFrame:NSMakeRect(0, 358, 500, 1)];
    separatorLine.wantsLayer = YES;
    separatorLine.layer.backgroundColor = [NSColor separatorColor].CGColor;
    [contentView addSubview:separatorLine];

    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 500, 350)];
    scrollView.hasVerticalScroller = YES;
    scrollView.hasHorizontalScroller = NO;
    scrollView.borderType = NSNoBorder;

    NSTableView* tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 500, 350)];
    tableView.headerView = nil;
    tableView.rowHeight = 40;

    NSTableColumn* nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    nameColumn.title = @"Setting";
    nameColumn.width = 200;
    [tableView addTableColumn:nameColumn];

    NSTableColumn* valueColumn = [[NSTableColumn alloc] initWithIdentifier:@"value"];
    valueColumn.title = @"Value";
    valueColumn.width = 280;
    [tableView addTableColumn:valueColumn];

    self.behaviorsTable = tableView;
    tableView.delegate = self;
    tableView.dataSource = self;

    scrollView.documentView = tableView;
    [contentView addSubview:scrollView];

    NSButton* closeButton = [[NSButton alloc] initWithFrame:NSMakeRect(410, 365, 70, 28)];
    [closeButton setTitle:@"Done"];
    [closeButton setTarget:self];
    [closeButton setAction:@selector(closeWindow:)];
    closeButton.bezelStyle = NSBezelStyleRounded;
    [contentView addSubview:closeButton];

    return self;
}

- (void)loadConfigItems {
    [self.configItems removeAllObjects];

    // Fun behaviors
    [self.configItems addObject:@{@"name": @"FUN", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Ball", @"key": @"behaviors.fun.ball", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  BreadCrumbs", @"key": @"behaviors.fun.breadCrumbs", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Hats", @"key": @"behaviors.fun.hats", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Rainbow", @"key": @"behaviors.fun.rainbow", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Acid", @"key": @"behaviors.fun.acid", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Anger", @"key": @"behaviors.fun.anger", @"type": @"bool"}];

    // Control behaviors
    [self.configItems addObject:@{@"name": @"CONTROL", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Honcker", @"key": @"behaviors.control.honcker", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Jail", @"key": @"behaviors.control.jail", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Portals", @"key": @"behaviors.control.portals", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Drag", @"key": @"behaviors.control.drag", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Banish", @"key": @"behaviors.control.banish", @"type": @"bool"}];

    // Info behaviors
    [self.configItems addObject:@{@"name": @"INFO", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Nametag", @"key": @"behaviors.info.nametag", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Debugoose", @"key": @"behaviors.info.debugoose", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Presence", @"key": @"behaviors.info.presence", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Clicker", @"key": @"behaviors.info.clicker", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  GooseManager", @"key": @"behaviors.info.gooseManager", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Color Picker", @"key": @"behaviors.info.colorPicker", @"type": @"bool"}];

    // Systems
    [self.configItems addObject:@{@"name": @"SYSTEMS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Health", @"key": @"behaviors.systems.health", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  AI", @"key": @"behaviors.systems.ai", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Pomodoro", @"key": @"behaviors.systems.pomodoro", @"type": @"bool"}];

    // Ball settings
    [self.configItems addObject:@{@"name": @"BALL SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Count", @"key": @"behaviors.ball.count", @"type": @"int"}];
    [self.configItems addObject:@{@"name": @"  Size", @"key": @"behaviors.ball.size", @"type": @"float"}];
    [self.configItems addObject:@{@"name": @"  Speed", @"key": @"behaviors.ball.speed", @"type": @"float"}];
    [self.configItems addObject:@{@"name": @"  Friction", @"key": @"behaviors.ball.friction", @"type": @"float"}];

    // Acid settings
    [self.configItems addObject:@{@"name": @"ACID SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Spin Speed", @"key": @"behaviors.acid.spinSpeed", @"type": @"float"}];
    [self.configItems addObject:@{@"name": @"  Honk Interval", @"key": @"behaviors.acid.honkInterval", @"type": @"float"}];
    [self.configItems addObject:@{@"name": @"  Trigger Chance", @"key": @"behaviors.acid.triggerChance", @"type": @"int"}];

    // Rainbow settings
    [self.configItems addObject:@{@"name": @"RAINBOW SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Hue Speed", @"key": @"behaviors.rainbow.hueSpeed", @"type": @"float"}];

    // Health settings
    [self.configItems addObject:@{@"name": @"HEALTH SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Max Health", @"key": @"behaviors.health.maxHealth", @"type": @"float"}];
    [self.configItems addObject:@{@"name": @"  Regen Rate", @"key": @"behaviors.health.regenRate", @"type": @"float"}];

    // Jail settings
    [self.configItems addObject:@{@"name": @"JAIL SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Size", @"key": @"behaviors.jail.size", @"type": @"float"}];

    // Clicker settings
    [self.configItems addObject:@{@"name": @"CLICKER SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Chance", @"key": @"behaviors.clicker.chance", @"type": @"int"}];

    // Pomodoro settings
    [self.configItems addObject:@{@"name": @"POMODORO SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Work Minutes", @"key": @"behaviors.pomodoro.workMinutes", @"type": @"int"}];
    [self.configItems addObject:@{@"name": @"  Break Minutes", @"key": @"behaviors.pomodoro.breakMinutes", @"type": @"int"}];
    [self.configItems addObject:@{@"name": @"  Long Break Minutes", @"key": @"behaviors.pomodoro.longBreakMinutes", @"type": @"int"}];
    [self.configItems addObject:@{@"name": @"  Sessions Before Long Break", @"key": @"behaviors.pomodoro.sessionsBeforeLongBreak", @"type": @"int"}];

    // AI settings
    [self.configItems addObject:@{@"name": @"AI SETTINGS", @"type": @"header"}];
    [self.configItems addObject:@{@"name": @"  Use Osaurus", @"key": @"ai.useOsaurus", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Use Ollama", @"key": @"ai.useOllama", @"type": @"bool"}];
    [self.configItems addObject:@{@"name": @"  Osaurus Port", @"key": @"ai.osaurusPort", @"type": @"int"}];
    [self.configItems addObject:@{@"name": @"  Ollama Port", @"key": @"ai.ollamaPort", @"type": @"int"}];
    [self.configItems addObject:@{@"name": @"  Osaurus Model", @"key": @"ai.osaurusModel", @"type": @"string"}];
    [self.configItems addObject:@{@"name": @"  Ollama Model", @"key": @"ai.ollamaModel", @"type": @"string"}];
}

- (void)closeWindow:(id)sender {
    [self.window close];
}

- (bool*)getBoolForKey:(NSString*)key {
    const char* k = [key UTF8String];
    std::string s = k ? k : "";

    if (s == "behaviors.fun.ball") return &g_config.behaviors.fun.ball;
    if (s == "behaviors.fun.breadCrumbs") return &g_config.behaviors.fun.breadCrumbs;
    if (s == "behaviors.fun.hats") return &g_config.behaviors.fun.hats;
    if (s == "behaviors.fun.rainbow") return &g_config.behaviors.fun.rainbow;
    if (s == "behaviors.fun.acid") return &g_config.behaviors.fun.acid;
    if (s == "behaviors.fun.anger") return &g_config.behaviors.fun.anger;
    if (s == "behaviors.control.honcker") return &g_config.behaviors.control.honcker;
    if (s == "behaviors.control.jail") return &g_config.behaviors.control.jail;
    if (s == "behaviors.control.portals") return &g_config.behaviors.control.portals;
    if (s == "behaviors.control.drag") return &g_config.behaviors.control.drag;
    if (s == "behaviors.control.banish") return &g_config.behaviors.control.banish;
    if (s == "behaviors.info.nametag") return &g_config.behaviors.info.nametag;
    if (s == "behaviors.info.debugoose") return &g_config.behaviors.info.debugoose;
    if (s == "behaviors.info.presence") return &g_config.behaviors.info.presence;
    if (s == "behaviors.info.configGUI") return &g_config.behaviors.info.configGUI;
    if (s == "behaviors.info.colorPicker") return &g_config.behaviors.info.colorPicker;
    if (s == "behaviors.info.clicker") return &g_config.behaviors.info.clicker;
    if (s == "behaviors.info.gooseManager") return &g_config.behaviors.info.gooseManager;
    if (s == "behaviors.systems.health") return &g_config.behaviors.systems.health;
    if (s == "behaviors.systems.ai") return &g_config.behaviors.systems.ai;
    if (s == "behaviors.systems.pomodoro") return &g_config.behaviors.systems.pomodoro;
    if (s == "ai.useOsaurus") return &g_config.ai.useOsaurus;
    if (s == "ai.useOllama") return &g_config.ai.useOllama;

    return nullptr;
}

- (id)getValueForKey:(NSString*)key type:(NSString*)type {
    if ([type isEqualToString:@"bool"]) {
        bool* flag = [self getBoolForKey:key];
        return flag ? @(*flag) : @NO;
    }

    const char* k = [key UTF8String];
    std::string s = k ? k : "";

    if (s == "behaviors.ball.count") return @(g_config.behaviors.ball.count);
    if (s == "behaviors.ball.size") return @(g_config.behaviors.ball.size);
    if (s == "behaviors.ball.speed") return @(g_config.behaviors.ball.speed);
    if (s == "behaviors.ball.friction") return @(g_config.behaviors.ball.friction);
    if (s == "behaviors.acid.spinSpeed") return @(g_config.behaviors.acid.spinSpeed);
    if (s == "behaviors.acid.honkInterval") return @(g_config.behaviors.acid.honkInterval);
    if (s == "behaviors.acid.triggerChance") return @(g_config.behaviors.acid.triggerChance);
    if (s == "behaviors.rainbow.hueSpeed") return @(g_config.behaviors.rainbow.hueSpeed);
    if (s == "behaviors.health.maxHealth") return @(g_config.behaviors.health.maxHealth);
    if (s == "behaviors.health.regenRate") return @(g_config.behaviors.health.regenRate);
    if (s == "behaviors.jail.size") return @(g_config.behaviors.jail.size);
    if (s == "behaviors.jail.keyO") return @(g_config.behaviors.jail.keyO);
    if (s == "behaviors.jail.keyP") return @(g_config.behaviors.jail.keyP);
    if (s == "behaviors.clicker.chance") return @(g_config.behaviors.clicker.chance);
    if (s == "behaviors.pomodoro.workMinutes") return @(g_config.behaviors.pomodoro.workMinutes);
    if (s == "behaviors.pomodoro.breakMinutes") return @(g_config.behaviors.pomodoro.breakMinutes);
    if (s == "behaviors.pomodoro.longBreakMinutes") return @(g_config.behaviors.pomodoro.longBreakMinutes);
    if (s == "behaviors.pomodoro.sessionsBeforeLongBreak") return @(g_config.behaviors.pomodoro.sessionsBeforeLongBreak);
    if (s == "ai.osaurusPort") return @(g_config.ai.osaurusPort);
    if (s == "ai.ollamaPort") return @(g_config.ai.ollamaPort);

    return @"";
}

- (void)setValue:(id)value forKey:(NSString*)key type:(NSString*)type {
    if ([type isEqualToString:@"bool"]) {
        bool* flag = [self getBoolForKey:key];
        if (flag) {
            *flag = [value boolValue];
        }
        return;
    }

    const char* k = [key UTF8String];
    std::string s = k ? k : "";

    if (s == "behaviors.ball.count") g_config.behaviors.ball.count = [value intValue];
    else if (s == "behaviors.ball.size") g_config.behaviors.ball.size = [value floatValue];
    else if (s == "behaviors.ball.speed") g_config.behaviors.ball.speed = [value floatValue];
    else if (s == "behaviors.ball.friction") g_config.behaviors.ball.friction = [value floatValue];
    else if (s == "behaviors.acid.spinSpeed") g_config.behaviors.acid.spinSpeed = [value floatValue];
    else if (s == "behaviors.acid.honkInterval") g_config.behaviors.acid.honkInterval = [value floatValue];
    else if (s == "behaviors.acid.triggerChance") g_config.behaviors.acid.triggerChance = [value intValue];
    else if (s == "behaviors.rainbow.hueSpeed") g_config.behaviors.rainbow.hueSpeed = [value floatValue];
    else if (s == "behaviors.health.maxHealth") g_config.behaviors.health.maxHealth = [value floatValue];
    else if (s == "behaviors.health.regenRate") g_config.behaviors.health.regenRate = [value floatValue];
    else if (s == "behaviors.jail.size") g_config.behaviors.jail.size = [value floatValue];
    else if (s == "behaviors.jail.keyO") g_config.behaviors.jail.keyO = [value intValue];
    else if (s == "behaviors.jail.keyP") g_config.behaviors.jail.keyP = [value intValue];
    else if (s == "behaviors.clicker.chance") g_config.behaviors.clicker.chance = [value intValue];
    else if (s == "behaviors.pomodoro.workMinutes") g_config.behaviors.pomodoro.workMinutes = [value intValue];
    else if (s == "behaviors.pomodoro.breakMinutes") g_config.behaviors.pomodoro.breakMinutes = [value intValue];
    else if (s == "behaviors.pomodoro.longBreakMinutes") g_config.behaviors.pomodoro.longBreakMinutes = [value intValue];
    else if (s == "behaviors.pomodoro.sessionsBeforeLongBreak") g_config.behaviors.pomodoro.sessionsBeforeLongBreak = [value intValue];
    else if (s == "ai.osaurusPort") g_config.ai.osaurusPort = [value intValue];
    else if (s == "ai.ollamaPort") g_config.ai.ollamaPort = [value intValue];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
    return self.configItems.count;
}

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    NSDictionary* item = self.configItems[row];
    NSString* identifier = column.identifier;
    NSString* type = item[@"type"];

    if ([identifier isEqualToString:@"name"]) {
        NSTextField* label = [tableView makeViewWithIdentifier:@"nameLabel" owner:self];
        if (!label) {
            label = [[NSTextField alloc] init];
            label.identifier = @"nameLabel";
            label.backgroundColor = [NSColor clearColor];
            label.bordered = NO;
            label.editable = NO;
            label.font = [NSFont systemFontOfSize:13];
        }
        label.stringValue = item[@"name"];
        return label;
    }

    if ([identifier isEqualToString:@"value"]) {
        if ([type isEqualToString:@"bool"]) {
            NSButton* checkbox = [tableView makeViewWithIdentifier:@"checkbox" owner:self];
            if (!checkbox) {
                checkbox = [[NSButton alloc] init];
                checkbox.identifier = @"checkbox";
                checkbox.buttonType = NSButtonTypeSwitch;
                checkbox.title = @"";
            }
            bool* flag = [self getBoolForKey:item[@"key"]];
            checkbox.state = flag && *flag ? NSControlStateValueOn : NSControlStateValueOff;
            checkbox.tag = row;
            [checkbox setTarget:self];
            [checkbox setAction:@selector(toggleBool:)];
            return checkbox;
        } else {
            NSTextField* field = [tableView makeViewWithIdentifier:@"numberField" owner:self];
            if (!field) {
                field = [[NSTextField alloc] init];
                field.identifier = @"numberField";
                field.font = [NSFont systemFontOfSize:12];
                field.alignment = NSTextAlignmentRight;
            }
            id value = [self getValueForKey:item[@"key"] type:type];
            if ([type isEqualToString:@"int"]) {
                field.stringValue = [NSString stringWithFormat:@"%d", [value intValue]];
            } else {
                field.stringValue = [NSString stringWithFormat:@"%.1f", [value floatValue]];
            }
            field.tag = row;
            field.delegate = self;
            return field;
        }
    }

    return nil;
}

- (void)toggleBool:(NSButton*)sender {
    NSInteger row = sender.tag;
    NSDictionary* item = self.configItems[row];
    bool* flag = [self getBoolForKey:item[@"key"]];
    if (flag) {
        *flag = !*flag;
        sender.state = *flag ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

- (void)controlTextDidEndEditing:(NSNotification*)notification {
    NSTextField* field = notification.object;
    NSInteger row = field.tag;
    if (row < 0 || row >= (NSInteger)self.configItems.count) return;

    NSDictionary* item = self.configItems[row];
    NSString* type = item[@"type"];
    NSString* key = item[@"key"];

    float value = [field.stringValue floatValue];
    [self setValue:@(value) forKey:key type:type];
}

- (void)windowWillClose:(NSNotification*)notification {
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