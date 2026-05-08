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

    std::string currentSection = "";
    for (size_t i = 0; i < g_configRegistry.size(); ++i) {
        const ConfigOption& opt = g_configRegistry[i];
        if (opt.section != currentSection) {
            currentSection = opt.section;
            NSString* sectionName = [[NSString stringWithUTF8String:opt.section] uppercaseString];
            [self.configItems addObject:@{@"name": sectionName, @"type": @"header"}];
        }
        
        NSString* name = [NSString stringWithFormat:@"  %s", opt.label];
        NSString* type = @"string";
        if (opt.type == CFG_BOOL) type = @"bool";
        else if (opt.type == CFG_INT) type = @"int";
        else if (opt.type == CFG_FLOAT) type = @"float";

        [self.configItems addObject:@{
            @"name": name,
            @"type": type,
            @"index": @(i)
        }];
    }
}

- (void)closeWindow:(id)sender {
    [self.window close];
}

- (ConfigOption*)getOptionForItem:(NSDictionary*)item {
    if (!item[@"index"]) return nullptr;
    size_t index = [item[@"index"] unsignedIntegerValue];
    if (index < g_configRegistry.size()) {
        return &g_configRegistry[index];
    }
    return nullptr;
}

- (id)getValueForItem:(NSDictionary*)item {
    ConfigOption* opt = [self getOptionForItem:item];
    if (!opt || !opt->ptr) return @"";

    if (opt->type == CFG_BOOL) {
        bool val = *(bool*)opt->ptr;
        return @(val);
    } else if (opt->type == CFG_INT) {
        int val = *(int*)opt->ptr;
        return @(val);
    } else if (opt->type == CFG_FLOAT) {
        float val = *(float*)opt->ptr;
        return @(val);
    } else {
        std::string val = *(std::string*)opt->ptr;
        return [NSString stringWithUTF8String:val.c_str()];
    }
}

- (void)setValue:(id)value forItem:(NSDictionary*)item {
    ConfigOption* opt = [self getOptionForItem:item];
    if (!opt || !opt->ptr) return;

    if (opt->type == CFG_BOOL) {
        *(bool*)opt->ptr = [value boolValue];
    } else if (opt->type == CFG_INT) {
        int val = [value intValue];
        val = std::clamp(val, static_cast<int>(opt->min), static_cast<int>(opt->max));
        *(int*)opt->ptr = val;
    } else if (opt->type == CFG_FLOAT) {
        float val = [value floatValue];
        val = std::clamp(val, opt->min, opt->max);
        *(float*)opt->ptr = val;
    } else {
        if ([value isKindOfClass:[NSString class]]) {
            *(std::string*)opt->ptr = [value UTF8String];
        }
    }
    if (opt->onChange) opt->onChange();
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
            id val = [self getValueForItem:item];
            checkbox.state = [val boolValue] ? NSControlStateValueOn : NSControlStateValueOff;
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
            id value = [self getValueForItem:item];
            if ([type isEqualToString:@"int"]) {
                field.stringValue = [NSString stringWithFormat:@"%d", [value intValue]];
            } else if ([type isEqualToString:@"float"]) {
                field.stringValue = [NSString stringWithFormat:@"%.1f", [value floatValue]];
            } else {
                field.stringValue = [value description];
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
    id currentVal = [self getValueForItem:item];
    bool newVal = ![currentVal boolValue];
    [self setValue:@(newVal) forItem:item];
    sender.state = newVal ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)controlTextDidEndEditing:(NSNotification*)notification {
    NSTextField* field = notification.object;
    NSInteger row = field.tag;
    if (row < 0 || row >= (NSInteger)self.configItems.count) return;

    NSDictionary* item = self.configItems[row];
    NSString* type = item[@"type"];

    if ([type isEqualToString:@"int"]) {
        [self setValue:@([field.stringValue intValue]) forItem:item];
    } else if ([type isEqualToString:@"float"]) {
        [self setValue:@([field.stringValue floatValue]) forItem:item];
    } else {
        [self setValue:field.stringValue forItem:item];
    }
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