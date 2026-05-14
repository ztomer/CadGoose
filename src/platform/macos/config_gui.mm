// config_gui.mm
// Free functions, ConfigGUIWindowController with tabs, and entry points
#import "config_gui_helpers.h"
#include "config.h"
#include "world.h"

bool s_getBoolForKey(const std::string& key) {
    if (key == "behaviors.fun.ball") return g_config.behaviors.fun.ball;
    if (key == "behaviors.fun.breadCrumbs") return g_config.behaviors.fun.breadCrumbs;
    if (key == "behaviors.fun.hats") return g_config.behaviors.fun.hats;
    if (key == "behaviors.fun.rainbow") return g_config.behaviors.fun.rainbow;
    if (key == "behaviors.fun.acid") return g_config.behaviors.fun.acid;
    if (key == "behaviors.fun.anger") return g_config.behaviors.fun.anger;
    if (key == "behaviors.fun.autumnLeaves") return g_config.behaviors.fun.autumnLeaves;
    if (key == "behaviors.control.honcker") return g_config.behaviors.control.honcker;
    if (key == "behaviors.control.jail") return g_config.behaviors.control.jail;
    if (key == "behaviors.control.portals") return g_config.behaviors.control.portals;
    if (key == "behaviors.control.drag") return g_config.behaviors.control.drag;
    if (key == "behaviors.control.banish") return g_config.behaviors.control.banish;
    if (key == "behaviors.info.nametag") return g_config.behaviors.info.nametag;
    if (key == "behaviors.info.presence") return g_config.behaviors.info.presence;
    if (key == "behaviors.info.configGUI") return g_config.behaviors.info.configGUI;
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
    else if (key == "behaviors.systems.health.opacity") g_config.behaviors.health.opacity = value;
    else if (key == "behaviors.systems.pomodoro.workDuration") g_config.behaviors.pomodoro.workMinutes = (int)value;
    else if (key == "behaviors.systems.pomodoro.breakDuration") g_config.behaviors.pomodoro.breakMinutes = (int)value;
    OnConfigChange();
}

NSMutableArray* g_configItemsForAccess = nil;

void s_setBoolValue(const std::string& key, bool value) {
    if (key == "behaviors.fun.ball") g_config.behaviors.fun.ball = value;
    else if (key == "behaviors.fun.breadCrumbs") g_config.behaviors.fun.breadCrumbs = value;
    else if (key == "behaviors.fun.hats") g_config.behaviors.fun.hats = value;
    else if (key == "behaviors.fun.rainbow") g_config.behaviors.fun.rainbow = value;
    else if (key == "behaviors.fun.acid") g_config.behaviors.fun.acid = value;
    else if (key == "behaviors.fun.anger") g_config.behaviors.fun.anger = value;
    else if (key == "behaviors.fun.autumnLeaves") g_config.behaviors.fun.autumnLeaves = value;
    else if (key == "behaviors.control.honcker") g_config.behaviors.control.honcker = value;
    else if (key == "behaviors.control.jail") g_config.behaviors.control.jail = value;
    else if (key == "behaviors.control.portals") g_config.behaviors.control.portals = value;
    else if (key == "behaviors.control.drag") g_config.behaviors.control.drag = value;
    else if (key == "behaviors.control.banish") g_config.behaviors.control.banish = value;
    else if (key == "behaviors.info.nametag") g_config.behaviors.info.nametag = value;
    else if (key == "behaviors.info.presence") g_config.behaviors.info.presence = value;
    else if (key == "behaviors.info.configGUI") g_config.behaviors.info.configGUI = value;
    else if (key == "behaviors.systems.health") g_config.behaviors.systems.health = value;
    else if (key == "behaviors.systems.ai") g_config.behaviors.systems.ai = value;
    else if (key == "behaviors.systems.pomodoro") g_config.behaviors.systems.pomodoro = value;
    OnConfigChange();
}

