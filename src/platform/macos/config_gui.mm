// config_gui.mm
// Free functions, ConfigGUIWindowController with tabs, and entry points
#import "config_gui_helpers.h"
#include "config.h"
#include "world.h"

// --- Layout constants ---
static constexpr float kWindowHeight = 520.0f;
static constexpr float kAppbarHeight = 44.0f;
static constexpr float kTableHeight = kWindowHeight - kAppbarHeight;
static constexpr float kRowHeight = 52.0f;
static constexpr float kHeaderRowHeight = 28.0f;
static constexpr float kRowPaddingX = 16.0f;
static constexpr float kRowIconX = kRowPaddingX;
static constexpr float kRowIconWidth = 24.0f;
static constexpr float kRowIconGap = 8.0f;
static constexpr float kRowToggleX = kRowIconX + kRowIconWidth + kRowIconGap;
static constexpr float kRowToggleWidth = 36.0f;
static constexpr float kRowNameX = kRowToggleX + kRowToggleWidth + kRowIconGap;
static constexpr float kRowDescGap = 12.0f;
static constexpr float kRowDescPaddingX = 16.0f;
static constexpr float kDetailLeftPad = 16.0f;
static constexpr float kDetailLabelGap = 8.0f;
static constexpr float kDetailSliderMinWidth = 160.0f;
static constexpr float kDetailValuePad = 16.0f;
static constexpr float kDetailRightPad = 16.0f;
static constexpr float kSeparatorWidth = 1.0f;
static constexpr float kTabBarWidth = 260.0f;
static constexpr float kTabBarY = 10.0f;
static constexpr float kTabBarHeight = 24.0f;
static constexpr float kNameFontSize = 14.0f;
static constexpr float kDescFontSize = 11.0f;
static constexpr float kDetailLabelFontSize = 12.0f;
static constexpr float kDetailValueFontSize = 11.0f;

bool s_getBoolForKey(const std::string& key) {
    const ConfigOption* opt = Config_FindOptionByKey(key);
    if (opt && opt->type == CFG_BOOL) return *(bool*)opt->ptr;
    return false;
}

void s_setFloatValue(const std::string& key, float value) {
    const ConfigOption* opt = Config_FindOptionByKey(key);
    if (!opt) return;
    if (opt->type == CFG_FLOAT) {
        *(float*)opt->ptr = value;
    } else if (opt->type == CFG_INT) {
        *(int*)opt->ptr = (int)value;
    }
    OnConfigChange();
}

NSMutableArray* g_configItemsForAccess = nil;

void s_setBoolValue(const std::string& key, bool value) {
    const ConfigOption* opt = Config_FindOptionByKey(key);
    if (opt && opt->type == CFG_BOOL) {
        *(bool*)opt->ptr = value;
        OnConfigChange();
    }
}

