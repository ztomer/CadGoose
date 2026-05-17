#import "effect_window.h"
#import "world.h"
#import "config.h"
#import "coordinate_system.h"
#import "goose_drawing.h"
#include <cmath>
#include <cstdio>
#include <ctime>

extern std::list<LeafPile> g_leafPiles;
extern RingBuffer<Footprint, kMaxFootprints> g_footprints;

static constexpr size_t kMaxEffectWindows = 50;

// ============================================================
// EffectContentView — Draws a single effect
// ============================================================

@implementation EffectContentView {
    EffectType _effectType;
    float _posX;
    float _posY;
    float _radius;
    double _currentTime;
}

- (instancetype)initWithFrame:(NSRect)frame effectType:(EffectType)type {
    self = [super initWithFrame:frame];
    if (self) {
        _effectType = type;
        self.wantsLayer = YES;
        self.layer.backgroundColor = [[NSColor clearColor] CGColor];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)setPosX:(float)x {
    _posX = x;
    [self setNeedsDisplay:YES];
}

- (void)setPosY:(float)y {
    _posY = y;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);
    CGContextSaveGState(ctx);

    if (_effectType == EffectTypeLeafPile) {
        // Find the leaf pile at this position and draw it
        for (auto& pile : g_leafPiles) {
            if (std::abs(pile.pos.x - _posX) < 1.0f && std::abs(pile.pos.y - _posY) < 1.0f) {
                // Create a temporary list with just this pile for DrawLeaves
                std::list<LeafPile> singlePile;
                singlePile.push_back(pile);
                // Draw centered in view
                CGContextTranslateCTM(ctx, -_posX + self.bounds.size.width * 0.5f,
                                          -_posY + self.bounds.size.height * 0.5f);
                DrawLeaves(ctx, singlePile, _currentTime);
                break;
            }
        }
    }

    CGContextRestoreGState(ctx);
}

@end

// ============================================================
// EffectWindow — Transparent floating window for effects
// ============================================================

@implementation EffectWindow {
    EffectContentView* _contentView;
    float _lastPosX;
    float _lastPosY;
    bool _hasLastPosition;
}

- (BOOL)canBecomeKeyWindow { return NO; }
- (BOOL)canBecomeMainWindow { return NO; }
- (BOOL)isOpaque { return NO; }

- (instancetype)initWithType:(EffectType)type posX:(float)x posY:(float)y radius:(float)rad {
    _hasLastPosition = false;

    float size = rad * 2.0f * g_config.general.globalScale;
    size = std::max(size, 40.0f); // minimum 40x40

    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;

    // Convert DEVICE position to SCREEN coords (bottom-left origin)
    DevicePoint devicePt = {x, y};
    ScreenPoint screenPt = CoordTransform::DeviceToScreenMacOS(devicePt, screenH);

    NSRect frame = NSMakeRect(screenPt.x - size * 0.5f, screenPt.y - size * 0.5f, size, size);

    self = [super initWithContentRect:frame
                            styleMask:NSWindowStyleMaskBorderless
                              backing:NSBackingStoreBuffered
                                defer:NO];
    if (self) {
        _effectType = type;
        _posX = x;
        _posY = y;
        _radius = rad;

        self.backgroundColor = [NSColor clearColor];
        self.ignoresMouseEvents = YES; // Always click-through
        self.level = NSStatusWindowLevel;
        self.hasShadow = NO;
        self.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                  NSWindowCollectionBehaviorIgnoresCycle;

        _contentView = [[EffectContentView alloc] initWithFrame:self.contentView.bounds effectType:type];
        _contentView.posX = x;
        _contentView.posY = y;
        _contentView.radius = rad;
        self.contentView = _contentView;

        [self orderFront:nil];
    }
    return self;
}

