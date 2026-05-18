#import "effect_window.h"
#import "effect_registration.h"
#import "world.h"
#import "config.h"
#import "coordinate_system.h"
#include <cmath>
#include <cstdio>



// Pomodoro bed accessor from behavior_pomodoro.cpp
extern PomodoroBedInfo Pomodoro_GetBedInfo(int gooseId);

static constexpr size_t kMaxEffectWindows = 50;
static constexpr float kEffectWindowMinSize = 40.0f;

// ============================================================
// EffectContentView — Draws a single effect
// ============================================================

@implementation EffectContentView {
    EffectType _effectType;
    float _posX;
    float _posY;
    float _radius;
    double _currentTime;
    void* _cgImage; // CGImageRef for portals
}

- (void)setCgImage:(void*)img {
    _cgImage = img;
    [self setNeedsDisplay:YES];
}

- (void*)cgImage {
    return _cgImage;
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

    if (_effectType == EffectTypeFootprint) {
        // Find the footprint at this position and draw it
        for (const auto& fp : g_world.footprints) {
            if (std::abs(fp.pos.x - _posX) < 1.0f && std::abs(fp.pos.y - _posY) < 1.0f) {
                float age = (float)(_currentTime - fp.timeSpawned);
                float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
                float alpha = std::max(0.0f, 1.0f - (age / life));
                if (alpha <= 0) break;

                CGContextSetRGBFillColor(ctx, g_config.color.footprint.r, g_config.color.footprint.g, g_config.color.footprint.b, alpha * g_config.color.footprintAlphaMultiplier);
                CGContextSaveGState(ctx);
                CGContextTranslateCTM(ctx, self.bounds.size.width * 0.5f, self.bounds.size.height * 0.5f);
                CGContextRotateCTM(ctx, fp.dir);
                CGContextFillEllipseInRect(ctx, CGRectMake(-g_config.render.footprintWidth/2, -g_config.render.footprintHeight/2, g_config.render.footprintWidth, g_config.render.footprintHeight));
                CGContextRestoreGState(ctx);
                break;
            }
        }
    } else if (_effectType == EffectTypePomodoroBed) {
        // Draw bed image centered in view
        if (_cgImage) {
            CGImageRef img = (CGImageRef)_cgImage;
            float imgW = (float)CGImageGetWidth(img);
            float imgH = (float)CGImageGetHeight(img);
            CGContextDrawImage(ctx, CGRectMake(self.bounds.size.width * 0.5f - imgW * 0.5f, self.bounds.size.height * 0.5f - imgH * 0.5f, imgW, imgH), img);
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

- (instancetype)initWithType:(EffectType)type posX:(float)x posY:(float)y radius:(float)rad cgImage:(void*)img {
    _hasLastPosition = false;

    float size = rad * 2.0f * g_config.general.globalScale;
    size = std::max(size, kEffectWindowMinSize); // minimum 40x40

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
        _cgImage = img;

        self.backgroundColor = [NSColor clearColor];
        self.ignoresMouseEvents = YES; // Always click-through
        self.level = NSStatusWindowLevel;
        self.hasShadow = NO;
        self.releasedWhenClosed = NO; // Lifetime managed by EffectWindowManager dictionary
        self.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                  NSWindowCollectionBehaviorIgnoresCycle;

        _contentView = [[EffectContentView alloc] initWithFrame:self.contentView.bounds effectType:type];
        _contentView.posX = x;
        _contentView.posY = y;
        _contentView.radius = rad;
        _contentView.cgImage = img;
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
    size = std::max(size, kEffectWindowMinSize);

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
    const auto& regs = EffectGetRegistrations();

    // Phase 1: Remove windows for effects that no longer exist
    for (size_t i = 0; i < _count; i++) {
        size_t idx = (_head + i) % kMaxEffectWindows;
        id obj = _windows[idx];
        if (![obj isKindOfClass:[EffectWindow class]]) continue;

        EffectWindow* win = (EffectWindow*)obj;
        BOOL effectStillExists = NO;

        // Find the registration for this effect type
        for (const auto& reg : regs) {
            if (reg.type == (int)win.effectType) {
                Vector2 pos = {win.posX, win.posY};
                if (reg.existsAt(pos)) {
                    effectStillExists = YES;
                }
                break;
            }
        }

        if (!effectStillExists) {
            [win closeAndRemove];
            [_windows replaceObjectAtIndex:idx withObject:[NSNull null]];
        }
    }

    // Phase 2: Create windows for new effects
    for (const auto& reg : regs) {
        std::vector<Vector2> positions = reg.getPositions();
        for (const auto& pos : positions) {
            // Check if window already exists for this position
            BOOL exists = NO;
            for (size_t i = 0; i < _count; i++) {
                size_t idx = (_head + i) % kMaxEffectWindows;
                id obj = _windows[idx];
                if (![obj isKindOfClass:[EffectWindow class]]) continue;

                EffectWindow* win = (EffectWindow*)obj;
                if ((int)win.effectType == reg.type &&
                    std::abs(pos.x - win.posX) < 1.0f &&
                    std::abs(pos.y - win.posY) < 1.0f) {
                    exists = YES;
                    break;
                }
            }

            if (!exists) {
                float radius = reg.getRadius(pos);
                EffectWindow* win = [[EffectWindow alloc] initWithType:(EffectType)reg.type
                                                                  posX:pos.x
                                                                  posY:pos.y
                                                                radius:radius cgImage:nil];
                if (reg.configureWindow) {
                    reg.configureWindow(win, pos);
                }
                [self addWindow:win];
            }
        }
    }

    // Phase 3: Update positions and redraw for existing windows
    double now = [[NSDate date] timeIntervalSince1970];
    for (size_t i = 0; i < _count; i++) {
        size_t idx = (_head + i) % kMaxEffectWindows;
        id obj = _windows[idx];
        if (![obj isKindOfClass:[EffectWindow class]]) continue;

        EffectWindow* win = (EffectWindow*)obj;
        [win updatePosition];

        // Find registration for this effect type to configure content view
        for (const auto& reg : regs) {
            if (reg.type == (int)win.effectType) {
                if (reg.configureContentView) {
                    Vector2 pos = {win.posX, win.posY};
                    reg.configureContentView((EffectContentView*)win.contentView, pos);
                }
                // Force redraw for effects that need time-based updates (footprint fading)
                if (reg.type == (int)EffectTypeFootprint) {
                    EffectContentView* cv = (EffectContentView*)win.contentView;
                    cv.currentTime = now;
                    [cv setNeedsDisplay:YES];
                }
                break;
            }
        }
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
