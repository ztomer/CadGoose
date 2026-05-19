import re

with open('src/platform/macos/item_window.mm', 'r') as f:
    content = f.read()

# 1. Coordinate helpers
coord_helper = """
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
"""

content = content.replace("// Calculate the bounding box size of a rotated rectangle", coord_helper + "\n// Calculate the bounding box size of a rotated rectangle")


# mouseDown
md_old = """    // Convert window-relative point to global screen coords
    NSPoint windowPt = [event locationInWindow];
    NSPoint globalScreenPt = [self.window convertPointToScreen:windowPt];
    ScreenPoint screenPt = {(float)globalScreenPt.x, (float)globalScreenPt.y};

    // Convert to DEVICE coords for game logic
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    DevicePoint mouseDevice = CoordTransform::ScreenToDeviceMacOS(screenPt, screenH);"""
md_new = """    NSPoint windowPt = [event locationInWindow];
    NSPoint globalScreenPt = [self.window convertPointToScreen:windowPt];
    ScreenPoint screenPt = {(float)globalScreenPt.x, (float)globalScreenPt.y};
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    DevicePoint mouseDevice = GetMouseDeviceCoords(event, self.window);"""
content = content.replace(md_old, md_new)

# mouseDragged
mdrag_old = """    // Convert window-relative point to global screen coords
    NSPoint windowPt = [event locationInWindow];
    NSPoint globalScreenPt = [self.window convertPointToScreen:windowPt];
    ScreenPoint screenPt = {(float)globalScreenPt.x, (float)globalScreenPt.y};

    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    DevicePoint mouseDevice = CoordTransform::ScreenToDeviceMacOS(screenPt, screenH);"""
mdrag_new = """    NSPoint globalScreenPt = [self.window convertPointToScreen:[event locationInWindow]];
    ScreenPoint screenPt = {(float)globalScreenPt.x, (float)globalScreenPt.y};
    DevicePoint mouseDevice = GetMouseDeviceCoords(event, self.window);"""
content = content.replace(mdrag_old, mdrag_new)

# syncWindows
sync_old = """    NSPoint rawMouseLoc = [NSEvent mouseLocation];
    ScreenPoint mouseScreen = {(float)rawMouseLoc.x, (float)rawMouseLoc.y};
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenH = (float)mainScreen.frame.size.height;
    DevicePoint mouseDevice = CoordTransform::ScreenToDeviceMacOS(mouseScreen, screenH);"""
sync_new = """    DevicePoint mouseDevice = GetMouseDeviceCoords(nil, nil);"""
content = content.replace(sync_old, sync_new)

# Magic numbers in drawRect
dr_old = """        CGContextMoveToPoint(ctx, closeX + 4, closeY + 4);
        CGContextAddLineToPoint(ctx, closeX + g_config.render.closeButtonSize - 4, closeY + g_config.render.closeButtonSize - 4);
        CGContextMoveToPoint(ctx, closeX + g_config.render.closeButtonSize - 4, closeY + 4);
        CGContextAddLineToPoint(ctx, closeX + 4, closeY + g_config.render.closeButtonSize - 4);"""
dr_new = """        CGContextMoveToPoint(ctx, closeX + kCloseButtonPadding, closeY + kCloseButtonPadding);
        CGContextAddLineToPoint(ctx, closeX + g_config.render.closeButtonSize - kCloseButtonPadding, closeY + g_config.render.closeButtonSize - kCloseButtonPadding);
        CGContextMoveToPoint(ctx, closeX + g_config.render.closeButtonSize - kCloseButtonPadding, closeY + kCloseButtonPadding);
        CGContextAddLineToPoint(ctx, closeX + kCloseButtonPadding, closeY + g_config.render.closeButtonSize - kCloseButtonPadding);"""
content = content.replace(dr_old, dr_new)

# Dependency Inversion for log path:
log_old = """static constexpr const char* kItemDragLogPath = "/tmp/cadgoose_item_drag.log";"""
log_new = """// Configurable log path (or use centralized logging)
static const char* GetItemDragLogPath() {
    return g_config.general.itemDragLogPath.empty() ? "/tmp/cadgoose_item_drag.log" : g_config.general.itemDragLogPath.c_str();
}"""
content = content.replace(log_old, log_new)
content = content.replace('s_debugFile = fopen(kItemDragLogPath, "w");', 's_debugFile = fopen(GetItemDragLogPath(), "w");')


# Extract DragTest
drag_test_start = content.find("// ============================================================\n// Drag Test")
if drag_test_start != -1:
    drag_test_code = content[drag_test_start:]
    content = content[:drag_test_start]
    with open('src/platform/macos/item_window_test.mm', 'w') as f:
        f.write('#import "item_window.h"\n#import "world.h"\n#import "config.h"\n#import "coordinate_system.h"\n\nextern std::list<DroppedItem> g_droppedItems;\n\n')
        f.write(drag_test_code)

with open('src/platform/macos/item_window.mm', 'w') as f:
    f.write(content)