- (void)updatePosition {
    if (std::abs(_posX - _lastPosX) < 0.1f && std::abs(_posY - _lastPosY) < 0.1f) {
        return;
    }

    _lastPosX = _posX;
    _lastPosY = _posY;
    _hasLastPosition = true;

    float size = _radius * 2.0f * g_config.general.globalScale;
    size = std::max(size, 40.0f);

    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;

    DevicePoint devicePt = {_posX, _posY};
    ScreenPoint screenPt = CoordTransform::DeviceToScreenMacOS(devicePt, screenH);

    NSRect newFrame = NSMakeRect(screenPt.x - size * 0.5f, screenPt.y - size * 0.5f, size, size);
    [self setFrame:newFrame display:YES];

    _contentView.posX = _posX;
    _contentView.posY = _posY;
    _contentView.radius = _radius;
}

- (void)closeAndRemove {
    [self close];
}

@end

// ============================================================
// EffectWindowManager — Circular buffer for effect windows
// ============================================================

@interface EffectWindowManager ()
@property (nonatomic, strong) NSMutableArray* windows;
@property (nonatomic, assign) size_t head;
@property (nonatomic, assign) size_t count;
@end

@implementation EffectWindowManager

+ (instancetype)shared {
    static EffectWindowManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[EffectWindowManager alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _windows = [NSMutableArray arrayWithCapacity:kMaxEffectWindows];
        for (size_t i = 0; i < kMaxEffectWindows; i++) {
            [_windows addObject:[NSNull null]];
        }
        _head = 0;
        _count = 0;
    }
    return self;
}

- (void)addWindow:(EffectWindow*)window {
    if (_count >= kMaxEffectWindows) {
        // Circular buffer full — remove oldest
        id obj = _windows[_head];
        if ([obj isKindOfClass:[EffectWindow class]]) {
            [(EffectWindow*)obj closeAndRemove];
        }
        [_windows replaceObjectAtIndex:_head withObject:window];
        _head = (_head + 1) % kMaxEffectWindows;
    } else {
        [_windows addObject:window];
        _count++;
    }
}

- (void)syncWindows {
    // Remove windows for leaf piles that no longer exist
    for (size_t i = 0; i < _count; i++) {
        size_t idx = (_head + i) % kMaxEffectWindows;
        id obj = _windows[idx];
        if (![obj isKindOfClass:[EffectWindow class]]) continue;

        EffectWindow* win = (EffectWindow*)obj;
        BOOL pileStillExists = NO;
        for (auto& pile : g_leafPiles) {
            if (std::abs(pile.pos.x - win.posX) < 1.0f &&
                std::abs(pile.pos.y - win.posY) < 1.0f) {
                pileStillExists = YES;
                break;
            }
        }
        if (!pileStillExists) {
            [win closeAndRemove];
            [_windows replaceObjectAtIndex:idx withObject:[NSNull null]];
        }
    }

    // Create windows for new leaf piles
    for (auto& pile : g_leafPiles) {
        BOOL exists = NO;
        for (size_t i = 0; i < _count; i++) {
            size_t idx = (_head + i) % kMaxEffectWindows;
            id obj = _windows[idx];
            if (![obj isKindOfClass:[EffectWindow class]]) continue;

            EffectWindow* win = (EffectWindow*)obj;
            if (std::abs(pile.pos.x - win.posX) < 1.0f &&
                std::abs(pile.pos.y - win.posY) < 1.0f) {
                exists = YES;
                break;
            }
        }
        if (!exists) {
            EffectWindow* win = [[EffectWindow alloc] initWithType:EffectTypeLeafPile
                                                              posX:pile.pos.x
                                                              posY:pile.pos.y
                                                            radius:pile.rad];
            [self addWindow:win];
        }
    }

    // Update positions for existing windows
    for (size_t i = 0; i < _count; i++) {
        size_t idx = (_head + i) % kMaxEffectWindows;
        id obj = _windows[idx];
        if (![obj isKindOfClass:[EffectWindow class]]) continue;

        EffectWindow* win = (EffectWindow*)obj;
        [win updatePosition];
    }
}

- (void)closeAll {
    for (size_t i = 0; i < _count; i++) {
        size_t idx = (_head + i) % kMaxEffectWindows;
        id obj = _windows[idx];
        if ([obj isKindOfClass:[EffectWindow class]]) {
            [(EffectWindow*)obj closeAndRemove];
        }
    }
    _head = 0;
    _count = 0;
}

@end
