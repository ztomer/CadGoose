// config_gui.mm
// Free functions, ConfigGUIWindowController with tabs, and entry points
#import "config_gui_helpers.h"
#include "config.h"
#include "world.h"

// --- Layout constants ---
static constexpr float kWindowHeight = 520.0f;
static constexpr float kAppbarHeight = 44.0f;
static constexpr float kTableHeight = kWindowHeight - kAppbarHeight;
static constexpr float kRowHeight = 28.0f;
static constexpr float kHeaderRowHeight = 28.0f;
// Pixels of empty space ABOVE the header label (visually: gap between the
// previous group's last item and this category title). Below the label we
// leave just enough room to clear the text descenders.
static constexpr float kHeaderTopMargin = 12.0f;
static constexpr float kHeaderBottomMargin = 2.0f;

static constexpr float kDetailWidth = 182.0f;
static constexpr float kDetailLeftPad = 8.0f;
static constexpr float kDetailLabelGap = 4.0f;
static constexpr float kDetailSliderMinWidth = 80.0f;
static constexpr float kDetailValuePad = 6.0f;
static constexpr float kDetailRightPad = 6.0f;
static constexpr float kSeparatorWidth = 1.0f;
static constexpr float kTabBarWidth = 260.0f;
static constexpr float kTabBarY = 10.0f;
static constexpr float kTabBarHeight = 24.0f;


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

        // Row: name(12,w=140) desc(154,w=230) gap(8) toggle(44) pad(8) = 444
        self.listWidth = 444.0f;
        CGFloat windowWidth = self.listWidth + kSeparatorWidth + kDetailWidth;
        fprintf(stderr, "[config] listWidth=%.0f detailWidth=%.0f\n", self.listWidth, kDetailWidth);

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
        _tabControl.segmentCount = 4;
        [_tabControl setLabel:@"Behaviors" forSegment:0];
        [_tabControl setLabel:@"Play" forSegment:1];
        [_tabControl setLabel:@"Appearance" forSegment:2];
        [_tabControl setLabel:@"AI" forSegment:3];
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

        _detailBehavior = [[BehaviorDetailView alloc] initWithFrame:NSMakeRect(self.listWidth + kSeparatorWidth, 0, kDetailWidth, kTableHeight)];
        [_behaviorsContainer addSubview:_detailBehavior];

        NSView* behaviorSeparator = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(self.listWidth, 0, kSeparatorWidth, kTableHeight)];
        [_behaviorsContainer addSubview:behaviorSeparator];

        {
            NSScrollView* sv = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kTableHeight)];
            sv.hasVerticalScroller = NO;
            sv.borderType = NSNoBorder;
            sv.drawsBackground = NO;

            NSTableView* tv = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kTableHeight)];
            tv.headerView = nil;
            tv.delegate = self;
            tv.dataSource = self;
            tv.allowsEmptySelection = YES;
            tv.selectionHighlightStyle = NSTableViewSelectionHighlightStyleNone;
            tv.backgroundColor = [NSColor clearColor];
            tv.intercellSpacing = NSMakeSize(0, 4);

            NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"main"];
            col.width = self.listWidth;
            [tv addTableColumn:col];

            self.behaviorsTable = tv;
            sv.documentView = tv;
            [_behaviorsContainer addSubview:sv];
        }
        [_contentContainer addSubview:_behaviorsContainer];

        // --- Play tab: list + detail split ---
        _playContainer = [[NSView alloc] initWithFrame:_contentContainer.bounds];
        _playContainer.hidden = YES;

        _detailPlay = [[BehaviorDetailView alloc] initWithFrame:NSMakeRect(self.listWidth + kSeparatorWidth, 0, kDetailWidth, kTableHeight)];
        [_playContainer addSubview:_detailPlay];

        NSView* playSeparator = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(self.listWidth, 0, kSeparatorWidth, kTableHeight)];
        [_playContainer addSubview:playSeparator];

        {
            NSScrollView* sv = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kTableHeight)];
            sv.hasVerticalScroller = NO;
            sv.borderType = NSNoBorder;
            sv.drawsBackground = NO;

            NSTableView* tv = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kTableHeight)];
            tv.headerView = nil;
            tv.delegate = self;
            tv.dataSource = self;
            tv.allowsEmptySelection = YES;
            tv.selectionHighlightStyle = NSTableViewSelectionHighlightStyleNone;
            tv.backgroundColor = [NSColor clearColor];
            tv.intercellSpacing = NSMakeSize(0, 4);

            NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"main"];
            col.width = self.listWidth;
            [tv addTableColumn:col];

            self.playTable = tv;
            sv.documentView = tv;
            [_playContainer addSubview:sv];
        }
        [_contentContainer addSubview:_playContainer];

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

        self.behaviorItems = [NSMutableArray array];
        self.playItems = [NSMutableArray array];
        [self loadBehaviorItems];
        [self loadPlayItems];
    }
    return self;
}

