#import "window.h"
#import "renderer.h"
#import <CoreGraphics/CoreGraphics.h>

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

@implementation GooseWindow

- (instancetype)initWithScreen:(NSScreen*)screen contentRect:(NSRect)rect {
    DEBUG_LOG("GooseWindow initWithScreen START, rect=%s", NSStringFromRect(rect).UTF8String);
    
    self = [super initWithContentRect:rect
                            styleMask:NSWindowStyleMaskBorderless
                              backing:NSBackingStoreBuffered
                                defer:NO
                               screen:screen];
    DEBUG_LOG("GooseWindow super init done, self=%p", self);
    
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
        self.ignoresMouseEvents = NO;
        self.acceptsMouseMovedEvents = YES;
        self.collectionBehavior |= NSWindowCollectionBehaviorIgnoresCycle;
        
        DEBUG_LOG("  window props set: opaque=%d, ignoresMouse=%d", self.opaque, self.ignoresMouseEvents);

        DEBUG_LOG("  creating GooseView with frame %s", NSStringFromRect(rect).UTF8String);
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
    }
    return self;
}

- (void)createWindowsForAllScreens {
    for (NSScreen* screen in NSScreen.screens) {
        [self createWindowForScreen:screen];
    }
}

- (void)createWindowForScreen:(NSScreen*)screen {
    NSRect rect = [screen frame];
    GooseWindow* window = [[GooseWindow alloc] initWithScreen:screen contentRect:rect];
    [_windows addObject:window];
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