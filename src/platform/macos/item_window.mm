#import "item_window.h"
#import "world.h"
#import "config.h"
#import "item_renderer.h"
#import "coordinate_system.h"
#include <cmath>
#include <cstdio>
#include <ctime>



// Configurable log path (or use centralized logging)
static const char* GetItemDragLogPath() {
    return g_config.general.itemDragLogPath.empty() ? "/tmp/cadgoose_item_drag.log" : g_config.general.itemDragLogPath.c_str();
}
static constexpr float kCloseButtonPadding = 4.0f;

static FILE* s_debugFile = NULL;

static void ItemLog(const char* fmt, ...) {
    if (!s_debugFile) {
        s_debugFile = fopen(GetItemDragLogPath(), "w");
    }
    if (!s_debugFile) return;
    time_t now = time(nullptr);
    struct tm* tm = localtime(&now);
    char ts[32]; strftime(ts, sizeof(ts), "%H:%M:%S", tm);
    fprintf(s_debugFile, "[%s] ", ts);
    va_list args;
    va_start(args, fmt);
    vfprintf(s_debugFile, fmt, args);
    va_end(args);
    fprintf(s_debugFile, "\n");
    fflush(s_debugFile);
}

static BOOL IsItemValid(DroppedItem* item) {
    if (!item || !item->data) return NO;
    for (auto& i : g_world.droppedItems) {
        if (&i == item && i.data) return YES;
    }
    return NO;
}

// ============================================================
// Coordinate helpers
// ============================================================

// DEVICE coords: top-left origin, Y-down (item.pos, all game logic)
// SCREEN coords: bottom-left origin, Y-up (macOS window frame, NSEvent)


static DevicePoint GetMouseDeviceCoords(NSEvent* event, NSWindow* window) {
    NSPoint globalScreenPt;
    if (event && window) {
        globalScreenPt = [window convertPointToScreen:[event locationInWindow]];
    } else {
        globalScreenPt = [NSEvent mouseLocation];
    }
    ScreenPoint screenPt = {(float)globalScreenPt.x, (float)globalScreenPt.y};
    NSScreen* mainScreen = [NSScreen mainScreen];
    return CoordTransform::ScreenToDeviceMacOS(screenPt, (float)mainScreen.frame.size.height);
}


// Calculate the bounding box size of a rotated rectangle
static DevicePoint RotatedBoundsSize(float width, float height, float rotation, float scale) {
    float w = width * scale;
    float h = height * scale;
    float cosA = std::abs(std::cos(rotation));
    float sinA = std::abs(std::sin(rotation));
    return {w * cosA + h * sinA, w * sinA + h * cosA};
}

// ============================================================
// ItemContentView — Rotated view that draws a single item
// ============================================================

@implementation ItemContentView {
    DroppedItem* _item;
    BOOL _isDragging;
    DevicePoint _dragStartMouseDevice;
    DevicePoint _dragStartItemPos;
}

