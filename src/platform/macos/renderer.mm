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
#include "cursor_io.h"
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
@property (nonatomic, assign) double currentTime;
@property (nonatomic, assign) int tickCount;
@property (nonatomic, strong) dispatch_source_t timer;
- (void)drawGoose:(Goose*)g inContext:(CGContextRef)ctx;
- (void)drawHeldItem:(Goose*)g inContext:(CGContextRef)ctx;
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
    self.currentTime += g_config.render.frameDt;
    self.tickCount++;

    CursorState cursor = {};
    CursorAction action = {};
    if (g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }

    for (auto& g : g_geese) {
        CursorAction a = g.Update(g_config.render.frameDt, self.currentTime, (int)self.bounds.size.width, (int)self.bounds.size.height, cursor);
        if (!a.isNone()) action = a;
        if (g_debugMode && self.tickCount % g_config.render.debugTickMod == 0) {
            DEBUG_LOG("Goose %d pos: %.1f,%.1f speed: %.1f", g.id, g.pos.x, g.pos.y, g.currentSpeed);
        }
    }

    if (g_cursorProvider && !action.isNone()) {
        g_cursorProvider->Execute(action);
    }

    g_droppedItems.remove_if([&](DroppedItem& i) {
        bool exp = i.isExpired(self.currentTime);
        if (exp) delete i.data;
        return exp;
    });

    g_footprints.remove_if([&](Footprint& fp) {
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        return (self.currentTime - fp.timeSpawned) > life;
    });

    if (self.currentTime > 120.0) {
        [self stopAnimation];
        exit(0);
        return;
    }

    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);

    [self drawDroppedItems:ctx];
    
    // Flip Y for goose and footprints
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 0, self.bounds.size.height);
    CGContextScaleCTM(ctx, 1.0, -1.0);
    [self drawFootprints:ctx];
    [self drawGeese:ctx];
    [self drawDebugOverlay:ctx];
    CGContextRestoreGState(ctx);
}

- (void)drawFootprints:(CGContextRef)ctx {
    for (const auto& fp : g_footprints) {
        float age = (float)(self.currentTime - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        float alpha = std::max(0.0f, 1.0f - (age / life));
        if (alpha <= 0) continue;

        CGContextSetRGBFillColor(ctx, g_config.color.footprint.r, g_config.color.footprint.g, g_config.color.footprint.b, alpha * g_config.color.footprintAlphaMultiplier);
        
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, fp.pos.x, fp.pos.y);
        CGContextRotateCTM(ctx, fp.dir);
        CGContextFillEllipseInRect(ctx, CGRectMake(-g_config.render.footprintWidth/2, -g_config.render.footprintHeight/2, g_config.render.footprintWidth, g_config.render.footprintHeight));
        CGContextRestoreGState(ctx);
    }
}

