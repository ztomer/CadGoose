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

@interface BehaviorRowView : NSView
@property (nonatomic, strong) NSButton* toggle;
@property (nonatomic, strong) NSTextField* iconLabel;
@property (nonatomic, strong) NSTextField* nameLabel;
@property (nonatomic, strong) NSTextField* descLabel;
@property (nonatomic, copy) NSString* configKey;
@property (nonatomic, weak) id target;
@property (nonatomic) SEL detailAction;
@property (nonatomic, getter=isSelected) BOOL selected;
- (void)openDetail;
+ (NSString*)iconForConfigKey:(NSString*)key;
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
- (NSInteger)currentProvider;
- (void)setProvider:(NSInteger)idx;
- (int)currentPort;
- (void)setPort:(int)port;
- (NSString*)currentModelName;
- (void)setModelName:(NSString*)name;
- (void)updateCustomVisibility;
- (NSString*)promptPreviewForEvilLevel:(float)level;
- (NSString*)modelsEndpointForTest;
- (void)refreshModels:(id)sender;
@end

@interface AppearanceTabView : NSView
@end

@interface ConfigGUIWindowController : NSWindowController <NSTableViewDelegate, NSTableViewDataSource>
@property (nonatomic, strong) NSTableView* behaviorsTable;
@property (nonatomic, strong) NSMutableArray* configItems;
@property (nonatomic, strong) BehaviorDetailView* detailView;
@property (nonatomic, strong) NSView* contentContainer;
@property (nonatomic, strong) NSView* behaviorsContainer;
@property (nonatomic, strong) AppearanceTabView* appearanceView;
@property (nonatomic, strong) AITabView* aiView;
@property (nonatomic, strong) NSSegmentedControl* tabControl;
@property (nonatomic) NSWindow* parentWindow;
@property (nonatomic) NSInteger selectedRowIndex;
@property (nonatomic) CGFloat listWidth;
@property (nonatomic) CGFloat descLabelX;
- (void)prepareForDisplay;
+ (NSMutableArray*)configItemsForAccess;
@end
