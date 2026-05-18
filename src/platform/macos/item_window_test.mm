#import "item_window.h"
#import "world.h"
#import "config.h"
#import "coordinate_system.h"



#include <cstdio>
#include <cstdarg>
#include <cmath>

static void ItemLog(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static DevicePoint RotatedBoundsSize(float width, float height, float rotation, float scale) {
    float w = width * scale;
    float h = height * scale;
    float cosA = std::abs(std::cos(rotation));
    float sinA = std::abs(std::sin(rotation));
    return {w * cosA + h * sinA, w * sinA + h * cosA};
}

// ============================================================
// Drag Test — Programmatic drag synthesis for testing
// ============================================================

#import <ApplicationServices/ApplicationServices.h>

void ItemWindow_DragTest(float targetX, float targetY) {
    if (g_world.droppedItems.empty() || !g_world.droppedItems.front().data) {
        ItemLog("DRAG_TEST: no items to drag");
        return;
    }

    // Must run on main thread for AppKit calls
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            ItemWindow_DragTest(targetX, targetY);
        });
        return;
    }

    DroppedItem& item = g_world.droppedItems.front();
    float scale = g_config.general.globalScale;
    DevicePoint itemCenter = ItemCoords::Center({item.pos.x, item.pos.y},
                                                 item.data->w, item.data->h, scale);

    ItemLog("DRAG_TEST: itemCenter=(%.1f,%.1f) target=(%.1f,%.1f)",
            itemCenter.x, itemCenter.y, targetX, targetY);

    // Find the item window
    ItemWindowManager* manager = [ItemWindowManager shared];
    ItemWindow* targetWin = nil;
    ItemContentView* contentView = nil;
    for (NSNumber* key in manager.windows) {
        ItemWindow* win = manager.windows[key];
        if (win && win.item == &item) {
            targetWin = win;
            contentView = (ItemContentView*)win.contentView;
            break;
        }
    }

    if (!targetWin || !contentView) {
        ItemLog("DRAG_TEST: item window not found");
        return;
    }

    // Window is sized to rotated bounding box, centered on item center
    DevicePoint winSize = RotatedBoundsSize(item.data->w, item.data->h, item.rotation, scale);

    // View coords (isFlipped=YES): top-left origin, Y-down
    // Window coords: bottom-left origin, Y-up
    // Conversion: windowY = viewHeight - viewY
    NSPoint viewCenter = NSMakePoint(winSize.x * 0.5f, winSize.y * 0.5f);
    NSPoint windowCenter = NSMakePoint(viewCenter.x, winSize.y - viewCenter.y);

    // Calculate target view coords
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    ScreenPoint targetScreen = CoordTransform::DeviceToScreenMacOS({targetX, targetY}, screenH);
    DevicePoint targetDevice = CoordTransform::ScreenToDeviceMacOS(targetScreen, screenH);
    float dx = targetDevice.x - item.pos.x;
    float dy = targetDevice.y - item.pos.y;
    NSPoint viewTarget = NSMakePoint(viewCenter.x + dx, viewCenter.y + dy);
    NSPoint windowTarget = NSMakePoint(viewTarget.x, winSize.y - viewTarget.y);

    ItemLog("DRAG_TEST: viewCenter=(%.1f,%.1f) windowCenter=(%.1f,%.1f)",
            viewCenter.x, viewCenter.y, windowCenter.x, windowCenter.y);
    ItemLog("DRAG_TEST: viewTarget=(%.1f,%.1f) windowTarget=(%.1f,%.1f)",
            viewTarget.x, viewTarget.y, windowTarget.x, windowTarget.y);

    // Synthesize mouse events with window-local coordinates
    NSEvent* downEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown
                                            location:windowCenter
                                       modifierFlags:0
                                           timestamp:NSProcessInfo.processInfo.systemUptime
                                        windowNumber:targetWin.windowNumber
                                             context:nil
                                         eventNumber:0
                                          clickCount:1
                                            pressure:1.0];

    NSEvent* dragEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
                                            location:windowTarget
                                       modifierFlags:0
                                           timestamp:NSProcessInfo.processInfo.systemUptime
                                        windowNumber:targetWin.windowNumber
                                             context:nil
                                         eventNumber:1
                                          clickCount:1
                                            pressure:1.0];

    NSEvent* upEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp
                                          location:windowTarget
                                     modifierFlags:0
                                         timestamp:NSProcessInfo.processInfo.systemUptime
                                      windowNumber:targetWin.windowNumber
                                           context:nil
                                       eventNumber:2
                                        clickCount:1
                                          pressure:0.0];

    ItemLog("DRAG_TEST: sending mouseDown");
    [contentView mouseDown:downEvent];
    usleep(100000);

    ItemLog("DRAG_TEST: sending mouseDragged");
    [contentView mouseDragged:dragEvent];
    usleep(100000);

    ItemLog("DRAG_TEST: sending mouseUp");
    [contentView mouseUp:upEvent];

    ItemLog("DRAG_TEST: done. item.pos now=(%.1f,%.1f)", item.pos.x, item.pos.y);
}
