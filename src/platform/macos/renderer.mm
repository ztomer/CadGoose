#import "renderer.h"
#import "window.h"
#import <vector>
#import <cmath>
#import <algorithm>
#import <time.h>

#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "cursor_backend.h"
#include "items.h"

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

namespace {
    // Goose rendering constants
    constexpr float SHADOW_OFFSET_X = 2.0f;
    constexpr float SHADOW_OFFSET_Y = 10.0f;
    constexpr float SHADOW_WIDTH = 40.0f;
    constexpr float SHADOW_HEIGHT = 30.0f;
    constexpr float FOOT_SIZE = 8.0f;
    constexpr float BODY_WIDTH = 30.0f;
    constexpr float BODY_HEIGHT = 20.0f;
    constexpr float NECK_SIZE = 16.0f;
    constexpr float HEAD1_SIZE = 20.0f;
    constexpr float HEAD2_SIZE = 14.0f;
    constexpr float BEAK_WIDTH = 16.0f;
    constexpr float BEAK_HEIGHT = 8.0f;
    constexpr float EYE_SIZE = 4.0f;
    constexpr float CLICK_RADIUS = 30.0f;
}

@interface GooseView ()
@property (nonatomic, assign) double currentTime;
@property (nonatomic, assign) int tickCount;
@property (nonatomic, strong) dispatch_source_t timer;
- (void)drawGoose:(Goose*)g inContext:(CGContextRef)ctx;
- (void)drawHeldItem:(void*)item inContext:(CGContextRef)ctx;
- (void)drawFootprints:(CGContextRef)ctx;
- (void)drawDroppedItems:(CGContextRef)ctx;
- (void)drawGeese:(CGContextRef)ctx;
- (void)drawDebugOverlay:(CGContextRef)ctx;
@end

@implementation GooseView

- (instancetype)initWithFrame:(NSRect)frameRect {
    DEBUG_LOG("GooseView initWithFrame START, frame=%s", NSStringFromRect(frameRect).UTF8String);
    
    self = [super initWithFrame:frameRect];
    DEBUG_LOG("  super init done, self=%p", self);
    
    if (self) {
        self.wantsLayer = YES;
        DEBUG_LOG("  wantsLayer=YES");
        
        self.layer.backgroundColor = [[NSColor clearColor] CGColor];
        DEBUG_LOG("  layer backgroundColor set");
        
        _currentTime = 0.0;
        _tickCount = 0;
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
    
    self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    DEBUG_LOG("  timer created: %p", self.timer);
    
    dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, 0), NSEC_PER_SEC/60, NSEC_PER_SEC/600);
    DEBUG_LOG("  timer scheduled");
    
    __weak GooseView* weakSelf = self;
    dispatch_source_set_event_handler(self.timer, ^{ [weakSelf tick]; });
    DEBUG_LOG("  event handler set");
    
    dispatch_resume(self.timer);
    DEBUG_LOG("  timer resumed");
    DEBUG_LOG("GooseView startAnimation END");
}

- (void)stopAnimation {
    if (self.timer) {
        dispatch_source_cancel(self.timer);
        self.timer = nil;
    }
}

- (void)tick {
    self.currentTime += 1.0 / 60.0;
    self.tickCount++;

    for (auto& g : g_geese) {
        g.Update(1.0 / 60.0, self.currentTime, (int)self.bounds.size.width, (int)self.bounds.size.height);
    }

    g_droppedItems.remove_if([&](DroppedItem& i) {
        bool exp = i.isExpired(self.currentTime);
        if (exp) delete i.data;
        return exp;
    });

    g_footprints.remove_if([&](Footprint& fp) {
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mudLifetime;
        return (self.currentTime - fp.timeSpawned) > life;
    });

    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 0, self.bounds.size.height);
    CGContextScaleCTM(ctx, 1.0, -1.0);

    [self drawFootprints:ctx];
    [self drawDroppedItems:ctx];
    [self drawGeese:ctx];
    [self drawDebugOverlay:ctx];

    CGContextRestoreGState(ctx);
}