- (void)addRow:(NSString*)label key:(NSString*)key desc:(NSString*)desc to:(NSMutableArray*)items {
    [items addObject:@{@"key": key, @"label": label, @"desc": desc, @"type": @"behavior"}];
}

- (void)loadBehaviorItems {
    [self.behaviorItems removeAllObjects];
    [self addRow:@"Ball" key:@"ball_enabled" desc:@"Pushable bouncing balls" to:self.behaviorItems];
    [self addRow:@"Hats" key:@"hats_enabled" desc:@"Put hats on geese" to:self.behaviorItems];
    [self addRow:@"Rainbow" key:@"rainbow_enabled" desc:@"Cycle colors on all geese" to:self.behaviorItems];
    [self addRow:@"Acid" key:@"acid_enabled" desc:@"Geese spin and honk rapidly" to:self.behaviorItems];
    [self addRow:@"Anger" key:@"anger_enabled" desc:@"Geese get angry and punch" to:self.behaviorItems];
    [self addRow:@"Autumn Leaves" key:@"autumn_leaves_enabled" desc:@"Leaf piles accumulate on screen" to:self.behaviorItems];
    [self addRow:@"Avoidance" key:@"avoidance_enabled" desc:@"Dodges fast-moving cursor" to:self.behaviorItems];
    [self addRow:@"Boredom Sigh" key:@"boredom_enabled" desc:@"Sighs after 10+ min idle" to:self.behaviorItems];
    [self addRow:@"Window Peeking" key:@"peeking_enabled" desc:@"Peeks at screen edges" to:self.behaviorItems];
    [self addRow:@"Interactive Drops" key:@"interactive_drops_enabled" desc:@"Drops puddles or flowers" to:self.behaviorItems];
    [self addRow:@"Toys" key:@"toys_enabled" desc:@"Scatter interactive toys" to:self.behaviorItems];

    // Keep g_configItemsForAccess pointing to behaviorItems for AX tests
    g_configItemsForAccess = self.behaviorItems;

    if (self.behaviorsTable) {
        [self.behaviorsTable reloadData];
    }
}

- (void)loadPlayItems {
    [self.playItems removeAllObjects];
    [self addRow:@"Breadcrumbs" key:@"breadcrumbs_enabled" desc:@"Hold key to drop crumbs at cursor" to:self.playItems];
    {
        NSString* hk = @(g_config.behaviors.honcker.hotkey.c_str());
        [self addRow:@"Honcker" key:@"honcker_enabled" desc:[NSString stringWithFormat:@"Press %@ to honk at cursor", hk] to:self.playItems];
    }
    {
        NSString* kO = @(g_config.behaviors.jail.hotkeyO.c_str());
        NSString* kP = @(g_config.behaviors.jail.hotkeyP.c_str());
        [self addRow:@"Jail" key:@"jail_enabled" desc:[NSString stringWithFormat:@"Set trap %@, trigger %@", kO, kP] to:self.playItems];
    }
    {
        NSString* k1 = @(g_config.portal.hotkey1.c_str());
        NSString* k2 = @(g_config.portal.hotkey2.c_str());
        NSString* k0 = @(g_config.portal.hotkey0.c_str());
        [self addRow:@"Portals" key:@"portals_enabled" desc:[NSString stringWithFormat:@"%@/%@ place, %@ toggle", k1, k2, k0] to:self.playItems];
    }
    [self addRow:@"Drag" key:@"drag_enabled" desc:@"Click and drag geese" to:self.playItems];
    [self addRow:@"Nametag" key:@"nametag_enabled" desc:@"Show goose name above head" to:self.playItems];
    [self addRow:@"Health" key:@"health_enabled" desc:@"Health bar system for geese" to:self.playItems];
    [self addRow:@"Pomodoro" key:@"pomodoro_enabled" desc:@"Pomodoro timer behavior" to:self.playItems];

    if (self.playTable) {
        [self.playTable reloadData];
    }
}

- (void)closeWindow:(id)sender {
    [self.window close];
}

