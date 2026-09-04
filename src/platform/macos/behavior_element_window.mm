#import "behavior_element_window.h"
#import "coordinate_system.h"
#include "config.h"
#include "log.h"
#include <cstdarg>
#include <cstdio>

// Route through the centralized async logger so the render thread never
// touches disk. Gated on debug.toTerminal like the rest of the debug logging.
static void BELog(const char* fmt, ...) {
    if (!g_config.debug.toTerminal) return;
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    CG_DEBUG_ASYNC("BEWIN", "%s", buffer);
}

// ============================================================
// BehaviorElementContentView — draws via block
// ============================================================

@implementation BehaviorElementContentView {
    BehaviorElementDrawBlock _drawBlock;
}

- (instancetype)initWithFrame:(NSRect)frame drawBlock:(BehaviorElementDrawBlock)block {
    self = [super initWithFrame:frame];
    if (self) {
        _drawBlock = [block copy];
        self.wantsLayer = YES;
        self.layer.backgroundColor = [[NSColor clearColor] CGColor];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!_drawBlock) return;

    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) {
        BELog("BehaviorElementContentView drawRect called (frame %d)\n", frameCounter);
    }

    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);
    _drawBlock(ctx);
}

@end

// ============================================================
// BehaviorElementWindow — transparent floating window
// ============================================================

@implementation BehaviorElementWindow {
    BehaviorElementDrawBlock _drawBlock;
    float _lastX, _lastY, _lastW, _lastH;
    BOOL _hasLastPosition;
}

- (instancetype)initWithDrawBlock:(BehaviorElementDrawBlock)block
                         deviceX:(float)x deviceY:(float)y
                           width:(float)w height:(float)h {
    _drawBlock = [block copy];
    _lastX = x; _lastY = y; _lastW = w; _lastH = h;
    _hasLastPosition = NO;

    // Window is centered on the element position
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    ScreenPoint screenOrigin = CoordTransform::DeviceToScreenMacOS({x, y + h}, screenH);

    NSRect frame = NSMakeRect(screenOrigin.x, screenOrigin.y, w, h);

    self = [super initWithContentRect:frame
                            styleMask:NSWindowStyleMaskBorderless
                              backing:NSBackingStoreBuffered
                                defer:NO];
    if (self) {
        self.backgroundColor = [NSColor clearColor];
        [self setOpaque:NO];
        self.ignoresMouseEvents = YES;
        self.acceptsMouseMovedEvents = NO;
        self.level = NSStatusWindowLevel; // Default mid-layer; goose +5, leaves -5, footprints -10
        self.hasShadow = NO;
        self.releasedWhenClosed = NO;
        self.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                  NSWindowCollectionBehaviorIgnoresCycle;

        BehaviorElementContentView* contentView = [[BehaviorElementContentView alloc] initWithFrame:self.contentView.bounds drawBlock:_drawBlock];
        self.contentView = contentView;

        [self orderFront:nil];
    }
    return self;
}

- (void)updatePosition:(float)x y:(float)y width:(float)w height:(float)h {
    if (_hasLastPosition &&
        std::abs(x - _lastX) < 0.1f && std::abs(y - _lastY) < 0.1f &&
        std::abs(w - _lastW) < 0.1f && std::abs(h - _lastH) < 0.1f) {
        return;
    }

    bool sizeChanged = (!_hasLastPosition || std::abs(w - _lastW) >= 0.1f || std::abs(h - _lastH) >= 0.1f);

    _lastX = x; _lastY = y; _lastW = w; _lastH = h;
    _hasLastPosition = YES;

    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    ScreenPoint screenOrigin = CoordTransform::DeviceToScreenMacOS({x, y + h}, screenH);

    if (sizeChanged) {
        NSRect newFrame = NSMakeRect(screenOrigin.x, screenOrigin.y, w, h);
        // display:NO — every caller issues setNeedsDisplay:YES right after, so a
        // synchronous redraw here is a redundant second draw on every resize.
        [self setFrame:newFrame display:NO];
    } else {
        [self setFrameOrigin:NSMakePoint(screenOrigin.x, screenOrigin.y)];
    }
}

- (void)closeAndRemove {
    [self close];
}

@end

// ============================================================
// BehaviorElementWindowManager — manages all behavior element windows
// ============================================================

@interface BehaviorElementWindowManager () {
    NSMutableDictionary<NSNumber*, BehaviorElementWindow*>* _windows;
    NSInteger _nextId;
}
@end

@implementation BehaviorElementWindowManager

+ (instancetype)shared {
    static BehaviorElementWindowManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[BehaviorElementWindowManager alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _windows = [NSMutableDictionary dictionary];
        _nextId = 1;
    }
    return self;
}

- (void)syncWindows {
    // Remove windows that are no longer valid
    NSMutableArray* toRemove = [NSMutableArray array];
    for (NSNumber* key in _windows) {
        BehaviorElementWindow* win = _windows[key];
        // Windows are removed explicitly by behaviors, not by scanning
        // This is a no-op by default; behaviors call closeAndRemove directly
    }
    for (NSNumber* key in toRemove) {
        [_windows removeObjectForKey:key];
    }
}

- (NSNumber*)registerWindow:(BehaviorElementWindow*)window {
    NSNumber* key = @(_nextId++);
    _windows[key] = window;
    return key;
}

- (void)unregisterWindow:(NSNumber*)key {
    [_windows removeObjectForKey:key];
}

- (void)closeAll {
    for (NSNumber* key in _windows) {
        [_windows[key] close];
    }
    [_windows removeAllObjects];
}

@end
