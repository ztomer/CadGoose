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
@property (nonatomic, strong) dispatch_source_t timer;
@property (nonatomic, assign) DroppedItem* draggedItem;
@property (nonatomic, assign) NSPoint dragOffset;
- (void)handleKeyDown:(NSEvent*)event;
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

- (void)keyDown:(NSEvent*)event {
    [self handleKeyDown:event];
}

- (void)stopAnimation {
    if (self.timer) {
        dispatch_source_cancel(self.timer);
        self.timer = nil;
    }
}

- (void)handleClickAtPoint:(NSPoint)point {
}

- (void)tick {
    @autoreleasepool {
    self.currentTime += g_config.render.frameDt;
    self.tickCount++;

    CursorState cursor = {};
    CursorAction action = {};
    if (g_cursorProvider) {
        cursor = g_cursorProvider->Read();
    }

    if (self.isPrimary) {
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
        }

    AI_TextMeme_Tick(self.currentTime);

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
            float dx = p.x - item.pos.x;
            float dy = p.y - item.pos.y;
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
}

- (NSView *)hitTest:(NSPoint)point {
    NSPoint p = [self convertPoint:point fromView:self.superview];
    float worldX = p.x;
    float worldY = p.y;

    for (auto it = g_droppedItems.rbegin(); it != g_droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
        float dx = worldX - item.pos.x;
        float dy = worldY - item.pos.y;
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
    float worldX = p.x;
    float worldY = p.y;

    for (auto it = g_droppedItems.rbegin(); it != g_droppedItems.rend(); ++it) {
        DroppedItem& item = *it;
        float dx = worldX - item.pos.x;
        float dy = worldY - item.pos.y;
        float cosA = cos(item.rotation);
        float sinA = sin(item.rotation);
        float lx = dx * cosA - dy * sinA;
        float ly = dx * sinA + dy * cosA;

        if (lx >= -item.data->w/2.0f && lx <= item.data->w/2.0f &&
            ly >= -item.data->h/2.0f && ly <= item.data->h/2.0f) {

            float closeX = -item.data->w/2.0f;
            float closeY = -item.data->h/2.0f;
            if (lx >= closeX && lx <= closeX + g_config.render.closeButtonSize &&
                ly >= closeY && ly <= closeY + g_config.render.closeButtonSize) {
                delete item.data;
                auto forward_it = std::prev(it.base());
                g_droppedItems.erase(forward_it);
                [self setNeedsDisplay:YES];
                return;
            }

            self.draggedItem = &item;
            self.draggedItem->pinned = true;
            self.dragOffset = NSMakePoint(item.pos.x - worldX, item.pos.y - worldY);
            auto forward_it = std::prev(it.base());
            g_droppedItems.splice(g_droppedItems.end(), g_droppedItems, forward_it);
            return;
        }
    }
}

- (void)mouseDragged:(NSEvent *)event {
    if (self.draggedItem) {
        NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
        float worldX = p.x;
        float worldY = p.y;

        self.draggedItem->pos.x = worldX + self.dragOffset.x;
        self.draggedItem->pos.y = worldY + self.dragOffset.y;
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

    CGContextSaveGState(ctx);

    for (const auto& item : g_droppedItems) {
        DrawDroppedItem(ctx, item, self.bounds.size.height);
    }

    DrawFootprints(ctx, g_footprints, self.currentTime);
    DrawLeaves(ctx, g_leafPiles, self.currentTime);

    DEBUG_LOG("drawRect: g_geese.size=%zu g_droppedItems.size=%zu", g_geese.size(), g_droppedItems.size());
    for (auto& g : g_geese) {
        DEBUG_LOG("  goose[%d]: heldItem=%p state=%d dragPos=(%.1f,%.1f) dragInit=%d",
                   g.id, (void*)g.heldItem, (int)g.state, g.dragPos.x, g.dragPos.y, g.dragInit);
        DrawGoose(&g, ctx);
        if (g.heldItem) {
            DEBUG_LOG("    heldItem: type=%d w=%d h=%d image=%p",
                      (int)g.heldItem->type, g.heldItem->w, g.heldItem->h,
                      (void*)g.heldItem->image);
        }
        DrawHeldItem(&g, ctx);
    }

    for (auto& g : g_geese) {
        BehaviorRegistry::Instance().RenderAll(&g, ctx);
    }

    DrawDebugOverlay(ctx, g_geese);
    CGContextRestoreGState(ctx);
}

@end
