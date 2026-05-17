// ===========================
// coordinate_system.h
// ===========================
// Strongly-typed coordinate system to prevent coordinate space bugs.
//
// COORDINATE SPACES:
//   DEVICE: Screen pixels, top-left origin, Y-down.
//     All game logic (goose pos, targets, items, cursor) lives here.
//     g_screenWidth, g_screenHeight are in DEVICE units.
//
//   WORLD: Goose-local unscaled coords (design units).
//     Only rig parts (neckHead, body, feet) are defined here.
//     These are config-driven offsets that get scaled when rendered.
//
//   SCREEN: Raw OS screen coordinates (identical to DEVICE on macOS/Linux).
//     Used only at platform boundaries (NSEvent, CGEvent, X11).
//
//   VIEW: NSView-local coordinates (top-left origin, Y-down for isFlipped=YES).
//     Used only in macOS event handlers.
//
// TRANSFORM RULES:
//   - Never mix DEVICE and WORLD in arithmetic without explicit transform
//   - All goose->pos, goose->target, item.pos are DEVICE
//   - All rig.* fields are WORLD (relative to goose body)
//   - All cursor positions are DEVICE
//   - All screen dimensions are DEVICE
// ===========================
#ifndef COORDINATE_SYSTEM_H
#define COORDINATE_SYSTEM_H

#include "goose_math.h"
#include <cmath>
#include <algorithm>

// ============================================================
// Typed Coordinate Wrappers
// ============================================================
// These are zero-cost abstractions (same layout as Vector2) that
// prevent accidental mixing of coordinate spaces at compile time.

struct DevicePoint {
    float x = 0, y = 0;

    DevicePoint() = default;
    DevicePoint(float x_, float y_) : x(x_), y(y_) {}
    explicit DevicePoint(Vector2 v) : x(v.x), y(v.y) {}
    operator Vector2() const { return {x, y}; }

    DevicePoint operator+(const DevicePoint& o) const { return {x + o.x, y + o.y}; }
    DevicePoint operator-(const DevicePoint& o) const { return {x - o.x, y - o.y}; }
    DevicePoint operator*(float s) const { return {x * s, y * s}; }
    DevicePoint operator/(float s) const { return {x / s, y / s}; }
    DevicePoint& operator+=(const DevicePoint& o) { x += o.x; y += o.y; return *this; }
    DevicePoint& operator-=(const DevicePoint& o) { x -= o.x; y -= o.y; return *this; }

    static float Distance(DevicePoint a, DevicePoint b) {
        return std::hypot(b.x - a.x, b.y - a.y);
    }
};

struct WorldPoint {
    float x = 0, y = 0;

    WorldPoint() = default;
    WorldPoint(float x_, float y_) : x(x_), y(y_) {}
    explicit WorldPoint(Vector2 v) : x(v.x), y(v.y) {}
    operator Vector2() const { return {x, y}; }

    WorldPoint operator+(const WorldPoint& o) const { return {x + o.x, y + o.y}; }
    WorldPoint operator-(const WorldPoint& o) const { return {x - o.x, y - o.y}; }
    WorldPoint operator*(float s) const { return {x * s, y * s}; }
    WorldPoint operator/(float s) const { return {x / s, y / s}; }

    static float Distance(WorldPoint a, WorldPoint b) {
        return std::hypot(b.x - a.x, b.y - a.y);
    }
};

struct ScreenPoint {
    float x = 0, y = 0;

    ScreenPoint() = default;
    ScreenPoint(float x_, float y_) : x(x_), y(y_) {}
    explicit ScreenPoint(Vector2 v) : x(v.x), y(v.y) {}
    operator Vector2() const { return {x, y}; }
};

struct ViewPoint {
    float x = 0, y = 0;

    ViewPoint() = default;
    ViewPoint(float x_, float y_) : x(x_), y(y_) {}
    explicit ViewPoint(Vector2 v) : x(v.x), y(v.y) {}
    operator Vector2() const { return {x, y}; }
};

// ============================================================
// Coordinate Transforms
// ============================================================

struct CoordTransform {
    // WORLD (goose-local) → DEVICE (screen pixels)
    // Formula: goosePos + (worldPos - goosePos) * globalScale
    // Used for: rendering rig parts, beak tip, foot positions
    static DevicePoint WorldToDevice(WorldPoint worldPos, DevicePoint goosePos, float globalScale) {
        return {
            goosePos.x + (worldPos.x - goosePos.x) * globalScale,
            goosePos.y + (worldPos.y - goosePos.y) * globalScale
        };
    }

    // DEVICE → WORLD (inverse transform)
    static WorldPoint DeviceToWorld(DevicePoint devicePos, DevicePoint goosePos, float globalScale) {
        if (globalScale < 0.001f) return {devicePos.x, devicePos.y};
        return {
            goosePos.x + (devicePos.x - goosePos.x) / globalScale,
            goosePos.y + (devicePos.y - goosePos.y) / globalScale
        };
    }

    // Scale a scalar value from world units to device units
    static float Scale(float worldValue, float globalScale) {
        return worldValue * globalScale;
    }

    // SCREEN → DEVICE (identity on macOS/Linux, but explicit for documentation)
    static DevicePoint ScreenToDevice(ScreenPoint screenPos) {
        return {screenPos.x, screenPos.y};
    }

