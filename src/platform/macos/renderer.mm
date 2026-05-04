#import "renderer.h"
#import "window.h"
#import <vector>
#import <cmath>
#import <algorithm>

#if defined(__APPLE__)
#define DEBUG_LOG(fmt, ...) do { if (g_debugMode) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)
#endif

@interface GooseView ()
@property (nonatomic, assign) double currentTime;
@property (nonatomic, assign) int tickCount;
@property (nonatomic, strong) dispatch_source_t timer;
@end

@implementation GooseView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.wantsLayer = YES;
        self.layer.backgroundColor = [[NSColor clearColor] CGColor];
        _currentTime = 0.0;
        _tickCount = 0;
    }
    return self;
}

- (void)startAnimation {
    [self stopAnimation];
    self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, 0), NSEC_PER_SEC/60, NSEC_PER_SEC/600);
    __weak GooseView* weakSelf = self;
    dispatch_source_set_event_handler(self.timer, ^{ [weakSelf tick]; });
    dispatch_resume(self.timer);
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
        if (item.type == ITEM_IMAGE && item.data) {
            // Draw dropped image
        } else if (item.type == ITEM_TEXT && item.data) {
            // Draw dropped text note
        }
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

    Vector2 bodyFront = g->rig.body + fwd * 11.0f;
    Vector2 bodyBack  = g->rig.body - fwd * 11.0f;
    Vector2 underFront = g->rig.underbody + fwd * 7.0f;
    Vector2 underBack = g->rig.underbody - fwd * 7.0f;

    [self drawLineCG:ctx from:bodyFront to:bodyBack width:24 r:0.82f g:0.82f b:0.82f];
    [self drawLineCG:ctx from:g->rig.neckBase to:g->rig.neckHead width:15 r:0.82f g:0.82f b:0.82f];
    [self drawLineCG:ctx from:g->rig.neckHead to:g->rig.head1 width:17 r:0.82f g:0.82f b:0.82f];
    [self drawLineCG:ctx from:g->rig.head1 to:g->rig.head2 width:12 r:0.82f g:0.82f b:0.82f];
    [self drawLineCG:ctx from:underFront to:underBack width:15 r:0.82f g:0.82f b:0.82f];

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

- (void)drawLineCG:(CGContextRef)ctx from:(Vector2)a to:(Vector2)b width:(float)w r:(float)r g:(float)g b:(float)b {
    Vector2 diff = b - a;
    float len = diff.Length();
    if (len < 0.001f) return;

    Vector2 perp = Vector2{-diff.y, diff.x}.Normalized() * (w * 0.5f);
    Vector2 p1 = a + perp;
    Vector2 p2 = a - perp;
    Vector2 p3 = b - perp;
    Vector2 p4 = b + perp;

    CGContextSetRGBFillColor(ctx, r, g, b, 1.0f);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, p1.x, p1.y);
    CGContextAddLineToPoint(ctx, p2.x, p2.y);
    CGContextAddLineToPoint(ctx, p3.x, p3.y);
    CGContextAddLineToPoint(ctx, p4.x, p4.y);
    CGContextClosePath(ctx);
    CGContextFillPath(ctx);
}

- (void)drawHeldItem:(void*)item inContext:(CGContextRef)ctx {
    if (!item) return;
    DroppedItem* di = (DroppedItem*)item;
    if (di->type == ITEM_TEXT && di->data) {
        const char* text = (const char*)di->data;
        // Draw text note
    }
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
            g.Honk();
            return;
        }
    }
}

@end