- (void)drawDroppedItems:(CGContextRef)ctx {
    for (const auto& item : g_droppedItems) {
        float x = item.pos.x - item.data->w / 2.0f;
        float y = item.pos.y - item.data->h / 2.0f;
        
        if (item.data->type == ItemData::TEXT) {
            CGContextSetRGBFillColor(ctx, 1.0, 1.0, 0.8, 1.0);
            CGContextFillRect(ctx, CGRectMake(x, y, item.data->w, item.data->h));
            
            NSString* text = [NSString stringWithUTF8String:item.data->Text().c_str()];
            NSDictionary* attrs = @{
                NSFontAttributeName: [NSFont systemFontOfSize:14],
                NSForegroundColorAttributeName: [NSColor blackColor]
            };
            [text drawInRect:NSMakeRect(x + 5, y + 5, item.data->w - 10, item.data->h - 10) withAttributes:attrs];
        } else if (item.data->type == ItemData::MEME) {
            if (item.data->image) {
                CGContextDrawImage(ctx, CGRectMake(x, y, item.data->w, item.data->h), item.data->image);
            } else {
                CGContextSetRGBFillColor(ctx, 0.9, 0.7, 0.9, 1.0);
                CGContextFillRect(ctx, CGRectMake(x, y, item.data->w, item.data->h));
            }
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
    CGContextScaleCTM(ctx, g_config.general.globalScale, g_config.general.globalScale);
    CGContextTranslateCTM(ctx, -g->pos.x, -g->pos.y);

    Vector2 rawFwd = Vector2::FromAngleDegrees(g->dir);
    Vector2 fwd{ rawFwd.x * g->ISO_SCALE.x, rawFwd.y * g->ISO_SCALE.y };

    float facing = Dot(Vector2::Normalize(fwd), Vector2{0, 1});
    float back = Clamp(-facing, 0.0f, 1.0f);
    bool facingBack = (back > g_config.render.facingBackThreshold);

    if (g->heldItem && facingBack) {
        [self drawHeldItem:g inContext:ctx];
    }

    // Beak color - use Canada beak if enabled
    float beakR = g_config.general.canadaGooseMode ? g_config.color.canadaBeak.r : g_config.color.beak.r;
    float beakG = g_config.general.canadaGooseMode ? g_config.color.canadaBeak.g : g_config.color.beak.g;
    float beakB = g_config.general.canadaGooseMode ? g_config.color.canadaBeak.b : g_config.color.beak.b;

    // Eye color - use Canada eye if enabled
    float eyeR = g_config.general.canadaGooseMode ? g_config.color.canadaEye.r : g_config.color.eye.r;
    float eyeG = g_config.general.canadaGooseMode ? g_config.color.canadaEye.g : g_config.color.eye.g;
    float eyeB = g_config.general.canadaGooseMode ? g_config.color.canadaEye.b : g_config.color.eye.b;

    // shadow
    DrawEllipse(ctx, g->pos + Vector2{g_config.render.shadowOffsetX, g_config.render.shadowOffsetY},
                g_config.render.shadowWidth / 2, g_config.render.shadowHeight / 2,
                g_config.color.shadow.r, g_config.color.shadow.g, g_config.color.shadow.b, 0.3f);

    // feet
    DrawEllipse(ctx, g->rig.lFoot.currentPos, g_config.render.footSize / 2, g_config.render.footSize / 2,
                beakR, beakG, beakB, 1.0f);
    DrawEllipse(ctx, g->rig.rFoot.currentPos, g_config.render.footSize / 2, g_config.render.footSize / 2,
                beakR, beakG, beakB, 1.0f);

    // body segments - compute front/back points along fwd axis
    Vector2 bodyFront = g->rig.body + fwd * 11.0f;
    Vector2 bodyBack  = g->rig.body - fwd * 11.0f;
    Vector2 underFront = g->rig.underbody + fwd * 7.0f;
    Vector2 underBack  = g->rig.underbody - fwd * 7.0f;

    // Colors - use Canada Goose colors if enabled
    float bodyR, bodyG, bodyB;
    float neckR, neckG, neckB;
    float headR, headG, headB;
    float outlineR, outlineG, outlineB;
    if (g_config.general.canadaGooseMode) {
        // Canada Goose: black head/neck, brownish body, tan breast
        headR = g_config.color.canadaHead.r; headG = g_config.color.canadaHead.g; headB = g_config.color.canadaHead.b; // Black head
        neckR = g_config.color.canadaNeck.r; neckG = g_config.color.canadaNeck.g; neckB = g_config.color.canadaNeck.b; // Solid black neck
        bodyR = g_config.color.canadaBody.r; bodyG = g_config.color.canadaBody.g; bodyB = g_config.color.canadaBody.b; // Brownish-grey body
        outlineR = g_config.color.canadaOutline.r; outlineG = g_config.color.canadaOutline.g; outlineB = g_config.color.canadaOutline.b; // Dark outline
    } else {
        // Default: white/light gray
        bodyR = g_config.color.goose.r;
        bodyG = g_config.color.goose.g;
        bodyB = g_config.color.goose.b;
        neckR = bodyR; neckG = bodyG; neckB = bodyB;
        headR = bodyR; headG = bodyG; headB = bodyB;
        outlineR = 0.82f; outlineG = 0.82f; outlineB = 0.82f;
    }

    // outlines
    DrawLine(ctx, bodyFront, bodyBack, 24, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, 15, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, 17, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, 12, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, underFront, underBack, 15, outlineR, outlineG, outlineB, 1.0f);

    // body squash when facing away
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, g->rig.body.x, g->rig.body.y);
    float squash = Lerp(1.0f, 0.92f, back);
    CGContextScaleCTM(ctx, 1.0f, squash);
    CGContextTranslateCTM(ctx, -g->rig.body.x, -g->rig.body.y);

    // fill - body, neck, head1, head2
    DrawLine(ctx, bodyFront, bodyBack, 22, bodyR, bodyG, bodyB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, 13, neckR, neckG, neckB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, 15, headR, headG, headB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, 10, headR, headG, headB, 1.0f);

    // beak
    float beakW = std::min(g_config.render.beakWidth, g_config.render.beakMaxWidth);
    Vector2 beakBase = g->rig.neckHead + fwd * g_config.rig.beakBaseOffset;
    Vector2 beakTip = beakBase + fwd * g_config.rig.beakLen;
    DrawLine(ctx, beakBase, beakTip, beakW, beakR, beakG, beakB, 1.0f);

    CGContextRestoreGState(ctx);

    // eyes
    Vector2 rawSide = Vector2::FromAngleDegrees(g->dir + 90.0f);
    Vector2 side{ rawSide.x * g->ISO_SCALE.x, rawSide.y * g->ISO_SCALE.y };
    Vector2 up{ 0, -1 };

    float eyeSep = Lerp(5.0f, 2.8f, back);
    float eyeLift = Lerp(0.0f, 1.5f, back);
    Vector2 eyeCenter = g->rig.neckHead + up * (3.0f + eyeLift);

    if (back > 0.82f) {
        DrawEllipse(ctx, eyeCenter, 2, 2, eyeR, eyeG, eyeB, 1.0f);
    } else {
        DrawEllipse(ctx, eyeCenter - side * eyeSep, 2, 2, eyeR, eyeG, eyeB, 1.0f);
        DrawEllipse(ctx, eyeCenter + side * eyeSep, 2, 2, eyeR, eyeG, eyeB, 1.0f);
    }

    if (g->heldItem && !facingBack) {
        [self drawHeldItem:g inContext:ctx];
    }

    CGContextRestoreGState(ctx);
}

- (void)drawHeldItem:(Goose*)g inContext:(CGContextRef)ctx {
    if (!g->heldItem) return;
    CGContextSaveGState(ctx);

    CGContextTranslateCTM(ctx, g->dragPos.x, g->dragPos.y);
    float dragRad = g->dragRot;
    CGContextRotateCTM(ctx, -dragRad);
    CGContextTranslateCTM(ctx, -g->heldItem->w / 2, 0);

    if (g->heldItem->type == ItemData::MEME && g->heldItem->image) {
        CGContextDrawImage(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h), g->heldItem->image);
    } else if (g->heldItem->type == ItemData::MEME) {
        CGContextSetRGBFillColor(ctx, 0.8f, 0.8f, 0.8f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h));
    } else if (g->heldItem->type == ItemData::TEXT) {
        CGContextSetRGBFillColor(ctx, 1, 1, 0.9f, 1.0f);
        CGContextFillRect(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h));
        CGContextSetRGBStrokeColor(ctx, 0, 0, 0, 1.0f);
        CGContextSetLineWidth(ctx, 2);
        CGContextStrokeRect(ctx, CGRectMake(0, 0, g->heldItem->w, g->heldItem->h));

        if (g->heldItem->textContent) {
            NSString* text = [NSString stringWithUTF8String:g->heldItem->textContent->c_str()];
            NSDictionary* attrs = @{NSFontAttributeName: [NSFont systemFontOfSize:10.0],
                                    NSForegroundColorAttributeName: [NSColor blackColor]};
            NSRect textRect = NSMakeRect(5, 5, g->heldItem->w - 10, g->heldItem->h - 10);
            [text drawInRect:textRect withAttributes:attrs];
        }
    }

    CGContextRestoreGState(ctx);
}

- (void)drawDebugOverlay:(CGContextRef)ctx {
    if (!g_config.debug.visuals) return;

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
        if (std::sqrt(dx*dx + dy*dy) < g_config.render.clickRadius) {
            return;
        }
    }
}

@end
