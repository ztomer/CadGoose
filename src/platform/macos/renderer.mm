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
#include "item_drag_controller.h"

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
static constexpr int kTargetFPS = 60;
static constexpr int kTimerJitterToleranceHz = 600;
static constexpr int kWorldCleanupTickInterval = 60;
static constexpr int kLeafSpawnProbabilityDenominator = 600;

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
@property (nonatomic, assign) NSPoint lastMouseLoc;
@property (nonatomic, assign) int lastItemCount;
@property (nonatomic, assign) ItemDragController* dragController;
- (void)handleKeyDown:(NSEvent*)event;
- (void)mouseDownAtPoint:(NSPoint)pt viewY:(float)viewY;
- (void)mouseDraggedAtPoint:(NSPoint)pt viewY:(float)viewY;
- (void)mouseUpAtPoint:(NSPoint)pt;
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

            __weak GooseView* weakView = self;
            [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown handler:^NSEvent*(NSEvent* event) {
                GooseView* view = weakView;
                if (!view || !view.window) return event;
                // SCREEN coords (top-left origin, Y-down)
                NSPoint screenPt = [NSEvent mouseLocation];
                NSRect frame = view.window.frame;
                // WINDOW coords (relative to window origin)
                NSPoint windowPt = NSMakePoint(screenPt.x - frame.origin.x, screenPt.y - frame.origin.y);
                // VIEW coords (isFlipped=YES, so top-left origin, Y-down)
                NSPoint viewPt = [view convertPoint:windowPt fromView:nil];
                // DEVICE coords (viewY is Y after flip, same as device Y)
                float viewY = view.bounds.size.height - viewPt.y;
                [view mouseDownAtPoint:viewPt viewY:viewY];
                return event;
            }];
            [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDragged handler:^NSEvent*(NSEvent* event) {
                GooseView* view = weakView;
                if (!view || !view.window) return event;
                // SCREEN → WINDOW → VIEW → DEVICE (same transform as mouseDown)
                NSPoint screenPt = [NSEvent mouseLocation];
                NSRect frame = view.window.frame;
                NSPoint windowPt = NSMakePoint(screenPt.x - frame.origin.x, screenPt.y - frame.origin.y);
                NSPoint viewPt = [view convertPoint:windowPt fromView:nil];
                float viewY = view.bounds.size.height - viewPt.y;
                [view mouseDraggedAtPoint:viewPt viewY:viewY];
                return event;
            }];
            [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseUp handler:^NSEvent*(NSEvent* event) {
                GooseView* view = weakView;
                if (!view) return event;
                [view mouseUpAtPoint:NSZeroPoint];
                return event;
            }];
            DEBUG_LOG("  event monitors installed");
        } else {
            self.isPrimary = NO;
            DEBUG_LOG("  SECONDARY GOOSE VIEW (display only)");
        }

        _currentTime = 0.0;
        _tickCount = 0;
        _dragController = new ItemDragController();
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

    dispatch_source_set_timer(self.timer, dispatch_time(DISPATCH_TIME_NOW, 0), NSEC_PER_SEC/kTargetFPS, NSEC_PER_SEC/kTimerJitterToleranceHz);
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

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
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

- (void)dealloc {
    delete self.dragController;
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

    if (self.tickCount % kWorldCleanupTickInterval == 0) {
        World_CleanupExpired(self.currentTime);
    }

    if (rand() % kLeafSpawnProbabilityDenominator == 0 && g_config.behaviors.fun.autumnLeaves) {
        World_SpawnRandomLeafPile(self.bounds.size.width, self.bounds.size.height, self.currentTime);
    }

    if (g_config.behaviors.fun.autumnLeaves) {
        World_TickLeafPiles(self.currentTime, g_config.render.frameDt,
                            g_geese.empty() ? nullptr : &g_geese.front());
    }

    bool shouldAcceptMouse = (self.dragController->GetDraggedItem() != nullptr);

    if (self.window.ignoresMouseEvents != !shouldAcceptMouse) {
        self.window.ignoresMouseEvents = !shouldAcceptMouse;
    }

    [self setNeedsDisplay:YES];
    }
}

// ============================================================
// Hit Testing & Mouse Events
// ============================================================
// Coordinate spaces:
//   - NSPoint from isFlipped=YES view: VIEW coords (top-left origin, Y-down)
//   - item.pos: DEVICE coords (top-left origin, Y-down, same as VIEW for isFlipped)
//   - No Y-flip needed: view coords == device coords for isFlipped=YES views

- (NSView *)hitTest:(NSPoint)point {
    DevicePoint mousePt = {(float)point.x, (float)point.y};
    return self.dragController->OnMouseDown(mousePt) ? self : nil;
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    DevicePoint mousePt = {(float)p.x, (float)p.y};
    self.dragController->OnMouseDown(mousePt);
}

- (void)mouseDragged:(NSEvent *)event {
    if (self.dragController->GetDraggedItem()) {
        NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
        DevicePoint mousePt = {(float)p.x, (float)p.y};
        self.dragController->OnMouseDragged(mousePt);
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseUp:(NSEvent *)event {
    self.dragController->OnMouseUp();
}

- (void)mouseDownAtPoint:(NSPoint)pt viewY:(float)viewY {
    DevicePoint mousePt = CoordTransform::ViewToDevice({(float)pt.x, (float)pt.y}, viewY);
    self.dragController->OnMouseDown(mousePt);
}

- (void)mouseDraggedAtPoint:(NSPoint)pt viewY:(float)viewY {
    if (self.dragController->GetDraggedItem()) {
        DevicePoint mousePt = CoordTransform::ViewToDevice({(float)pt.x, (float)pt.y}, viewY);
        self.dragController->OnMouseDragged(mousePt);
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseUpAtPoint:(NSPoint)pt {
    self.dragController->OnMouseUp();
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