- (void)reloadConfig:(id)sender {
    Config_Init();
    [self loadBehaviorItems];
    [self loadPlayItems];
}

- (void)prepareForDisplay {
    // Reset to Behaviors tab on every window open to avoid stale layer state
    // in Appearance/AI views (wantsLayer=YES subviews) causing mutex re-lock
    // on window layer tree rebuild after close+reopen.
    [_tabControl setSelectedSegment:0];
    _behaviorsContainer.hidden = NO;
    _playContainer.hidden = YES;
    _appearanceView.hidden = YES;
    _aiView.hidden = YES;
}

- (void)tabChanged:(NSSegmentedControl*)sender {
    NSInteger idx = sender.selectedSegment;
    _behaviorsContainer.hidden = (idx != 0);
    _playContainer.hidden = (idx != 1);
    _appearanceView.hidden = (idx != 2);
    _aiView.hidden = (idx != 3);
}

- (void)tableViewSelectionDidChange:(NSNotification*)note {
    NSTableView* table = (NSTableView*)note.object;
    BOOL isBehavior = (table == self.behaviorsTable);
    NSMutableArray* items = isBehavior ? self.behaviorItems : self.playItems;
    NSInteger row = table.selectedRow;
    NSInteger* selectedIdx = isBehavior ? &_selectedBehaviorRowIndex : &_selectedPlayRowIndex;
    BehaviorDetailView* detail = isBehavior ? self.detailBehavior : self.detailPlay;
    NSTableView* otherTable = isBehavior ? self.playTable : self.behaviorsTable;

    NSInteger prevRow = *selectedIdx;
    if (row >= 0 && row < (NSInteger)items.count) {
        NSDictionary* item = items[row];
        if ([item[@"type"] isEqualToString:@"behavior"]) {
            *selectedIdx = row;
            [detail configureForBehavior:item[@"key"]];
        } else {
            *selectedIdx = -1;
        }
    } else {
        *selectedIdx = -1;
    }
    // Reload affected rows in the sending table only
    NSIndexSet* rowsToReload = nil;
    if (prevRow >= 0 && row >= 0 && prevRow != row) {
        rowsToReload = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange((NSUInteger)MIN(prevRow, row), (NSUInteger)ABS(row - prevRow) + 1)];
    } else if (prevRow >= 0) {
        rowsToReload = [NSIndexSet indexSetWithIndex:(NSUInteger)prevRow];
    } else if (row >= 0) {
        rowsToReload = [NSIndexSet indexSetWithIndex:(NSUInteger)row];
    }
    if (rowsToReload) {
        [table reloadDataForRowIndexes:rowsToReload columnIndexes:[NSIndexSet indexSetWithIndex:0]];
    }
    // Reload the same index in the other table to clear its selection highlight
    [otherTable reloadDataForRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, items.count)] columnIndexes:[NSIndexSet indexSetWithIndex:0]];
}

- (CGFloat)tableView:(NSTableView*)tableView heightOfRow:(NSInteger)row {
    return kRowHeight;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
    if (tableView == self.behaviorsTable) return self.behaviorItems.count;
    return self.playItems.count;
}

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)column row:(NSInteger)row {
    BOOL isBehavior = (tableView == self.behaviorsTable);
    NSMutableArray* items = isBehavior ? self.behaviorItems : self.playItems;
    NSInteger* selectedIdx = isBehavior ? &_selectedBehaviorRowIndex : &_selectedPlayRowIndex;
    BehaviorDetailView* detail = isBehavior ? self.detailBehavior : self.detailPlay;

    NSDictionary* item = items[row];
    NSString* type = item[@"type"];

    if ([type isEqualToString:@"behavior"]) {
        BehaviorRowView* rowView = [tableView makeViewWithIdentifier:@"behaviorRow" owner:self];
        if (!rowView) {
            rowView = [[BehaviorRowView alloc] initWithFrame:NSMakeRect(0, 0, self.listWidth, kRowHeight)];
            rowView.identifier = @"behaviorRow";
        }

        rowView.configKey = item[@"key"];
        rowView.nameLabel.stringValue = item[@"label"];
        rowView.descLabel.stringValue = item[@"desc"] ?: @"";
        rowView.detailView = detail;
        rowView.selected = (row == *selectedIdx);

        std::string key = std::string([item[@"key"] UTF8String]);
        rowView.toggle.state = s_getBoolForKey(key) ? NSControlStateValueOn : NSControlStateValueOff;

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