    // DEVICE → SCREEN (identity on macOS/Linux)
    static ScreenPoint DeviceToScreen(DevicePoint devicePos) {
        return {devicePos.x, devicePos.y};
    }

    // SCREEN → DEVICE for macOS (Y-flip: macOS screen has bottom-left origin)
    static DevicePoint ScreenToDeviceMacOS(ScreenPoint screenPos, float screenHeight) {
        return {screenPos.x, screenHeight - screenPos.y};
    }

    // DEVICE → SCREEN for macOS (Y-flip: macOS screen has bottom-left origin)
    static ScreenPoint DeviceToScreenMacOS(DevicePoint devicePos, float screenHeight) {
        return {devicePos.x, screenHeight - devicePos.y};
    }

    // VIEW → DEVICE (for isFlipped=YES NSView)
    // viewY is the Y coordinate after Y-flip: viewHeight - viewPoint.y
    static DevicePoint ViewToDevice(ViewPoint viewPoint, float viewY) {
        return {viewPoint.x, viewY};
    }

    // Y-flip helpers for Linux Cairo (bottom-left origin)
    static DevicePoint CairoToDevice(float cairoX, float cairoY, float screenHeight) {
        return {cairoX, screenHeight - cairoY};
    }

    static Vector2 DeviceToCairo(DevicePoint devicePos, float screenHeight) {
        return {devicePos.x, screenHeight - devicePos.y};
    }
};

// ============================================================
// Item Coordinate Helpers
// ============================================================
// All item positions are in DEVICE coordinates.
// These helpers compute derived positions correctly.

struct ItemCoords {
    // Returns the center of an item in DEVICE coordinates
    static DevicePoint Center(DevicePoint itemPos, float width, float height, float globalScale) {
        float scaledW = width * globalScale;
        float scaledH = height * globalScale;
        return {itemPos.x + scaledW * 0.5f, itemPos.y + scaledH * 0.5f};
    }

    // Returns the half-size of an item in DEVICE coordinates
    static DevicePoint HalfSize(float width, float height, float globalScale) {
        return {width * globalScale * 0.5f, height * globalScale * 0.5f};
    }

    // Returns the full size of an item in DEVICE coordinates
    static DevicePoint Size(float width, float height, float globalScale) {
        return {width * globalScale, height * globalScale};
    }
};

// ============================================================
// Hit Testing
// ============================================================

struct HitTest {
    // Test if a point (in DEVICE coords) hits an item
    // itemPos: center of item in DEVICE coords
    // width, height: item dimensions in world units
    // rotation: rotation in radians
    static bool PointInItem(DevicePoint point, DevicePoint itemPos,
                           float width, float height, float rotation, float globalScale) {
        float dx = point.x - itemPos.x;
        float dy = point.y - itemPos.y;
        float cosA = std::cos(rotation);
        float sinA = std::sin(rotation);
        float lx = dx * cosA - dy * sinA;
        float ly = dx * sinA + dy * cosA;
        float halfW = width * globalScale * 0.5f;
        float halfH = height * globalScale * 0.5f;
        return lx >= -halfW && lx <= halfW && ly >= -halfH && ly <= halfH;
    }

    // Test if a point hits the close button of an item
    // Button is at top-left corner of item
    static bool PointInCloseButton(DevicePoint point, DevicePoint itemPos,
                                   float width, float height, float rotation,
                                   float buttonSize, float globalScale) {
        float dx = point.x - itemPos.x;
        float dy = point.y - itemPos.y;
        float cosA = std::cos(rotation);
        float sinA = std::sin(rotation);
        float lx = dx * cosA - dy * sinA;
        float ly = dx * sinA + dy * cosA;
        float scaledButton = buttonSize; // button size is already in device units
        float halfW = width * globalScale * 0.5f;
        float halfH = height * globalScale * 0.5f;
        float closeX = -halfW;
        float closeY = -halfH;
        return lx >= closeX && lx <= closeX + scaledButton &&
               ly >= closeY && ly <= closeY + scaledButton;
    }
};

// ============================================================
// Screen Bounds
// ============================================================

struct ScreenBounds {
    float minX, minY, maxX, maxY;

    static ScreenBounds FromDimensions(float width, float height) {
        return {0, 0, width, height};
    }

    static ScreenBounds FromMonitor(int monitorX, int monitorY, int monitorW, int monitorH) {
        return {(float)monitorX, (float)monitorY,
                (float)(monitorX + monitorW), (float)(monitorY + monitorH)};
    }

    bool Contains(DevicePoint p, float margin = 0) const {
        return p.x >= minX + margin && p.x <= maxX - margin &&
               p.y >= minY + margin && p.y <= maxY - margin;
    }

    DevicePoint Clamp(DevicePoint p, float snapDistance = 0) const {
        DevicePoint result = p;
        if (result.x < minX) result.x = minX + snapDistance;
        if (result.x > maxX) result.x = maxX - snapDistance;
        if (result.y < minY) result.y = minY + snapDistance;
        if (result.y > maxY) result.y = maxY - snapDistance;
        return result;
    }
};

#endif // COORDINATE_SYSTEM_H
