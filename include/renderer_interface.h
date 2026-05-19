#pragma once

// Abstract rendering interface that decouples behaviors from platform-specific APIs.
// macOS implements CGRenderer (CoreGraphics), Linux implements CairoRenderer.
//
// All coordinates are in the current transform space (set by the caller).
// Behaviors should NOT call platform APIs directly — use this interface.

struct RenderColor {
    float r, g, b, a;

    static RenderColor White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static RenderColor Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static RenderColor Red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    static RenderColor Green() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
    static RenderColor Blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
    static RenderColor Yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
    static RenderColor Clear() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
};

struct RenderRect {
    float x, y, w, h;
};

struct RenderPoint {
    float x, y;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // State management
    virtual void SaveState() = 0;
    virtual void RestoreState() = 0;

    // Transforms
    virtual void Translate(float x, float y) = 0;
    virtual void Scale(float sx, float sy) = 0;
    virtual void Rotate(float radians) = 0;

    // Drawing primitives
    virtual void DrawEllipse(RenderPoint center, float rx, float ry, RenderColor fill) = 0;
    virtual void DrawEllipseOutline(RenderPoint center, float rx, float ry, RenderColor stroke, float lineWidth) = 0;
    virtual void DrawLine(RenderPoint a, RenderPoint b, RenderColor color, float lineWidth) = 0;
    virtual void DrawRect(RenderRect rect, RenderColor fill) = 0;
    virtual void DrawRectOutline(RenderRect rect, RenderColor stroke, float lineWidth) = 0;
    virtual void DrawRoundedRect(RenderRect rect, float cornerRadius, RenderColor fill) = 0;
    virtual void DrawPolygon(const RenderPoint* points, int count, RenderColor fill) = 0;

    // Image drawing (platform-specific image handle)
    virtual void DrawImage(void* image, RenderRect destRect) = 0;

    // Image metrics (platform-specific image handle).
    // Returns true and fills width/height if the image is known.
    virtual bool GetImageSize(void* image, float* outWidth, float* outHeight) = 0;

    // Text drawing (platform-specific font handle)
    virtual void DrawText(const char* text, RenderPoint position, RenderColor color, float fontSize) = 0;

    // Measure text width in the current font/size; returns 0 if unsupported.
    virtual float MeasureText(const char* text, float fontSize) = 0;

    // Alpha
    virtual void SetAlpha(float alpha) = 0;

    // Native context access (platform-specific)
    virtual void* nativeContext() const { return nullptr; }
};
