#import "window.h"
#import "renderer.h"
#import <CoreGraphics/CoreGraphics.h>

#if defined(__APPLE__)
#define DEBUG_LOG(fmt, ...) do { if (g_debugMode) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)
#endif

@implementation GooseWindow

- (instancetype)initWithScreen:(NSScreen*)screen contentRect:(NSRect)rect {
    self = [super initWithContentRect:rect
                            styleMask:NSWindowStyleMaskBorderless
                              backing:NSBackingStoreBuffered
                                defer:NO
                               screen:screen];
    if (self) {
        self.level = NSFloatingWindowLevel;
        self.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                   NSWindowCollectionBehaviorFullScreenAuxiliary |
                                   NSWindowCollectionBehaviorStationary;
        self.backgroundColor = [NSColor clearColor];
        self.opaque = NO;
        self.hasShadow = NO;
        self.ignoresMouseEvents = YES;
        self.acceptsMouseMovedEvents = YES;
        self.collectionBehavior |= NSWindowCollectionBehaviorIgnoresCycle;

        self.gooseView = [[GooseView alloc] initWithFrame:rect];
        self.contentView = self.gooseView;
    }
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
    [self.windows addObject:window];
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
    return self.windows;
}

@end