- (instancetype)initWithFrame:(NSRect)frame item:(DroppedItem*)item {
    self = [super initWithFrame:frame];
    if (self) {
        _item = item;
        self.wantsLayer = YES;
        self.layer.backgroundColor = [[NSColor clearColor] CGColor];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    if (!_item || !_item->data) return;

    if (!IsItemValid(_item)) { _item = nullptr; return; }

    NSPoint windowPt = [event locationInWindow];
    NSPoint globalScreenPt = [self.window convertPointToScreen:windowPt];
    ScreenPoint screenPt = {(float)globalScreenPt.x, (float)globalScreenPt.y};
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    DevicePoint mouseDevice = GetMouseDeviceCoords(event, self.window);

    ItemLog("MOUSE_DOWN: windowPt=(%.1f,%.1f) globalScreenPt=(%.1f,%.1f) devicePt=(%.1f,%.1f) screenH=%.1f",
            windowPt.x, windowPt.y, screenPt.x, screenPt.y, mouseDevice.x, mouseDevice.y, screenH);
    ItemLog("  item.pos=(%.1f,%.1f) item.size=(%.0fx%.0f) rotation=%.3f",
            _item->pos.x, _item->pos.y, _item->data->w, _item->data->h, _item->rotation);
    ItemLog("  window.frame=(%.1f,%.1f,%.1f,%.1f)",
            (float)self.window.frame.origin.x, (float)self.window.frame.origin.y,
            (float)self.window.frame.size.width, (float)self.window.frame.size.height);

    // Check close button — use HitTest with DEVICE coords
    if (_item->data->type != ItemData::TOY) {
        DevicePoint itemCenter = ItemCoords::Center({_item->pos.x, _item->pos.y},
                                                     _item->data->w, _item->data->h,
                                                     g_config.general.globalScale);
        if (HitTest::PointInCloseButton(mouseDevice, itemCenter,
                                        _item->data->w, _item->data->h,
                                        _item->rotation,
                                        g_config.render.closeButtonSize,
                                        g_config.general.globalScale)) {
            ItemLog("  CLOSE BUTTON CLICKED");
            delete _item->data;
            _item->data = nullptr;
            _item->pinned = true;
            [self.window close];
            return;
        }
    }

    // Check if point is inside item — use HitTest with DEVICE coords
    DevicePoint itemCenter = ItemCoords::Center({_item->pos.x, _item->pos.y},
                                                 _item->data->w, _item->data->h,
                                                 g_config.general.globalScale);
    if (HitTest::PointInItem(mouseDevice, itemCenter,
                             _item->data->w, _item->data->h,
                             _item->rotation,
                             g_config.general.globalScale)) {
        // Start drag — all in DEVICE coords
        _isDragging = YES;
        ((ItemWindow*)self.parentWindow).isBeingDragged = YES;
        _dragStartMouseDevice = mouseDevice;
        _dragStartItemPos = {_item->pos.x, _item->pos.y};
        _item->pinned = true;

        ItemLog("  DRAG START: mouseDevice=(%.1f,%.1f) itemPos=(%.1f,%.1f)",
                _dragStartMouseDevice.x, _dragStartMouseDevice.y,
                _dragStartItemPos.x, _dragStartItemPos.y);
    }
}

- (void)mouseDragged:(NSEvent *)event {
    if (!_isDragging || !_item || !_item->data) return;

    if (!IsItemValid(_item)) { _isDragging = NO; _item = nullptr; return; }

    NSPoint globalScreenPt = [self.window convertPointToScreen:[event locationInWindow]];
    ScreenPoint screenPt = {(float)globalScreenPt.x, (float)globalScreenPt.y};
    DevicePoint mouseDevice = GetMouseDeviceCoords(event, self.window);

    // Delta in DEVICE coords (same as screen delta since X is same and Y flip cancels in subtraction)
    float dx = mouseDevice.x - _dragStartMouseDevice.x;
    float dy = mouseDevice.y - _dragStartMouseDevice.y;

    DevicePoint newPos = {_dragStartItemPos.x + dx, _dragStartItemPos.y + dy};

    ItemLog("DRAG: globalScreenPt=(%.1f,%.1f) devicePt=(%.1f,%.1f) delta=(%.1f,%.1f) newPos=(%.1f,%.1f)",
            screenPt.x, screenPt.y, mouseDevice.x, mouseDevice.y, dx, dy, newPos.x, newPos.y);

    _item->pos = newPos;

    // Update window frame — same calculation as updatePosition
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    float scale = g_config.general.globalScale;
    DevicePoint winSize = RotatedBoundsSize(_item->data->w, _item->data->h, _item->rotation, scale);
    DevicePoint itemCenter = ItemCoords::Center({_item->pos.x, _item->pos.y},
                                                 _item->data->w, _item->data->h, scale);
    DevicePoint windowTopLeft = {itemCenter.x - winSize.x * 0.5f, itemCenter.y - winSize.y * 0.5f};
    ScreenPoint screenOrigin = CoordTransform::DeviceToScreenMacOS({windowTopLeft.x, windowTopLeft.y + winSize.y}, screenH);

    [self.window setFrameOrigin:NSMakePoint(screenOrigin.x, screenOrigin.y)];
}

- (void)mouseUp:(NSEvent *)event {
    ItemLog("MOUSE_UP: isDragging=%d", _isDragging);
    _isDragging = NO;
    if (self.parentWindow) {
        ((ItemWindow*)self.parentWindow).isBeingDragged = NO;
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!_item || !_item->data) return;

    if (!IsItemValid(_item)) { _item = nullptr; return; }

    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGContextClearRect(ctx, self.bounds);
    CGContextSaveGState(ctx);

    // Draw item centered and rotated
    CGContextTranslateCTM(ctx, self.bounds.size.width * 0.5f, self.bounds.size.height * 0.5f);
    CGContextRotateCTM(ctx, -_item->rotation);

    float scale = g_config.general.globalScale;
    float itemW = _item->data->w * scale;
    float itemH = _item->data->h * scale;

    ItemRenderer* renderer = ItemRenderer::ForType(_item->data->type);
    bool showClose = renderer->DrawDropped(ctx, *_item, itemW, itemH);

    if (showClose) {
        float closeX = -itemW / 2.0f;
        float closeY = -itemH / 2.0f;
        CGContextSetRGBFillColor(ctx, g_config.render.closeButtonColor.r,
                                 g_config.render.closeButtonColor.g,
                                 g_config.render.closeButtonColor.b, 0.8);
        CGContextFillRect(ctx, CGRectMake(closeX, closeY, g_config.render.closeButtonSize, g_config.render.closeButtonSize));
        CGContextSetRGBStrokeColor(ctx, g_config.render.closeButtonStroke.r,
                                   g_config.render.closeButtonStroke.g,
                                   g_config.render.closeButtonStroke.b, 1.0);
        CGContextSetLineWidth(ctx, 2.0);
        CGContextMoveToPoint(ctx, closeX + kCloseButtonPadding, closeY + kCloseButtonPadding);
        CGContextAddLineToPoint(ctx, closeX + g_config.render.closeButtonSize - kCloseButtonPadding, closeY + g_config.render.closeButtonSize - kCloseButtonPadding);
        CGContextMoveToPoint(ctx, closeX + g_config.render.closeButtonSize - kCloseButtonPadding, closeY + kCloseButtonPadding);
        CGContextAddLineToPoint(ctx, closeX + kCloseButtonPadding, closeY + g_config.render.closeButtonSize - kCloseButtonPadding);
        CGContextStrokePath(ctx);
    }

    CGContextRestoreGState(ctx);
}

@end

// ============================================================
// ItemWindow — Transparent floating window for a single item
// ============================================================

@implementation ItemWindow {
    DroppedItem* _item;
    DevicePoint _lastPosition;
    bool _hasLastPosition;
    BOOL _isDragging;
}

@synthesize isBeingDragged = _isBeingDragged;

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

- (BOOL)isOpaque {
    return NO;
}

- (instancetype)initWithItem:(DroppedItem*)item {
    _item = item;
    _hasLastPosition = false;

    float scale = g_config.general.globalScale;
    DevicePoint winSize = RotatedBoundsSize(item->data->w, item->data->h, item->rotation, scale);

    // Window is centered on item center, sized to rotated bounding box
    DevicePoint itemCenter = ItemCoords::Center({item->pos.x, item->pos.y},
                                                 item->data->w, item->data->h, scale);
    // Window frame origin (bottom-left in SCREEN coords)
    DevicePoint windowTopLeft = {itemCenter.x - winSize.x * 0.5f, itemCenter.y - winSize.y * 0.5f};

    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    ScreenPoint screenOrigin = CoordTransform::DeviceToScreenMacOS({windowTopLeft.x, windowTopLeft.y + winSize.y}, screenH);

    NSRect frame = NSMakeRect(screenOrigin.x, screenOrigin.y, winSize.x, winSize.y);

    ItemLog("ITEM_WINDOW_CREATE: item.pos=(%.1f,%.1f) [DEVICE] center=(%.1f,%.1f) winSize=(%.1f,%.1f) [DEVICE]",
            item->pos.x, item->pos.y, itemCenter.x, itemCenter.y, winSize.x, winSize.y);
    ItemLog("  screenH=%.1f screenOrigin=(%.1f,%.1f) frame=(%.1f,%.1f,%.1f,%.1f) [SCREEN]",
            screenH, screenOrigin.x, screenOrigin.y,
            frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);

    self = [super initWithContentRect:frame
                            styleMask:NSWindowStyleMaskBorderless
                              backing:NSBackingStoreBuffered
                                defer:NO];
    if (self) {
        self.backgroundColor = [NSColor clearColor];
        self.ignoresMouseEvents = YES; // Click-through by default
        self.acceptsMouseMovedEvents = YES;
        self.level = NSStatusWindowLevel;
        self.hasShadow = NO;
        self.releasedWhenClosed = NO; // Lifetime managed by ItemWindowManager dictionary
        self.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                  NSWindowCollectionBehaviorIgnoresCycle;

        ItemContentView* contentView = [[ItemContentView alloc] initWithFrame:self.contentView.bounds item:item];
        contentView.parentWindow = self;
        self.contentView = contentView;

        [self orderFront:nil];
    }
    return self;
}

- (BOOL)isPointInsideItem:(NSPoint)pt {
    if (!_item || !_item->data) return NO;

    if (!IsItemValid(_item)) { _item = nullptr; return NO; }

    // pt is in local view coordinates (top-left origin, Y-down, isFlipped=YES)
    // Convert to DEVICE coords relative to item center
    float scale = g_config.general.globalScale;
    DevicePoint itemCenter = ItemCoords::Center({_item->pos.x, _item->pos.y},
                                                 _item->data->w, _item->data->h,
                                                 scale);

    // View center in view coords
    float viewCx = self.contentView.bounds.size.width * 0.5f;
    float viewCy = self.contentView.bounds.size.height * 0.5f;

    // pt relative to view center, then mapped to device coords
    // Since view is isFlipped=YES and sized to item, view coords = device coords relative to item top-left
    DevicePoint viewMousePt = {(float)pt.x, (float)pt.y};
    DevicePoint devicePt = {_item->pos.x + viewMousePt.x, _item->pos.y + viewMousePt.y};

    return HitTest::PointInItem(devicePt, itemCenter,
                                _item->data->w, _item->data->h,
                                _item->rotation,
                                scale);
}

- (void)updatePosition {
    if (!_item || !_item->data) return;

    if (!IsItemValid(_item)) { _item = nullptr; return; }

    float scale = g_config.general.globalScale;
    DevicePoint winSize = RotatedBoundsSize(_item->data->w, _item->data->h, _item->rotation, scale);
    DevicePoint itemCenter = ItemCoords::Center({_item->pos.x, _item->pos.y},
                                                 _item->data->w, _item->data->h, scale);
    DevicePoint windowTopLeft = {itemCenter.x - winSize.x * 0.5f, itemCenter.y - winSize.y * 0.5f};

    // Only update if position changed
    if (_hasLastPosition &&
        std::abs(_item->pos.x - _lastPosition.x) < 0.1f &&
        std::abs(_item->pos.y - _lastPosition.y) < 0.1f) {
        return;
    }

    _lastPosition = {_item->pos.x, _item->pos.y};
    _hasLastPosition = true;

    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    ScreenPoint screenOrigin = CoordTransform::DeviceToScreenMacOS({windowTopLeft.x, windowTopLeft.y + winSize.y}, screenH);

    NSRect newFrame = NSMakeRect(screenOrigin.x, screenOrigin.y, winSize.x, winSize.y);

    ItemLog("UPDATE_POSITION: item.pos=(%.1f,%.1f) [DEVICE] center=(%.1f,%.1f) winSize=(%.1f,%.1f) screenOrigin=(%.1f,%.1f) [SCREEN]",
            _item->pos.x, _item->pos.y,
            itemCenter.x, itemCenter.y,
            winSize.x, winSize.y,
            screenOrigin.x, screenOrigin.y);

    [self setFrame:newFrame display:YES];
}

- (void)closeAndRemove {
    [self close];
}

@end

// ============================================================
// ItemWindowManager — Cursor polling + dynamic ignoresMouseEvents
// ============================================================

@implementation ItemWindowManager {
    NSMutableDictionary<NSNumber*, ItemWindow*>* _windows;
    NSInteger _nextId;
}

@synthesize windows = _windows;

+ (instancetype)shared {
    static ItemWindowManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[ItemWindowManager alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _windows = [NSMutableDictionary dictionary];
        _nextId = 1;
    }
    return self;
}

- (void)syncWindows {
    // Remove windows for items that no longer exist or have null data
    NSMutableArray* toRemove = [NSMutableArray array];
    for (NSNumber* key in _windows) {
        ItemWindow* win = _windows[key];
        if (!win) continue;

        BOOL itemValid = IsItemValid(win.item);
        if (!itemValid) {
            [win close];
            [toRemove addObject:key];
            continue;
        }
    }
    for (NSNumber* key in toRemove) {
        [_windows removeObjectForKey:key];
    }

    // Create windows for new items
    for (auto& item : g_world.droppedItems) {
        BOOL exists = NO;
        for (NSNumber* key in _windows) {
            ItemWindow* win = _windows[key];
            if (win.item == &item) {
                exists = YES;
                break;
            }
        }
        if (!exists && item.data) {
            NSNumber* key = @(_nextId++);
            ItemWindow* win = [[ItemWindow alloc] initWithItem:&item];
            _windows[key] = win;
        }
    }

    // Update positions and hit states for existing items
    if (_windows.count == 0) return;

    DevicePoint mouseDevice = GetMouseDeviceCoords(nil, nil);

    for (NSNumber* key in _windows) {
        ItemWindow* win = _windows[key];
        if (!win || !win.item) continue;

        BOOL itemValid = IsItemValid(win.item);
        if (!itemValid) continue;

        // Skip position update during drag (window moves via mouseDragged)
        if (!win.isBeingDragged) {
            [win updatePosition];
        }

        // Toggle ignoresMouseEvents based on cursor position
        DevicePoint itemCenter = ItemCoords::Center({win.item->pos.x, win.item->pos.y},
                                                     win.item->data->w, win.item->data->h,
                                                     g_config.general.globalScale);
        bool inside = HitTest::PointInItem(mouseDevice, itemCenter,
                                 win.item->data->w, win.item->data->h,
                                 win.item->rotation,
                                 g_config.general.globalScale);
        if (inside) {
            if (win.ignoresMouseEvents) win.ignoresMouseEvents = NO;
        } else {
            if (!win.ignoresMouseEvents) win.ignoresMouseEvents = YES;
        }
    }
}

- (void)closeAll {
    for (NSNumber* key in _windows) {
        [_windows[key] close];
    }
    [_windows removeAllObjects];
}

@end

