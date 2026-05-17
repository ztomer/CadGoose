#import "renderer.h"
#import "window.h"
#import "item_window.h"
#import <vector>
#import <cmath>
#import <algorithm>
#import <time.h>
#import <QuartzCore/CADisplayLink.h>

#include "goose.h"
#include "goose_drawing.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "cursor_io.h"
#include "items.h"
#include "behavior.h"
#include "ai_text_meme.h"
#include "world_utils.h"

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
static constexpr int kLeafSpawnProbabilityDenominator = 600;
static constexpr double kIdleTimeout = 3.0; // seconds before entering low-power mode

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
@property (nonatomic, assign) double lastActiveTime;
@property (nonatomic, strong) CADisplayLink* displayLink;
- (void)handleKeyDown:(NSEvent*)event;
- (void)onFrameRefresh:(CADisplayLink*)displayLink;
@end

static BOOL s_hasPrimary = NO;

@implementation GooseView

+ (void)resetPrimaryGuard { s_hasPrimary = NO; }

- (BOOL)isFlipped {
    return YES;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    DEBUG_LOG("GooseView initWithFrame START, frame=%s", NSStringFromRect(frameRect).UTF8String);

    self = [super initWithFrame:frameRect];
    DEBUG_LOG("  super init done, self=%p", self);

    if (self) {
        self.wantsLayer = YES;
        DEBUG_LOG("  wantsLayer=YES");

        self.layer.backgroundColor = [[NSColor clearColor] CGColor];
        DEBUG_LOG("  layer backgroundColor set");

        if (!s_hasPrimary) {
            self.isPrimary = YES;
            s_hasPrimary = YES;
            DEBUG_LOG("  PRIMARY GOOSE VIEW (will update geese)");
        } else {
            self.isPrimary = NO;
            DEBUG_LOG("  SECONDARY GOOSE VIEW (display only)");
        }

        _currentTime = 0.0;
        _tickCount = 0;
        _needsRedraw = NO;
        _lastActiveTime = 0.0;
        _displayLink = nil;
        DEBUG_LOG("  time/count initialized");
    } else {
        LOG("ERROR: GooseView init returned nil!");
    }

    DEBUG_LOG("GooseView initWithFrame END");
    return self;
}

- (void)startAnimation {
    DEBUG_LOG("GooseView startAnimation START");

    [self stopAnimation];
    DEBUG_LOG("  stopped old timer");

    self.displayLink = [self displayLinkWithTarget:self selector:@selector(onFrameRefresh:)];
    if (@available(macOS 14.0, *)) {
        self.displayLink.preferredFrameRateRange = CAFrameRateRangeMake(30, 60, 60);
    }
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    DEBUG_LOG("  CADisplayLink created and added to run loop");

    [self becomeFirstResponder];
    DEBUG_LOG("GooseView startAnimation END");
}

- (void)stopAnimation {
    [self.displayLink invalidate];
    self.displayLink = nil;
}

- (void)onFrameRefresh:(CADisplayLink*)displayLink {
    @autoreleasepool {
        [self tick];
    }
}

- (void)handleKeyDown:(NSEvent*)event {
    unichar key = [[event characters] characterAtIndex:0];
    NSUInteger flags = [event modifierFlags];

    if (key == 'f' || key == 'F') {
        fprintf(stderr, "[HONCKER] F key pressed\n");
        for (auto& g : g_geese) {
            Honcker_Honk(&g, self.currentTime);
        }
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

- (void)keyDown:(NSEvent*)event {
    [self handleKeyDown:event];
}

- (void)handleClickAtPoint:(NSPoint)point {
}

- (void)tick {
    double dt = self.displayLink.duration;
    self.currentTime += dt;
    self.tickCount++;

    bool gooseActive = NO;
    CursorAction action = {};

    // Skip cursor polling when idle (saves CGEvent overhead)
    CursorState cursor = {};
    if (self.isPrimary) {
        for (auto& g : g_geese) {
            if (g.currentSpeed > 0.01f) gooseActive = YES;

            CursorAction a = g.Update(dt, self.currentTime, (int)self.bounds.size.width, (int)self.bounds.size.height, cursor);
            if (!a.isNone()) action = a;
            if (g.currentSpeed > 0.01f) gooseActive = YES;
            if (g_debugMode && self.tickCount % g_config.render.debugTickMod == 0) {
                DEBUG_LOG("Goose %d pos: %.1f,%.1f speed: %.1f", g.id, g.pos.x, g.pos.y, g.currentSpeed);
            }

            BehaviorContext ctx;
            ctx.goose = &g;
            ctx.time = self.currentTime;
            ctx.isJailed = false;
            BehaviorRegistry::Instance().TickAll(&g, dt, self.currentTime);
        }
    }

    // Track idle time for skipping expensive operations
    if (gooseActive || !action.isNone()) {
        self.lastActiveTime = self.currentTime;
    }
    double idleTime = self.currentTime - self.lastActiveTime;
    bool isIdle = (idleTime > kIdleTimeout);

    // Skip cursor polling when idle (expensive CGEvent call)
    if (!isIdle && g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }

    if (!isIdle && g_cursorProvider && !action.isNone()) {
        g_cursorProvider->Execute(action);
        self.needsRedraw = YES;
    }

    if (self.tickCount % kWorldCleanupTickInterval == 0) {
        World_CleanupExpired(self.currentTime);
        self.needsRedraw = YES;
    }

    // Skip leaf pile updates when idle
    if (!isIdle && g_config.behaviors.fun.autumnLeaves) {
        if (rand() % kLeafSpawnProbabilityDenominator == 0) {
            World_SpawnRandomLeafPile(self.bounds.size.width, self.bounds.size.height, self.currentTime);
        }
        World_TickLeafPiles(self.currentTime, dt,
                            g_geese.empty() ? nullptr : &g_geese.front());
        self.needsRedraw = YES;
    }

    // Sync item windows only when items exist
    if (!g_droppedItems.empty()) {
        [[ItemWindowManager shared] syncWindows];
        self.needsRedraw = YES;
    }

    // Only redraw when something actually changed
    if (self.needsRedraw) {
        [self setNeedsDisplay:YES];
        self.needsRedraw = NO;
    }
}

- (void)setNeedsDisplay:(BOOL)flag {
    [super setNeedsDisplay:flag];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);

    CGContextSaveGState(ctx);

    DrawFootprints(ctx, g_footprints, self.currentTime);
    if (g_config.behaviors.fun.autumnLeaves) {
        DrawLeaves(ctx, g_leafPiles, self.currentTime);
    }

    for (auto& g : g_geese) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, g.pos.x, g.pos.y);
        CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
        CGContextTranslateCTM(ctx, -g.pos.x, -g.pos.y);
        BehaviorRegistry::Instance().RenderPass(&g, ctx, true);
        CGContextRestoreGState(ctx);
    }

    for (auto& g : g_geese) {
        DrawGoose(&g, ctx);
        if (g.heldItem) {
            DrawHeldItem(&g, ctx);
        }
    }

    for (auto& g : g_geese) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, g.pos.x, g.pos.y);
        CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
        CGContextTranslateCTM(ctx, -g.pos.x, -g.pos.y);
        BehaviorRegistry::Instance().RenderPass(&g, ctx, false);
        CGContextRestoreGState(ctx);
    }

    DrawDebugOverlay(ctx, g_geese);
    CGContextRestoreGState(ctx);
}

@end