// Thin separator view drawn via drawRect (layer-backing triggers AGXMetalG16X blit shader compilation crash on Mac16,6 macOS 26.5)
@implementation AppBarBorderView
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor separatorColor] setFill];
    NSRectFill(dirtyRect);
}
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

        // Compute list width from content: toggle+icon+name(max)+gap+desc(max)+padding
        NSFont* nameFont = [NSFont fontWithName:@"Comic Sans MS" size:14] ?: [NSFont systemFontOfSize:14 weight:NSFontWeightSemibold];
        NSFont* descFont = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
        CGFloat maxNW = [@"Autumn Leaves" sizeWithAttributes:@{NSFontAttributeName: nameFont}].width + 4;
        CGFloat maxDW = [@"Create portals with P+1/2, teleport with P+0" sizeWithAttributes:@{NSFontAttributeName: descFont}].width + 18;
        self.descLabelX = 64 + maxNW + 2;
        self.listWidth = self.descLabelX + maxDW + 12;
        fprintf(stderr, "[config] listWidth=%.0f descLabelX=%.0f maxNW=%.0f maxDW=%.0f\n",
                self.listWidth, self.descLabelX, maxNW, maxDW);

        // Compute detail width from content: instruction text + slider controls
        NSDictionary* font12 = @{NSFontAttributeName: [NSFont systemFontOfSize:12]};
        CGFloat labelW = 120; // max label width for slider names
        CGFloat valW = [@"100.00" sizeWithAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:11]}].width + 12;
        CGFloat sliderMin = 150;
        CGFloat detailWidth = 12 + labelW + 6 + sliderMin + 6 + valW + 12;
        CGFloat windowWidth = self.listWidth + 1 + detailWidth;

        self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, windowWidth, WINDOW_HEIGHT)
                                                 styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskFullSizeContentView
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
        self.window.titleVisibility = NSWindowTitleHidden;
        self.window.titlebarAppearsTransparent = YES;
        self.window.title = @"Preferences";
        self.window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        [self.window center];

        NSView* contentView = self.window.contentView;

        // Solid dark background
        contentView.wantsLayer = YES;
        contentView.layer.backgroundColor = [[NSColor colorWithWhite:0.12 alpha:1.0] CGColor];

        // Appbar with tab control
        NSView* appBar = [[NSView alloc] initWithFrame:NSMakeRect(0, WINDOW_HEIGHT - APPBAR_HEIGHT, windowWidth, APPBAR_HEIGHT)];

        _tabControl = [[NSSegmentedControl alloc] initWithFrame:NSMakeRect((windowWidth - 260) / 2, 7, 260, 24)];
        _tabControl.segmentCount = 3;
        [_tabControl setLabel:@"Behaviors" forSegment:0];
        [_tabControl setLabel:@"Appearance" forSegment:1];
        [_tabControl setLabel:@"AI" forSegment:2];
        _tabControl.target = self;
        _tabControl.action = @selector(tabChanged:);
        _tabControl.selectedSegment = 0;
        _tabControl.autoresizingMask = NSViewMinXMargin | NSViewMaxXMargin;
        [appBar addSubview:_tabControl];

        [contentView addSubview:appBar];

        // Appbar bottom border drawn via drawRect (layer-backed triggers AGX Metal crash)
        NSView* appBarBorder = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(0, WINDOW_HEIGHT - APPBAR_HEIGHT, windowWidth, 1)];
        [contentView addSubview:appBarBorder];

        // Content container (below appbar)
        _contentContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, windowWidth, TABLE_HEIGHT)];
        _contentContainer.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        [contentView addSubview:_contentContainer];

        // --- Behaviors tab: list + detail split ---
        _behaviorsContainer = [[NSView alloc] initWithFrame:_contentContainer.bounds];

        _detailView = [[BehaviorDetailView alloc] initWithFrame:NSMakeRect(self.listWidth + 1, 0, detailWidth, TABLE_HEIGHT)];
        [_behaviorsContainer addSubview:_detailView];

        NSView* separator = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(self.listWidth, 0, 1, TABLE_HEIGHT)];
        [_behaviorsContainer addSubview:separator];

        NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, TABLE_HEIGHT)];
        scrollView.hasVerticalScroller = NO;
        scrollView.borderType = NSNoBorder;
        scrollView.drawsBackground = NO;

        NSTableView* tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, TABLE_HEIGHT)];
        tableView.headerView = nil;
        tableView.delegate = self;
        tableView.dataSource = self;
        tableView.allowsEmptySelection = YES;
        tableView.selectionHighlightStyle = NSTableViewSelectionHighlightStyleNone;
        tableView.backgroundColor = [NSColor clearColor];
        tableView.intercellSpacing = NSMakeSize(0, 0);

        NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"main"];
        column.width = self.listWidth;
        [tableView addTableColumn:column];

        self.behaviorsTable = tableView;
        scrollView.documentView = tableView;
        [_behaviorsContainer addSubview:scrollView];
        [_contentContainer addSubview:_behaviorsContainer];

// --- Appearance tab (wantsLayer to avoid gray bg in layer-shared compositing) ---
         _appearanceView = [[AppearanceTabView alloc] initWithFrame:_contentContainer.bounds];
         _appearanceView.wantsLayer = YES;
         _appearanceView.layer.backgroundColor = [[NSColor clearColor] CGColor];
         _appearanceView.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
         _appearanceView.hidden = YES;
        [_contentContainer addSubview:_appearanceView];

