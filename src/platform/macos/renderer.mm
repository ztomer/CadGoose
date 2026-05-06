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
@property (nonatomic, assign) DroppedItem* draggedItem;
@property (nonatomic, assign) NSPoint dragOffset;
- (void)drawGoose:(Goose*)g inContext:(CGContextRef)ctx;
- (void)drawHeldItem:(Goose*)g inContext:(CGContextRef)ctx;
- (void)drawFootprints:(CGContextRef)ctx;
- (void)drawLeaves:(CGContextRef)ctx;
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

    if (self.tickCount % 60 == 0) {
        g_droppedItems.remove_if([&](DroppedItem& i) {
            bool exp = i.isExpired(self.currentTime);
            if (exp) delete i.data;
            return exp;
        });

        g_footprints.remove_if([&](Footprint& fp) {
            float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
            return (self.currentTime - fp.timeSpawned) > life;
        });
        
        g_leafPiles.remove_if([&](const LeafPile& p) {
            return (p.timeSinceKicked > 0.0f && self.currentTime - p.timeSinceKicked > 10.0f);
        });
    }

    if (rand() % 600 == 0 && g_leafPiles.size() < 10) {
        LeafPile pile;
        pile.Init(Vector2{(float)(rand() % (int)std::max(1.0, self.bounds.size.width)), (float)(rand() % (int)std::max(1.0, self.bounds.size.height))}, 50.0f, 100.0f, self.currentTime);
        g_leafPiles.push_back(pile);
    }
    
    for (auto& pile : g_leafPiles) {
        pile.Tick(g_geese.empty() ? nullptr : &g_geese.front(), self.currentTime, g_config.render.frameDt);
    }
    
    bool shouldAcceptMouse = (self.draggedItem != nullptr);
    if (!shouldAcceptMouse && !g_droppedItems.empty()) {
        NSPoint mouseLoc = [NSEvent mouseLocation];
        NSRect windowRect = [self.window convertRectFromScreen:NSMakeRect(mouseLoc.x, mouseLoc.y, 0, 0)];
        NSPoint p = [self convertPoint:windowRect.origin fromView:nil];
        
        for (auto it = g_droppedItems.rbegin(); it != g_droppedItems.rend(); ++it) {
            DroppedItem& item = *it;
            float cx = item.pos.x;
            float cy = self.bounds.size.height - item.pos.y;
            float dx = p.x - cx;
            float dy = p.y - cy;
            float cosA = cos(item.rotation);
            float sinA = sin(item.rotation);
            float lx = dx * cosA - dy * sinA;
            float ly = dx * sinA + dy * cosA;
            if (lx >= -item.data->w/2.0f && lx <= item.data->w/2.0f &&
                ly >= -item.data->h/2.0f && ly <= item.data->h/2.0f) {
                shouldAcceptMouse = true;
                break;
            }
        }
    }
    if (self.window.ignoresMouseEvents == shouldAcceptMouse) {
        self.window.ignoresMouseEvents = !shouldAcceptMouse;
    }

    [self setNeedsDisplay:YES];
}

