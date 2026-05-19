#import <Cocoa/Cocoa.h>

class Goose;
@class GooseView;

// Forward declare DevicePoint for ObjC header
struct DevicePoint;

@interface GooseWindow : NSWindow
@property (nonatomic, strong) GooseView* gooseView;
- (void)centerOnDevicePoint:(DevicePoint)devicePt;
- (void)updateSizeForGoose:(const Goose*)goose;
@end

#ifdef __cplusplus
#include <vector>
@interface WindowManager : NSObject
+ (instancetype)shared;
- (void)createWindowsForAllScreens;
- (void)updateWindowPositionsForGeese:(const std::vector<Goose*>&)geese;
- (void)updateWindowForScreen:(NSScreen*)screen;
- (GooseWindow*)windowForScreen:(NSScreen*)screen;
- (NSArray<GooseWindow*>*)windows;
@end
#else
@interface WindowManager : NSObject
+ (instancetype)shared;
- (void)createWindowsForAllScreens;
- (void)updateWindowForScreen:(NSScreen*)screen;
- (GooseWindow*)windowForScreen:(NSScreen*)screen;
- (NSArray<GooseWindow*>*)windows;
@end
#endif