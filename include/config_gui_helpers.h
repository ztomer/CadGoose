#pragma once
#import <Cocoa/Cocoa.h>
#include <string>

#define LIST_WIDTH 545 // computed at runtime in init, kept as default for header usage
#define WINDOW_HEIGHT 520
#define APPBAR_HEIGHT 44
#define TABLE_HEIGHT (WINDOW_HEIGHT - APPBAR_HEIGHT)

bool s_getBoolForKey(const std::string& key);
void s_setFloatValue(const std::string& key, float value);
void s_setBoolValue(const std::string& key, bool value);

#include "hotkey.h"

extern NSMutableArray* g_configItemsForAccess;

@interface AppBarBorderView : NSView @end

@interface ColorSwatchView : NSView
@property (nonatomic, strong) NSColor* color;
@property (nonatomic, copy) NSString* colorPrefix;
@end

@class BehaviorDetailView;

@interface BehaviorRowView : NSView
@property (nonatomic, strong) NSSwitch* toggle;
@property (nonatomic, strong) NSTextField* nameLabel;
@property (nonatomic, strong) NSTextField* descLabel;
@property (nonatomic, copy) NSString* configKey;
@property (nonatomic, weak) BehaviorDetailView* detailView;
@property (nonatomic, getter=isSelected) BOOL selected;
- (void)openDetail;
@end

@interface PreviewGooseView : NSView
- (void)updatePreview;
@end

@interface BehaviorDetailView : NSView
@property (nonatomic, strong) NSTextField* titleLabel;
@property (nonatomic, strong) NSView* contentView;
@property (nonatomic, copy) NSString* configKey;
- (void)configureForBehavior:(NSString*)key;
- (void)addInstructionLabel:(NSString*)text atY:(float)y;
- (void)addSliderWithLabel:(NSString*)label min:(float)min max:(float)max value:(float)value atY:(float)y key:(NSString*)key;
- (float)addGeeseListAtY:(float)y;
- (void)gooseNameChanged:(NSTextField*)sender;
- (void)sliderChanged:(NSSlider*)sender;
- (void)valueFieldChanged:(NSTextField*)sender;
@end

@interface AITabView : NSView
@property (nonatomic, strong) NSTextField* statusLabel;
@property (nonatomic, strong) NSTextView* promptBody;
@property (nonatomic, strong) NSTextField* endpointField;
@property (nonatomic, strong) NSTextField* customModelField;
@property (nonatomic, strong) NSTextField* portLabel;
@property (nonatomic, strong) NSTextField* portField;
@property (nonatomic, strong) NSTextField* modelLabel;
@property (nonatomic, strong) NSPopUpButton* modelPopup;
@property (nonatomic, strong) NSButton* refreshBtn;
@property (nonatomic, strong) NSTextField* foundationNote;
@property (nonatomic) CGFloat personalitySliderWidth;
@property (nonatomic) CGFloat personalitySliderLeftX;
- (NSInteger)currentProvider;
- (void)setProvider:(NSInteger)idx;
- (int)currentPort;
- (void)setPort:(int)port;
- (NSString*)currentModelName;
- (void)setModelName:(NSString*)name;
- (void)updateCustomVisibility;
@end

// Category interfaces (implemented in separate files)
@interface AITabView (Prompts)
- (NSString*)promptPreviewForEvilLevel:(float)level;
- (void)evilSliderChanged:(NSSlider*)sender;
@end

@interface AITabView (Connection)
- (NSString*)modelsEndpointForTest;
- (void)refreshModels:(id)sender;
- (void)testConnection:(id)sender;
@end

@interface AppearanceTabView : NSView
@end

@interface ConfigGUIWindowController : NSWindowController <NSTableViewDelegate, NSTableViewDataSource>
@property (nonatomic, strong) NSTableView* behaviorsTable;
@property (nonatomic, strong) NSTableView* playTable;
@property (nonatomic, strong) NSMutableArray* configItems;
@property (nonatomic, strong) NSMutableArray* behaviorItems;
@property (nonatomic, strong) NSMutableArray* playItems;
@property (nonatomic, strong) BehaviorDetailView* detailBehavior;
@property (nonatomic, strong) BehaviorDetailView* detailPlay;
@property (nonatomic, strong) NSView* contentContainer;
@property (nonatomic, strong) NSView* behaviorsContainer;
@property (nonatomic, strong) NSView* playContainer;
@property (nonatomic, strong) AppearanceTabView* appearanceView;
@property (nonatomic, strong) AITabView* aiView;
@property (nonatomic, strong) NSSegmentedControl* tabControl;
@property (nonatomic) NSWindow* parentWindow;
@property (nonatomic) NSInteger selectedBehaviorRowIndex;
@property (nonatomic) NSInteger selectedPlayRowIndex;
@property (nonatomic) CGFloat listWidth;

- (void)prepareForDisplay;
+ (NSMutableArray*)configItemsForAccess;
@end