- (void)drawFootprints:(CGContextRef)ctx {
    for (const auto& fp : g_footprints) {
        float age = (float)(self.currentTime - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mudLifetime;
        float alpha = std::max(0.0f, 1.0f - (age / life));
        if (alpha <= 0) continue;

        CGContextSetRGBFillColor(ctx, 0.4, 0.25, 0.1, alpha * 0.6);
        CGContextFillEllipseInRect(ctx, CGRectMake(fp.pos.x - 6, fp.pos.y - 4, 12, 8));
    }
}

- (void)drawDroppedItems:(CGContextRef)ctx {
    for (const auto& item : g_droppedItems) {
        // Draw placeholder for dropped items
        CGContextSetRGBFillColor(ctx, 0.5, 0.5, 0.5, 0.5);
        CGContextFillEllipseInRect(ctx, CGRectMake(item.pos.x - 10, item.pos.y - 10, 20, 20));
    }
}

- (void)drawGeese:(CGContextRef)ctx {
    for (auto& g : g_geese) {
        [self drawGoose:&g inContext:ctx];
    }
}

- (void)drawGoose:(Goose*)g inContext:(CGContextRef)ctx {
    if (!std::isfinite(g->pos.x) || !std::isfinite(g->pos.y)) return;

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, g->pos.x, g->pos.y);
    CGContextScaleCTM(ctx, g_config.globalScale, g_config.globalScale);
    CGContextTranslateCTM(ctx, -g->pos.x, -g->pos.y);

    Vector2 rawFwd = Vector2::FromAngleDegrees(g->dir);
    float isoScaleX = 1.0f;
    float isoScaleY = 0.7f;
    Vector2 fwd{ rawFwd.x * isoScaleX, rawFwd.y * isoScaleY };

    float facing = Dot(Vector2::Normalize(fwd), Vector2{0, 1});
    float back = std::max(0.0f, std::min(1.0f, -facing));
    bool facingBack = (back > 0.55f);

    if (g->heldItem && facingBack) {
        [self drawHeldItem:g->heldItem inContext:ctx];
    }

    CGContextSetRGBFillColor(ctx, 0, 0, 0, 0.3f);
    CGContextFillEllipseInRect(ctx, CGRectMake(g->pos.x + 2 - 20, g->pos.y + 10 - 15, 40, 30));

    CGContextSetRGBFillColor(ctx, 1, 0.64, 0, 1);
    CGContextFillEllipseInRect(ctx, CGRectMake(g->rig.lFoot.currentPos.x - 4, g->rig.lFoot.currentPos.y - 4, 8, 8));
    CGContextFillEllipseInRect(ctx, CGRectMake(g->rig.rFoot.currentPos.x - 4, g->rig.rFoot.currentPos.y - 4, 8, 8));

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, g->rig.body.x, g->rig.body.y);
    float squash = 1.0f + (back * (1.0f - 0.92f));
    CGContextScaleCTM(ctx, 1.0f, squash);
    CGContextTranslateCTM(ctx, -g->rig.body.x, -g->rig.body.y);

    // Draw body as circles
    CGContextSetRGBFillColor(ctx, 0.82f, 0.82f, 0.82f, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(g->rig.body.x - 15, g->rig.body.y - 10, 30, 20));
    CGContextFillEllipseInRect(ctx, CGRectMake(g->rig.neckBase.x - 8, g->rig.neckBase.y - 8, 16, 16));
    CGContextFillEllipseInRect(ctx, CGRectMake(g->rig.head1.x - 10, g->rig.head1.y - 10, 20, 20));
    CGContextFillEllipseInRect(ctx, CGRectMake(g->rig.head2.x - 7, g->rig.head2.y - 7, 14, 14));

    float beakW = std::min(16.0f, 9.0f);
    float beakH = 8.0f;
    Vector2 beakBase = g->rig.head2;
    Vector2 beakTip = beakBase + fwd * beakW;

    CGContextSetRGBFillColor(ctx, 1.0f, 0.6f, 0.0f, 1.0f);
    CGRect beakRect = CGRectMake(beakTip.x - beakW * 0.5f, beakTip.y - beakH * 0.5f, beakW, beakH);
    CGContextFillEllipseInRect(ctx, beakRect);

    float eyeOffset = back > 0.5f ? -3.0f : 3.0f;
    Vector2 eyePos = g->rig.head2 + Vector2{eyeOffset, -4.0f};
    CGContextSetRGBFillColor(ctx, 0.1f, 0.1f, 0.1f, 1.0f);
    CGContextFillEllipseInRect(ctx, CGRectMake(eyePos.x - 2, eyePos.y - 2, 4, 4));

    CGContextRestoreGState(ctx);
    CGContextRestoreGState(ctx);
}

- (void)drawHeldItem:(void*)item inContext:(CGContextRef)ctx {
    if (!item) return;
    // Draw placeholder for held item
    CGContextSetRGBFillColor(ctx, 1.0, 0.8, 0.0, 0.8);
    CGContextFillEllipseInRect(ctx, CGRectMake(-15, -15, 30, 30));
}

- (void)drawDebugOverlay:(CGContextRef)ctx {
    if (!g_config.debugVisuals) return;

    CGContextSetRGBStrokeColor(ctx, 1, 0, 0, 1);
    CGContextSetLineWidth(ctx, 1);
    for (const auto& g : g_geese) {
        CGContextStrokeRect(ctx, CGRectMake(g.pos.x - 20, g.pos.y - 20, 40, 40));
    }
}

- (void)handleClickAtPoint:(NSPoint)point {
    for (auto& g : g_geese) {
        float dx = point.x - g.pos.x;
        float dy = point.y - g.pos.y;
        if (std::sqrt(dx*dx + dy*dy) < 30.0f) {
            // g.Honk(); // TODO: Add Honk method to Goose class
            return;
        }
    }
}

@end