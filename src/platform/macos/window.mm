#import "window.h"
#import "renderer.h"
#import "coordinate_system.h"
#import <CoreGraphics/CoreGraphics.h>
#include <list>
#include "goose.h"

#if defined(__APPLE__)
extern bool g_debugMode;
#define DEBUG_LOG(fmt, ...) do { \
    if (g_debugMode) { \
        time_t now = time(nullptr); \
        struct tm* tm = localtime(&now); \
        char ts[32]; strftime(ts, sizeof(ts), "%H:%M:%S", tm); \
        fprintf(stderr, "[%s] DEBUG: " fmt "\n", ts, ##__VA_ARGS__); \
    } \
} while(0)
#define LOG(fmt, ...) fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__)
#endif

static constexpr float kGooseWindowSize = 600.0f;

@implementation GooseWindow

- (instancetype)initWithScreen:(NSScreen*)screen {
    DEBUG_LOG("GooseWindow initWithScreen START");

    NSRect rect = NSMakeRect(0, 0, kGooseWindowSize, kGooseWindowSize);
    self = [super initWithContentRect:rect
                            styleMask:NSWindowStyleMaskBorderless
                              backing:NSBackingStoreBuffered
                                defer:NO
                               screen:screen];
    DEBUG_LOG("GooseWindow super init done, self=%p");

    if (self) {
        self.level = NSFloatingWindowLevel;
        DEBUG_LOG("  level=%ld", (long)self.level);

        self.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                   NSWindowCollectionBehaviorFullScreenAuxiliary |
                                   NSWindowCollectionBehaviorStationary;
        DEBUG_LOG("  collectionBehavior set");

        self.backgroundColor = [NSColor clearColor];
        self.opaque = NO;
        self.hasShadow = NO;
        self.ignoresMouseEvents = YES;
        self.acceptsMouseMovedEvents = NO;
        self.collectionBehavior |= NSWindowCollectionBehaviorIgnoresCycle;

        DEBUG_LOG("  window props set: opaque=%d, ignoresMouse=%d", self.opaque, self.ignoresMouseEvents);

        self.gooseView = [[GooseView alloc] initWithFrame:rect];
        DEBUG_LOG("  gooseView created: %p", self.gooseView);

        self.contentView = self.gooseView;
        DEBUG_LOG("  contentView set");
    } else {
        LOG("ERROR: GooseWindow init returned nil!");
    }
    DEBUG_LOG("GooseWindow initWithScreen END");
    return self;
}

- (void)centerOnDevicePoint:(DevicePoint)devicePt {
    NSScreen* windowScreen = self.screen ?: [NSScreen mainScreen];
    float screenH = (float)windowScreen.frame.size.height;

    ScreenPoint screenPt = CoordTransform::DeviceToScreenMacOS(devicePt, screenH);

    NSRect frame = self.frame;
    NSPoint newOrigin = NSMakePoint(screenPt.x - frame.size.width / 2.0, screenPt.y - frame.size.height / 2.0);

    // Clamp to screen bounds
    NSRect screenFrame = windowScreen.frame;
    newOrigin.x = MAX(screenFrame.origin.x, MIN(newOrigin.x, screenFrame.origin.x + screenFrame.size.width - frame.size.width));
    newOrigin.y = MAX(screenFrame.origin.y, MIN(newOrigin.y, screenFrame.origin.y + screenFrame.size.height - frame.size.height));

    // Only move if position changed significantly
    if (std::abs(frame.origin.x - newOrigin.x) < 2.0f && std::abs(frame.origin.y - newOrigin.y) < 2.0f) {
        return;
    }

    [self setFrameOrigin:newOrigin];
}

- (BOOL)canBecomeKeyWindow { return NO; }
- (BOOL)canBecomeMainWindow { return NO; }

@end

@interface WindowManager ()
@property (nonatomic, strong) NSMutableArray<GooseWindow*>* windows;
@end

@implementation WindowManager

+ (instancetype)shared {
    static WindowManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[WindowManager alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _windows = [NSMutableArray array];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(screenParametersChanged:)
                                                     name:NSApplicationDidChangeScreenParametersNotification
                                                   object:nil];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)screenParametersChanged:(NSNotification*)notification {
    DEBUG_LOG("Screen parameters changed, re-evaluating windows...");
    for (GooseWindow* w in _windows) {
        [w close];
    }
    [_windows removeAllObjects];
    [self createWindowsForAllScreens];
    for (GooseWindow* w in _windows) {
        [w.gooseView startAnimation];
        [w orderFront:nil];
    }
}

- (void)createWindowsForAllScreens {
    for (NSScreen* screen in NSScreen.screens) {
        [self createWindowForScreen:screen];
    }
}

- (void)createWindowForScreen:(NSScreen*)screen {
    GooseWindow* window = [[GooseWindow alloc] initWithScreen:screen];
    [_windows addObject:window];
}

- (void)updateWindowPositionsForGeese:(const std::list<Goose>*)geese {
    if (!geese || geese->empty()) return;

    for (GooseWindow* window in self.windows) {
        const Goose* primaryGoose = &geese->front();
        DevicePoint devicePt = {primaryGoose->pos.x, primaryGoose->pos.y};
        [window centerOnDevicePoint:devicePt];
    }
}

- (void)updateWindowForScreen:(NSScreen*)screen {
    for (GooseWindow* window in self.windows) {
        if (window.screen == screen) {
            NSRect frame = [screen frame];
            [window setFrame:frame display:YES];
            [window.gooseView setFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height)];
            break;
        }
    }
}

- (GooseWindow*)windowForScreen:(NSScreen*)screen {
    for (GooseWindow* window in self.windows) {
        if (window.screen == screen) {
            return window;
        }
    }
    return nil;
}

- (NSArray<GooseWindow*>*)windows {
    return _windows;
}

@end