// --- AI tab ---
         _aiView = [[AITabView alloc] initWithFrame:_contentContainer.bounds];
         _aiView.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
         _aiView.hidden = YES;
        [_contentContainer addSubview:_aiView];

        [self loadConfigItems];
    }
    return self;
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
    [self addRow:@"Autumn Leaves" key:@"behaviors.fun.autumnLeaves" desc:@"Piles of leaves accumulate on screen"];

    [self.configItems addObject:@{@"name": @"CONTROL", @"type": @"header"}];
    {
        NSString* hk = @(g_config.behaviors.honcker.hotkey.c_str());
        [self addRow:@"Honcker" key:@"behaviors.control.honcker" desc:[NSString stringWithFormat:@"Press %@ to honk at cursor", hk]];
    }
    {
        NSString* kO = @(g_config.behaviors.jail.hotkeyO.c_str());
        NSString* kP = @(g_config.behaviors.jail.hotkeyP.c_str());
        [self addRow:@"Jail" key:@"behaviors.control.jail" desc:[NSString stringWithFormat:@"Set traps with %@, trigger with %@", kO, kP]];
    }
    {
        NSString* k1 = @(g_config.portal.hotkey1.c_str());
        NSString* k2 = @(g_config.portal.hotkey2.c_str());
        NSString* k0 = @(g_config.portal.hotkey0.c_str());
        [self addRow:@"Portals" key:@"behaviors.control.portals" desc:[NSString stringWithFormat:@"Press %@/%@ to place portals, %@ to toggle. Based on PortalGoos by Moonaliss1", k1, k2, k0]];
    }
    [self addRow:@"Drag" key:@"behaviors.control.drag" desc:@"Click and drag geese around"];
    {
        NSString* bk = @(g_config.behaviors.banish.hotkey.c_str());
        [self addRow:@"Banish" key:@"behaviors.control.banish" desc:[NSString stringWithFormat:@"Press %@ (or Ctrl+Alt+MiddleClick) to banish goose to the shadow realm", bk]];
    }

    [self.configItems addObject:@{@"name": @"INFO", @"type": @"header"}];
    [self addRow:@"Nametag" key:@"behaviors.info.nametag" desc:@"Show goose name above head"];

    [self.configItems addObject:@{@"name": @"SYSTEMS", @"type": @"header"}];
    [self addRow:@"Health" key:@"behaviors.systems.health" desc:@"Health bar system for geese"];
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

- (void)prepareForDisplay {
    // Reset to Behaviors tab on every window open to avoid stale layer state
    // in Appearance/AI views (wantsLayer=YES subviews) causing mutex re-lock
    // on window layer tree rebuild after close+reopen.
    [_tabControl setSelectedSegment:0];
    _behaviorsContainer.hidden = NO;
    _appearanceView.hidden = YES;
    _aiView.hidden = YES;
}

- (void)tabChanged:(NSSegmentedControl*)sender {
    NSInteger idx = sender.selectedSegment;
    _behaviorsContainer.hidden = (idx != 0);
    _appearanceView.hidden = (idx != 1);
    _aiView.hidden = (idx != 2);
}

- (void)showDetailForBehavior:(NSString*)key {
    [_detailView configureForBehavior:key];
}

- (void)tableViewSelectionDidChange:(NSNotification*)note {
    NSInteger row = self.behaviorsTable.selectedRow;
    if (row >= 0 && row < (NSInteger)self.configItems.count) {
        NSDictionary* item = self.configItems[row];
        if ([item[@"type"] isEqualToString:@"behavior"]) {
            self.selectedRowIndex = row;
            [self showDetailForBehavior:item[@"key"]];
        } else {
            self.selectedRowIndex = -1;
        }
    } else {
        self.selectedRowIndex = -1;
    }
    [self.behaviorsTable reloadData];
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
            label.font = [NSFont fontWithName:@"Comic Sans MS" size:14] ?: [NSFont systemFontOfSize:14 weight:NSFontWeightSemibold];
            label.textColor = [NSColor colorWithRed:0.9 green:0.1 blue:0.1 alpha:1.0];
        }
        label.frame = NSMakeRect(12, 2, self.listWidth, 24);
        label.stringValue = item[@"name"];
        return label;
    }

    if ([type isEqualToString:@"behavior"]) {
        BehaviorRowView* rowView = [tableView makeViewWithIdentifier:@"behaviorRow" owner:self];
        if (!rowView) {
            rowView = [[BehaviorRowView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, 44)];
            rowView.identifier = @"behaviorRow";
        }

        rowView.configKey = item[@"key"];
        NSString* nameText = item[@"label"];
        NSString* descText = item[@"desc"] ?: @"";
        rowView.nameLabel.stringValue = nameText;
        rowView.descLabel.stringValue = descText;
        NSFont* df = rowView.descLabel.font;
        CGFloat dw = MIN([descText sizeWithAttributes:@{NSFontAttributeName: df}].width + 18, self.listWidth - self.descLabelX - 10);
        rowView.descLabel.frame = NSMakeRect(self.descLabelX, 8, dw, 14);
        rowView.iconLabel.stringValue = [BehaviorRowView iconForConfigKey:item[@"key"]];
        rowView.target = self;
        rowView.detailAction = @selector(showDetailForBehavior:);
        rowView.selected = (row == self.selectedRowIndex && [item[@"type"] isEqualToString:@"behavior"]);

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
    [g_configGuiController prepareForDisplay];
    [g_configGuiController.window makeKeyAndOrderFront:nil];
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}

void ConfigGUI_CloseWindow() {
    if (g_configGuiController) {
        [g_configGuiController.window close];
    }
}