- (NSView *)hitTest:(NSPoint)point {
    NSPoint p = [self convertPoint:point fromView:self.superview];
    for (auto it = g_droppedItems.rbegin(); it != g_droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
        float cx = item.pos.x;
        float cy = self.bounds.size.height - item.pos.y;
        float dx = p.x - cx;
        float dy = p.y - cy;
        float cosA = cos(item.rotation);
        float sinA = sin(item.rotation);
        float lx = dx * cosA - dy * sinA;
        float ly = dx * sinA + dy * cosA;
        if (lx >= -item.data->w/2.0f && lx <= item.data->w/2.0f &&
            ly >= -item.data->h/2.0f && ly <= item.data->h/2.0f) {
            return self;
        }
    }
    return nil;
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    for (auto it = g_droppedItems.rbegin(); it != g_droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
        float cx = item.pos.x;
        float cy = self.bounds.size.height - item.pos.y;
        float dx = p.x - cx;
        float dy = p.y - cy;
        float cosA = cos(item.rotation);
        float sinA = sin(item.rotation);
        float lx = dx * cosA - dy * sinA;
        float ly = dx * sinA + dy * cosA;
        
        if (lx >= -item.data->w/2.0f && lx <= item.data->w/2.0f &&
            ly >= -item.data->h/2.0f && ly <= item.data->h/2.0f) {
            
            if (lx >= item.data->w/2.0f - 20 && lx <= item.data->w/2.0f &&
                ly >= item.data->h/2.0f - 20 && ly <= item.data->h/2.0f) {
                delete item.data;
                auto forward_it = std::prev(it.base());
                g_droppedItems.erase(forward_it);
                [self setNeedsDisplay:YES];
                return;
            }
            
            self.draggedItem = &item;
            self.draggedItem->pinned = true;
            self.dragOffset = NSMakePoint(cx - p.x, cy - p.y);
            auto forward_it = std::prev(it.base());
            g_droppedItems.splice(g_droppedItems.end(), g_droppedItems, forward_it);
            return;
        }
    }
}

