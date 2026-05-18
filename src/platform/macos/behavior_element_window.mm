#import "behavior_element_window.h"
#import "coordinate_system.h"
#include <cstdio>

static FILE* s_debugFile = NULL;

static void BELog(const char* fmt, ...) {
    if (!s_debugFile) {
        s_debugFile = fopen("/tmp/cadgoose_behavior_window.log", "w");
    }
    if (!s_debugFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(s_debugFile, fmt, args);
    va_end(args);
    fprintf(s_debugFile, "\n");
    fflush(s_debugFile);
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
        self.ignoresMouseEvents = YES;
        self.acceptsMouseMovedEvents = NO;
        self.level = NSStatusWindowLevel; // Below goose window
        self.hasShadow = NO;
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

    _lastX = x; _lastY = y; _lastW = w; _lastH = h;
    _hasLastPosition = YES;

    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    ScreenPoint screenOrigin = CoordTransform::DeviceToScreenMacOS({x, y + h}, screenH);

    NSRect newFrame = NSMakeRect(screenOrigin.x, screenOrigin.y, w, h);
    [self setFrame:newFrame display:YES];
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
