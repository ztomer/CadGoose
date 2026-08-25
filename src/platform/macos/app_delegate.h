// app_delegate.h
// NSApplicationDelegate for CadGoose: menu-bar icon, behavior toggles,
// preferences / presence panel wiring. Split out of main.mm along its seams.

#import <Cocoa/Cocoa.h>

extern bool g_mcpMode;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSStatusItem* statusItem;
@property (nonatomic, strong) NSMenuItem* muteMenuItem;
- (void)setupMenubar;
- (void)addBehaviorItem:(NSString*)title configKey:(NSString*)key toMenu:(NSMenu*)menu;
- (void)toggleBehavior:(NSMenuItem*)sender;
- (bool*)getBehaviorFlag:(NSString*)key;
- (void)openPresencePanel:(id)sender;
- (void)toggleMute:(id)sender;
- (void)updateMuteMenuItem;
@end
