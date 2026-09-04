#pragma once

// item_window_logic — the pure decision/geometry layer behind ItemWindow.
//
// ItemWindow (src/platform/macos/item_window.mm) is an NSWindow subclass: it
// cannot be instantiated without a window server, so anything living inside its
// methods is unreachable from a headless test. Everything here is the part of
// that class that is just arithmetic on item state, lifted out so it can be
// tested directly. item_window.mm keeps only the AppKit calls.
//
// Coordinate spaces (see coordinate_system.h):
//   DEVICE — top-left origin, Y-down. All game/item state.
//   SCREEN — bottom-left origin, Y-up. macOS window frames and NSEvent.

#include "coordinate_system.h"

namespace item_window_logic {

// Bounding-box size of a rectangle rotated by `rotation` radians and uniformly
// scaled. Used to size the window that must contain the rotated item.
DevicePoint RotatedBoundsSize(float width, float height, float rotation, float scale);

// The window frame for an item, in SCREEN coords, ready for -setFrameOrigin:.
struct WindowFrame {
    ScreenPoint origin;  // bottom-left, SCREEN space
    DevicePoint size;    // width/height, already rotation-expanded
};
WindowFrame ComputeWindowFrame(DevicePoint itemPos, float width, float height,
                               float rotation, float scale, float screenHeight);

// -updatePosition skips work when the item has not meaningfully moved. Pulled
// out so the epsilon and the first-call behaviour are pinned by tests rather
// than living as an untested early-return inside an ObjC method.
bool ShouldUpdatePosition(DevicePoint lastPos, DevicePoint newPos, bool hasLastPosition);

// Maps a point in ItemContentView's local coords (top-left origin, Y-down,
// because the view is isFlipped) to DEVICE coords.
DevicePoint LocalViewPointToDevice(DevicePoint itemPos, DevicePoint localPoint);

// Whether a local view point falls inside the item's ROTATED rectangle — the
// test that makes click-through outside the item work.
bool IsLocalPointInsideItem(DevicePoint itemPos, DevicePoint localPoint,
                            float width, float height, float rotation, float scale);

// What -syncWindows should do with one item this pass. The AppKit side just
// executes the verdict.
enum class SyncAction {
    None,          // leave the window alone
    Create,        // no window yet and the item is valid
    Destroy,       // window exists but the item is gone or invalid
    SetInteractive,    // cursor is over the item: accept mouse events
    SetClickThrough,   // cursor is elsewhere: pass clicks to whatever is below
};

// `hasWindow`   — a window already exists for this item
// `itemValid`   — the item still exists and has data
// `cursorInside`— the cursor is inside the item's rotated bounds
// `currentlyInteractive` — the window is currently accepting mouse events
SyncAction DecideSyncAction(bool hasWindow, bool itemValid, bool cursorInside,
                            bool currentlyInteractive);

}  // namespace item_window_logic
