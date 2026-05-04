#import <Cocoa/Cocoa.h>

@class GooseView;

@interface GooseWindow : NSWindow
@property (nonatomic, strong) GooseView* gooseView;
@end

@interface WindowManager : NSObject
+ (instancetype)shared;
- (void)createWindowsForAllScreens;
- (void)updateWindowForScreen:(NSScreen*)screen;
- (GooseWindow*)windowForScreen:(NSScreen*)screen;
- (NSArray<GooseWindow*>*)windows;
@end