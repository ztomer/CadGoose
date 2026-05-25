#import "renderer.h"
#import "window.h"
#import "item_window.h"
#import "effect_window.h"
#import "goose_drawing.h"
#import <vector>
#import <cmath>
#import <algorithm>
#import <time.h>
#import <QuartzCore/QuartzCore.h>

#include "goose.h"
#include "random_util.h"
#include "goose_drawing.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "cursor_io.h"
#include "items.h"
#include "behavior.h"
#include "ai_text_meme.h"
#include "world_utils.h"
#include "coordinate_system.h"
#include "actor.h"
#include "cg_renderer.h"

void Honcker_Honk(Goose* goose, double time);
float Rainbow_GetHue(int gooseId);
void Rainbow_SetHue(int gooseId, float hue);

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

// --- Timer and rendering constants ---
static constexpr int kWorldCleanupTickInterval = 60;
static constexpr int kLeafSpawnProbabilityDenominator = 10800;
static constexpr float kDisplayLinkMinFps = 30;
static constexpr float kDisplayLinkMaxFps = 60;
static constexpr float kDisplayLinkDefaultFps = 60;
static constexpr float kSpeedRedrawThreshold = 0.01f;

static void DrawEllipse(CGContextRef ctx, Vector2 p, float rx, float ry, float r, float g, float b, float a) {
    CGContextSetRGBFillColor(ctx, r, g, b, a);
    CGContextFillEllipseInRect(ctx, CGRectMake(p.x - rx, p.y - ry, rx * 2, ry * 2));
}

static void DrawLine(CGContextRef ctx, Vector2 a, Vector2 b, float width, float r, float g, float bl, float al) {
    CGContextSetRGBStrokeColor(ctx, r, g, bl, al);
    CGContextSetLineWidth(ctx, width);
    CGContextSetLineCap(ctx, kCGLineCapRound);
    CGContextMoveToPoint(ctx, a.x, a.y);
    CGContextAddLineToPoint(ctx, b.x, b.y);
    CGContextStrokePath(ctx);
}

@interface GooseView ()
@property (nonatomic, assign) BOOL isPrimary;
@property (nonatomic, assign) double currentTime;
@property (nonatomic, assign) int tickCount;
@property (nonatomic, assign) BOOL needsRedraw;
@property (nonatomic, strong) CADisplayLink* displayLink;
- (void)handleKeyDown:(NSEvent*)event;
- (void)onFrameRefresh:(CADisplayLink*)displayLink;
@end

static BOOL s_hasPrimary = NO;

@implementation GooseView

+ (void)resetPrimaryGuard { s_hasPrimary = NO; }

- (BOOL)isFlipped { return YES; }

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.wantsLayer = YES;
        self.layer.backgroundColor = [[NSColor clearColor] CGColor];
        self.layer.masksToBounds = NO;

        if (!s_hasPrimary) {
            self.isPrimary = YES;
            s_hasPrimary = YES;
        } else {
            self.isPrimary = NO;
        }

        _currentTime = 0.0;
        _tickCount = 0;
        _needsRedraw = NO;
        _displayLink = nil;
    }
    return self;
}

- (void)startAnimation {
    [self stopAnimation];

    self.displayLink = [self displayLinkWithTarget:self selector:@selector(onFrameRefresh:)];
    if (@available(macOS 14.0, *)) {
        self.displayLink.preferredFrameRateRange = CAFrameRateRangeMake(kDisplayLinkMinFps, kDisplayLinkMaxFps, kDisplayLinkDefaultFps);
    }
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];

    [self becomeFirstResponder];
}

- (void)stopAnimation {
    [self.displayLink invalidate];
    self.displayLink = nil;
}

- (void)onFrameRefresh:(CADisplayLink*)displayLink {
    [self tick];
}

- (void)handleKeyDown:(NSEvent*)event {
    unichar key = [[event characters] characterAtIndex:0];
    NSUInteger flags = [event modifierFlags];

    if (key == 'f' || key == 'F') {
        for (auto* g : ActorManager::Instance().getGeese()) {
            Honcker_Honk(g, self.currentTime);
        }
    }
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)event { return YES; }

- (void)keyDown:(NSEvent*)event {
    [self handleKeyDown:event];
}

- (void)handleClickAtPoint:(NSPoint)point {}

- (void)tick {
    double dt = g_config.render.frameDt;
    self.currentTime += dt;
    self.tickCount++;

    g_time = self.currentTime;

    if (self.isPrimary) {
        // Tick all actors (geese, toys, flowers, etc.)
        ActorManager::Instance().tickAll(g_world, dt, self.currentTime);
        ActorManager::Instance().cleanup();

        // Update window positions for geese
        auto geese = ActorManager::Instance().getGeese();
        [[WindowManager shared] updateWindowPositionsForGeese:geese];
    }

    if (self.tickCount % kWorldCleanupTickInterval == 0) {
        World_CleanupExpired(self.currentTime);
        self.needsRedraw = YES;
    }

    if (g_config.behaviors.fun.autumnLeaves) {
        if (rng_util::RandRange(kLeafSpawnProbabilityDenominator) == 0) {
            World_SpawnRandomLeafPile(g_world.screenWidth, g_world.screenHeight, self.currentTime);
        }
        auto geese = ActorManager::Instance().getGeese();
        World_TickLeafPiles(self.currentTime, dt,
                            geese.empty() ? nullptr : geese.front());
    }

    [[EffectWindowManager shared] syncWindows];
    if (ActorManager::Instance().countByType("leafpile") > 0) self.needsRedraw = YES;

    [[ItemWindowManager shared] syncWindows];
    if (!ActorManager::Instance().getDroppedItems().empty()) self.needsRedraw = YES;

    auto geese = ActorManager::Instance().getGeese();
    for (auto* g : geese) {
        if (g->heldItem) {
            self.needsRedraw = YES;
            break;
        }
    }

    if (self.needsRedraw) {
        [self setNeedsDisplay:YES];
        self.needsRedraw = NO;
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);
    CGContextSaveGState(ctx);

    NSScreen* windowScreen = self.window.screen ?: [NSScreen mainScreen];
    float screenH = (float)windowScreen.frame.size.height;
    NSRect winFrame = self.window.frame;
    ScreenPoint screenTopLeft = {(float)winFrame.origin.x, (float)winFrame.origin.y + (float)winFrame.size.height};
    DevicePoint viewOriginDevice = CoordTransform::ScreenToDeviceMacOS(screenTopLeft, screenH);

    CGContextTranslateCTM(ctx, -viewOriginDevice.x, -viewOriginDevice.y);

    CGRenderer renderer(ctx);
    ActorManager::Instance().renderAll(&renderer);

    [[ItemWindowManager shared] showPendingWindows];

    DrawDebugOverlay(ctx, ActorManager::Instance().getGeese());
    CGContextRestoreGState(ctx);
}

@end
