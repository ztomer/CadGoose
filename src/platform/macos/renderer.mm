#import "renderer.h"
#import "window.h"
#import <vector>
#import <cmath>
#import <algorithm>
#import <time.h>

#include "goose.h"
#include "goose_drawing.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "cursor_io.h"
#include "items.h"
#include "behavior.h"
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
- (void)handleKeyDown:(NSEvent*)event;
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

    [self becomeFirstResponder];
    DEBUG_LOG("GooseView startAnimation END");
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

- (BOOL)keyDown:(NSEvent*)event {
    [self handleKeyDown:event];
    return YES;
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

        BehaviorContext ctx;
        ctx.goose = &g;
        ctx.time = self.currentTime;
        ctx.isJailed = false;
        BehaviorRegistry::Instance().TickAll(&g, g_config.render.frameDt, self.currentTime);
    }

    if (g_cursorProvider && !action.isNone()) {
        g_cursorProvider->Execute(action);
    }

    if (self.tickCount % 60 == 0) {
        World_CleanupExpired(self.currentTime);
    }

    if (rand() % 600 == 0) {
        World_SpawnRandomLeafPile(self.bounds.size.width, self.bounds.size.height, self.currentTime);
    }

    World_TickLeafPiles(self.currentTime, g_config.render.frameDt,
                        g_geese.empty() ? nullptr : &g_geese.front());

    bool shouldAcceptMouse = (self.draggedItem != nullptr);
    if (!shouldAcceptMouse && ShouldAcceptMouseEvents()) {
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
            
            float closeX = item.data->w/2.0f - g_config.render.closeButtonSize;
            float closeY = item.data->h/2.0f - g_config.render.closeButtonSize;
            if (lx >= closeX && lx <= item.data->w/2.0f &&
                ly >= closeY && ly <= item.data->h/2.0f) {
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

- (void)setNeedsDisplay:(BOOL)flag {
    [super setNeedsDisplay:flag];
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);

    for (const auto& item : g_droppedItems) {
        DrawDroppedItem(ctx, item, self.bounds.size.height);
    }

    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx, 0, self.bounds.size.height);
    CGContextScaleCTM(ctx, 1.0, -1.0);
    DrawFootprints(ctx, g_footprints, self.currentTime);
    DrawLeaves(ctx, g_leafPiles, self.currentTime);

    for (auto& g : g_geese) {
        DrawGoose(&g, ctx);
    }

    for (auto& g : g_geese) {
        BehaviorRegistry::Instance().RenderAll(&g, ctx);
    }

    DrawDebugOverlay(ctx, g_geese);
    CGContextRestoreGState(ctx);
}

@end
