#include "item_window_logic.h"

#include <cmath>

namespace item_window_logic {

DevicePoint RotatedBoundsSize(float width, float height, float rotation, float scale) {
    const float w = width * scale;
    const float h = height * scale;
    const float cosA = std::abs(std::cos(rotation));
    const float sinA = std::abs(std::sin(rotation));
    return {w * cosA + h * sinA, w * sinA + h * cosA};
}

WindowFrame ComputeWindowFrame(DevicePoint itemPos, float width, float height,
                               float rotation, float scale, float screenHeight) {
    const DevicePoint winSize = RotatedBoundsSize(width, height, rotation, scale);
    const DevicePoint itemCenter = ItemCoords::Center(itemPos, width, height, scale);
    const DevicePoint windowTopLeft = {itemCenter.x - winSize.x * 0.5f,
                                       itemCenter.y - winSize.y * 0.5f};

    // The window's SCREEN origin is its bottom-left, so take the DEVICE
    // top-left and step down by the window height before converting.
    const ScreenPoint origin = CoordTransform::DeviceToScreenMacOS(
        {windowTopLeft.x, windowTopLeft.y + winSize.y}, screenHeight);

    return {origin, winSize};
}

bool ShouldUpdatePosition(DevicePoint lastPos, DevicePoint newPos, bool hasLastPosition) {
    // First call always updates: there is no previous frame to compare against.
    if (!hasLastPosition) return true;
    constexpr float kEpsilon = 0.1f;
    return std::abs(newPos.x - lastPos.x) >= kEpsilon ||
           std::abs(newPos.y - lastPos.y) >= kEpsilon;
}

DevicePoint LocalViewPointToDevice(DevicePoint itemPos, DevicePoint localPoint) {
    // The content view is isFlipped and sized to the item, so local view coords
    // are DEVICE coords relative to the item's top-left.
    return {itemPos.x + localPoint.x, itemPos.y + localPoint.y};
}

bool IsLocalPointInsideItem(DevicePoint itemPos, DevicePoint localPoint,
                            float width, float height, float rotation, float scale) {
    const DevicePoint itemCenter = ItemCoords::Center(itemPos, width, height, scale);
    const DevicePoint devicePt = LocalViewPointToDevice(itemPos, localPoint);
    return HitTest::PointInItem(devicePt, itemCenter, width, height, rotation, scale);
}

SyncAction DecideSyncAction(bool hasWindow, bool itemValid, bool cursorInside,
                            bool currentlyInteractive) {
    if (!itemValid) {
        // A window outlived its item — tear it down. Nothing to do otherwise.
        return hasWindow ? SyncAction::Destroy : SyncAction::None;
    }
    if (!hasWindow) return SyncAction::Create;

    // Window and item both live: the only remaining decision is whether the
    // window should swallow mouse events or pass them through. Only report a
    // change when the desired state differs from the current one, so the caller
    // does not thrash ignoresMouseEvents every frame.
    if (cursorInside && !currentlyInteractive) return SyncAction::SetInteractive;
    if (!cursorInside && currentlyInteractive) return SyncAction::SetClickThrough;
    return SyncAction::None;
}

}  // namespace item_window_logic