// Separator view - using backgroundColor instead of custom drawRect to avoid Metal shader compilation issues
// The AppBarBorderView class is kept for compatibility but now uses layer-backed approach
@implementation AppBarBorderView
- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.wantsLayer = YES;
        self.layer.backgroundColor = [[NSColor separatorColor] CGColor];
    }
    return self;
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

        // Compute list width from content: icon+toggle+name(max)+gap+desc(max)+padding
        NSFont* nameFont = [NSFont fontWithName:@"Maple Mono" size:kNameFontSize] ?: [NSFont systemFontOfSize:kNameFontSize weight:NSFontWeightSemibold];
        NSFont* descFont = [NSFont fontWithName:@"Maple Mono" size:kDescFontSize] ?: [NSFont systemFontOfSize:kDescFontSize];
        CGFloat maxNW = [@"Interactive Drops" sizeWithAttributes:@{NSFontAttributeName: nameFont}].width + 4;
        // Measure longest description to ensure no truncation
        CGFloat maxDW = [@"Create portals with P+1/2, teleport with P+0. Based on PortalGoos by Moonaliss1" sizeWithAttributes:@{NSFontAttributeName: descFont}].width + kRowDescPaddingX;
        self.descLabelX = kRowNameX + maxNW + kRowDescGap;
        self.listWidth = self.descLabelX + maxDW + kRowDescPaddingX;
        fprintf(stderr, "[config] listWidth=%.0f descLabelX=%.0f maxNW=%.0f maxDW=%.0f\n",
                self.listWidth, self.descLabelX, maxNW, maxDW);

        // Compute detail width from content: instruction text + slider controls
        NSDictionary* font12 = @{NSFontAttributeName: [NSFont systemFontOfSize:kDetailLabelFontSize]};
        CGFloat labelW = 120; // max label width for slider names
        CGFloat valW = [@"100.00" sizeWithAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:kDetailValueFontSize]}].width + kDetailValuePad;
        CGFloat detailWidth = kDetailLeftPad + labelW + kDetailLabelGap + kDetailSliderMinWidth + kDetailLabelGap + valW + kDetailRightPad;
        CGFloat windowWidth = self.listWidth + kSeparatorWidth + detailWidth;

        self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, windowWidth, kWindowHeight)
                                                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskFullSizeContentView
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO];
        self.window.titleVisibility = NSWindowTitleHidden;
        self.window.titlebarAppearsTransparent = YES;
        self.window.backgroundColor = [NSColor clearColor];
        self.window.opaque = NO;
        self.window.title = @"Preferences";
        self.window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];

        // Hide zoom and miniaturize buttons (keep only close)
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

        // Appbar with tab control
        NSView* appBar = [[NSView alloc] initWithFrame:NSMakeRect(0, kWindowHeight - kAppbarHeight, windowWidth, kAppbarHeight)];

        _tabControl = [[NSSegmentedControl alloc] initWithFrame:NSMakeRect((windowWidth - kTabBarWidth) / 2, kTabBarY, kTabBarWidth, kTabBarHeight)];
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
        NSView* appBarBorder = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(0, kWindowHeight - kAppbarHeight, windowWidth, kSeparatorWidth)];
        [contentView addSubview:appBarBorder];

        // Content container (below appbar)
        _contentContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, windowWidth, kTableHeight)];
        _contentContainer.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        [contentView addSubview:_contentContainer];

        // --- Behaviors tab: list + detail split ---
        _behaviorsContainer = [[NSView alloc] initWithFrame:_contentContainer.bounds];

        _detailView = [[BehaviorDetailView alloc] initWithFrame:NSMakeRect(self.listWidth + kSeparatorWidth, 0, detailWidth, kTableHeight)];
        [_behaviorsContainer addSubview:_detailView];

        NSView* separator = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(self.listWidth, 0, kSeparatorWidth, kTableHeight)];
        [_behaviorsContainer addSubview:separator];

        NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kTableHeight)];
        scrollView.hasVerticalScroller = NO;
        scrollView.borderType = NSNoBorder;
        scrollView.drawsBackground = NO;

        NSTableView* tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kTableHeight)];
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
    {
        NSString* hk = @(g_config.behaviors.breadCrumbs.hotkey.c_str());
        [self addRow:@"Breadcrumbs" key:@"behaviors.fun.breadCrumbs" desc:[NSString stringWithFormat:@"Hold %@ to drop breadcrumbs at cursor. Crumbs expire after %.0fs", hk, g_config.behaviors.breadCrumbs.lifetime]];
    }
    [self addRow:@"Hats" key:@"behaviors.fun.hats" desc:@"Put various hats on geese"];
    [self addRow:@"Rainbow" key:@"behaviors.fun.rainbow" desc:@"Cycle colors on all geese"];
    [self addRow:@"Acid" key:@"behaviors.fun.acid" desc:@"Geese spin and honk rapidly"];
    [self addRow:@"Anger" key:@"behaviors.fun.anger" desc:@"Geese get angry and punch things"];
    [self addRow:@"Autumn Leaves" key:@"behaviors.fun.autumnLeaves" desc:@"Piles of leaves accumulate on screen"];
    
    [self.configItems addObject:@{@"name": @"JOY", @"type": @"header"}];
    [self addRow:@"Avoidance" key:@"behaviors.fun.avoidance" desc:@"Goose dodges fast-moving cursor"];
    [self addRow:@"Boredom Sigh" key:@"behaviors.fun.boredom" desc:@"Goose sighs dramatically after 10+ minutes idle"];
    [self addRow:@"Window Peeking" key:@"behaviors.fun.peeking" desc:@"Goose peeks head around monitor bezel at screen edges"];
    [self addRow:@"Custom Affirmations" key:@"behaviors.fun.affirmations" desc:@"Goose drops configurable positive messages"];
    [self addRow:@"Interactive Drops" key:@"behaviors.fun.interactiveDrops" desc:@"Goose drops puddles that splash or flowers that grow"];
    [self addRow:@"Sonic Mode" key:@"behaviors.fun.sonicMode" desc:@"Goose moves at supersonic speed"];
    [self addRow:@"Toys" key:@"behaviors.fun.toysEnabled" desc:@"Scatter interactive toys for the goose"];

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
    if ([item[@"type"] isEqualToString:@"header"]) return kHeaderRowHeight;
    return kRowHeight;
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
            label.font = [NSFont fontWithName:@"Maple Mono" size:kNameFontSize] ?: [NSFont systemFontOfSize:kNameFontSize weight:NSFontWeightSemibold];
            label.textColor = [NSColor colorWithRed:0.9 green:0.1 blue:0.1 alpha:1.0];
        }
        label.frame = NSMakeRect(kRowPaddingX, 2, self.listWidth - kRowPaddingX * 2, kHeaderRowHeight - 4);
        label.stringValue = item[@"name"];
        return label;
    }

    if ([type isEqualToString:@"behavior"]) {
        BehaviorRowView* rowView = [tableView makeViewWithIdentifier:@"behaviorRow" owner:self];
        if (!rowView) {
            rowView = [[BehaviorRowView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kRowHeight)];
            rowView.identifier = @"behaviorRow";
        }

        rowView.configKey = item[@"key"];
        NSString* nameText = item[@"label"];
        NSString* descText = item[@"desc"] ?: @"";
        rowView.nameLabel.stringValue = nameText;
        rowView.descLabel.stringValue = descText;
        NSFont* df = rowView.descLabel.font;
        CGFloat descAvailableWidth = self.listWidth - self.descLabelX - kRowDescPaddingX;
        CGFloat dw = MIN([descText sizeWithAttributes:@{NSFontAttributeName: df}].width + kRowDescPaddingX, descAvailableWidth);
        rowView.descLabel.frame = NSMakeRect(self.descLabelX, (kRowHeight - kDescFontSize) / 2 - 2, dw, kDescFontSize + 2);
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