- (void)mouseDragged:(NSEvent *)event {
    if (self.draggedItem) {
        NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
        float newCx = p.x + self.dragOffset.x;
        float newCy = p.y + self.dragOffset.y;
        self.draggedItem->pos.x = newCx;
        self.draggedItem->pos.y = self.bounds.size.height - newCy;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseUp:(NSEvent *)event {
    if (self.draggedItem) {
        self.draggedItem = nullptr;
    }
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
    [self drawLeaves:ctx];
    [self drawGeese:ctx];
    [self drawDebugOverlay:ctx];
    CGContextRestoreGState(ctx);
}

- (void)drawLeaves:(CGContextRef)ctx {
    struct ColorRGB { float r, g, b; };
    ColorRGB colors[4] = {
        {208/255.0f, 122/255.0f, 45/255.0f},
        {234/255.0f, 198/255.0f, 54/255.0f},
        {172/255.0f, 193/255.0f, 79/255.0f},
        {208/255.0f, 87/255.0f,  64/255.0f}
    };
    
    for (const auto& pile : g_leafPiles) {
        float tKicked = pile.timeSinceKicked;
        float alpha = 1.0f;
        if (tKicked > 0.0f) {
            float age = self.currentTime - tKicked;
            if (age > 8.0f) {
                alpha = std::max(0.0f, 1.0f - (age - 8.0f) / 2.0f);
            }
        }
        if (alpha <= 0.0f) continue;
        
        for (int i = 0; i < pile.leaves.size(); i++) {
            const Leaf& leaf = pile.leaves[i];
            Vector2 p = leaf.GetScreenOffset(1.0f) + pile.pos;
            float sz = 5.0f + 5.0f * (leaf.curPosZ / 900.0f);
            sz *= 2.0f;
            ColorRGB c = colors[leaf.colorIndex % 4];
            CGContextSetRGBFillColor(ctx, c.r, c.g, c.b, alpha);
            CGContextFillEllipseInRect(ctx, CGRectMake(p.x - sz/2.0f, p.y - (sz*0.6f)/2.0f, sz, sz*0.6f));
        }
    }
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
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, item.pos.x, self.bounds.size.height - item.pos.y);
        CGContextRotateCTM(ctx, -item.rotation);

        float x = -item.data->w / 2.0f;
        float y = -item.data->h / 2.0f;

        if (item.data->type == ItemData::TEXT) {
            CGContextSetRGBFillColor(ctx, 1.0, 1.0, 0.8, 1.0);
            CGContextFillRect(ctx, CGRectMake(x, y, item.data->w, item.data->h));

            static NSDictionary* textAttrs = nil;
            if (!textAttrs) {
                textAttrs = @{
                    NSFontAttributeName: [NSFont systemFontOfSize:14],
                    NSForegroundColorAttributeName: [NSColor blackColor]
                };
            }

            NSString* text = [NSString stringWithUTF8String:item.data->Text().c_str()];
            [text drawInRect:NSMakeRect(x + 5, y + 5, item.data->w - 10, item.data->h - 10) withAttributes:textAttrs];
        } else if (item.data->type == ItemData::MEME) {
            if (item.data->image) {
                CGContextDrawImage(ctx, CGRectMake(x, y, item.data->w, item.data->h), item.data->image);
            } else {
                CGContextSetRGBFillColor(ctx, 0.9, 0.7, 0.9, 1.0);
                CGContextFillRect(ctx, CGRectMake(x, y, item.data->w, item.data->h));
            }
        }

        // Draw close button 'X' inside a square
        CGContextSetRGBFillColor(ctx, 0.9, 0.1, 0.1, 0.8);
        CGContextFillRect(ctx, CGRectMake(item.data->w / 2.0f - 20, item.data->h / 2.0f - 20, 20, 20));
        CGContextSetRGBStrokeColor(ctx, 1.0, 1.0, 1.0, 1.0);
        CGContextSetLineWidth(ctx, 2.0);
        CGContextMoveToPoint(ctx, item.data->w / 2.0f - 16, item.data->h / 2.0f - 16);
        CGContextAddLineToPoint(ctx, item.data->w / 2.0f - 4, item.data->h / 2.0f - 4);
        CGContextMoveToPoint(ctx, item.data->w / 2.0f - 4, item.data->h / 2.0f - 16);
        CGContextAddLineToPoint(ctx, item.data->w / 2.0f - 16, item.data->h / 2.0f - 4);
        CGContextStrokePath(ctx);

        CGContextRestoreGState(ctx);
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
    Vector2 bodyFront = g->rig.body + fwd * (g_config.render.bodyHeight / 2.0f);
    Vector2 bodyBack  = g->rig.body - fwd * (g_config.render.bodyHeight / 2.0f);
    Vector2 underFront = g->rig.underbody + fwd * (g_config.render.bodyHeight * 0.3f);
    Vector2 underBack  = g->rig.underbody - fwd * (g_config.render.bodyHeight * 0.3f);

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
    DrawLine(ctx, bodyFront, bodyBack, g_config.render.bodyWidth + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, g_config.render.neckSize + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, g_config.render.head1Size + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, g_config.render.head2Size + 2.0f, outlineR, outlineG, outlineB, 1.0f);
    DrawLine(ctx, underFront, underBack, g_config.render.bodyWidth - 7.0f, outlineR, outlineG, outlineB, 1.0f);

    // body squash when facing away
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, g->rig.body.x, g->rig.body.y);
    float squash = Lerp(1.0f, g_config.render.squashFactor, back);
    CGContextScaleCTM(ctx, 1.0f, squash);
    CGContextTranslateCTM(ctx, -g->rig.body.x, -g->rig.body.y);

    // fill - body, neck, head1, head2
    DrawLine(ctx, bodyFront, bodyBack, g_config.render.bodyWidth, bodyR, bodyG, bodyB, 1.0f);
    DrawLine(ctx, g->rig.neckBase, g->rig.neckHead, g_config.render.neckSize, neckR, neckG, neckB, 1.0f);
    DrawLine(ctx, g->rig.neckHead, g->rig.head1, g_config.render.head1Size, headR, headG, headB, 1.0f);
    DrawLine(ctx, g->rig.head1, g->rig.head2, g_config.render.head2Size, headR, headG, headB, 1.0f);

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

    float eyeSep = Lerp(5.0f, g_config.render.eyeOffsetXFront, back);
    float eyeLift = Lerp(0.0f, 1.5f, back);
    Vector2 eyeCenter = g->rig.neckHead + up * (-g_config.render.eyeOffsetY + eyeLift);

    if (back > g_config.render.eyeFacingThreshold) {
        DrawEllipse(ctx, eyeCenter, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
    } else {
        DrawEllipse(ctx, eyeCenter - side * eyeSep, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
        DrawEllipse(ctx, eyeCenter + side * eyeSep, g_config.render.eyeSize / 2.0f, g_config.render.eyeSize / 2.0f, eyeR, eyeG, eyeB, 1.0f);
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
