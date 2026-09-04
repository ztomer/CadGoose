// CGRenderer — CoreGraphics implementation of IRenderer
// Used on macOS to provide platform-agnostic rendering to behaviors.

#pragma once

#include "renderer_interface.h"

#ifdef __APPLE__
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#include "cg_color_cache.h"

class CGRenderer : public IRenderer {
public:
    explicit CGRenderer(CGContextRef ctx) : m_ctx(ctx) {}

    void SaveState() override {
        CGContextSaveGState(m_ctx);
    }

    void RestoreState() override {
        CGContextRestoreGState(m_ctx);
    }

    void Translate(float x, float y) override {
        CGContextTranslateCTM(m_ctx, x, y);
    }

    void Scale(float sx, float sy) override {
        CGContextScaleCTM(m_ctx, sx, sy);
    }

    void Rotate(float radians) override {
        CGContextRotateCTM(m_ctx, radians);
    }

    void DrawEllipse(RenderPoint center, float rx, float ry, RenderColor fill) override {
        CGCtx_SetFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);
        CGContextFillEllipseInRect(m_ctx, CGRectMake(center.x - rx, center.y - ry, rx * 2, ry * 2));
    }

    void DrawEllipseOutline(RenderPoint center, float rx, float ry, RenderColor stroke, float lineWidth) override {
        CGCtx_SetStrokeColor(m_ctx, stroke.r, stroke.g, stroke.b, stroke.a);
        CGContextSetLineWidth(m_ctx, lineWidth);
        CGContextStrokeEllipseInRect(m_ctx, CGRectMake(center.x - rx, center.y - ry, rx * 2, ry * 2));
    }

    void DrawLine(RenderPoint a, RenderPoint b, RenderColor color, float lineWidth) override {
        CGCtx_SetStrokeColor(m_ctx, color.r, color.g, color.b, color.a);
        CGContextSetLineWidth(m_ctx, lineWidth);
        CGContextSetLineCap(m_ctx, kCGLineCapRound);
        CGContextMoveToPoint(m_ctx, a.x, a.y);
        CGContextAddLineToPoint(m_ctx, b.x, b.y);
        CGContextStrokePath(m_ctx);
    }

    void DrawRect(RenderRect rect, RenderColor fill) override {
        CGCtx_SetFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);
        CGContextFillRect(m_ctx, CGRectMake(rect.x, rect.y, rect.w, rect.h));
    }

    void DrawRectOutline(RenderRect rect, RenderColor stroke, float lineWidth) override {
        CGCtx_SetStrokeColor(m_ctx, stroke.r, stroke.g, stroke.b, stroke.a);
        CGContextSetLineWidth(m_ctx, lineWidth);
        CGContextStrokeRect(m_ctx, CGRectMake(rect.x, rect.y, rect.w, rect.h));
    }

    void DrawRoundedRect(RenderRect rect, float cornerRadius, RenderColor fill) override {
        // H3: Use CGPathCreateWithRoundedRect (1 API call, hardware-optimized) and
        // cache the path when rect+radius haven't changed, avoiding per-frame
        // CGMutablePathRef alloc + 8 arc operations.
        CGCtx_SetFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);

        // Cache key: 5 floats packed as bit patterns for exact equality.
        struct RRKey {
            float x, y, w, h, r;
            bool operator==(const RRKey& o) const {
                return x==o.x && y==o.y && w==o.w && h==o.h && r==o.r;
            }
        };
        thread_local RRKey   s_lastKey{};
        thread_local CGPathRef s_cachedPath = nullptr;

        CGFloat radius = cornerRadius;
        CGFloat minDim = (rect.w < rect.h ? rect.w : rect.h) * 0.5f;
        if (radius > minDim) radius = minDim;

        RRKey key{ rect.x, rect.y, rect.w, rect.h, (float)radius };
        if (!s_cachedPath || !(key == s_lastKey)) {
            if (s_cachedPath) CGPathRelease(s_cachedPath);
            CGRect r = CGRectMake(rect.x, rect.y, rect.w, rect.h);
            s_cachedPath = CGPathCreateWithRoundedRect(r, radius, radius, nullptr);
            s_lastKey = key;
        }

        CGContextAddPath(m_ctx, s_cachedPath);
        CGContextFillPath(m_ctx);
    }

    void DrawPolygon(const RenderPoint* points, int count, RenderColor fill) override {
        if (count < 3) return;
        CGCtx_SetFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);
        CGContextBeginPath(m_ctx);
        CGContextMoveToPoint(m_ctx, points[0].x, points[0].y);
        for (int i = 1; i < count; ++i) {
            CGContextAddLineToPoint(m_ctx, points[i].x, points[i].y);
        }
        CGContextClosePath(m_ctx);
        CGContextFillPath(m_ctx);
    }

    void DrawImage(void* image, RenderRect destRect) override {
        CGImageRef img = static_cast<CGImageRef>(image);
        if (!img) return;
        CGContextDrawImage(m_ctx, CGRectMake(destRect.x, destRect.y, destRect.w, destRect.h), img);
    }

    bool GetImageSize(void* image, float* outWidth, float* outHeight) override {
        CGImageRef img = static_cast<CGImageRef>(image);
        if (!img) return false;
        if (outWidth)  *outWidth  = (float)CGImageGetWidth(img);
        if (outHeight) *outHeight = (float)CGImageGetHeight(img);
        return true;
    }

    void DrawText(const char* text, RenderPoint position, RenderColor color, float fontSize) override {
        // H2: CTFontCreateWithName is ~100µs. Cache fonts keyed by quantized size
        // in a thread-local map so repeated calls with the same fontSize are free.
        CTFontRef font = GetCachedFont(fontSize);
        if (!font) return;

        CGColorRef cgColor = CGColorCreateGenericRGB(color.r, color.g, color.b, color.a);
        CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
        CFTypeRef values[] = { font, cgColor };
        CFDictionaryRef attrs = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        CFStringRef string = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
        CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attrs);
        CTLineRef line = CTLineCreateWithAttributedString(attrStr);

        if (line) {
            CGAffineTransform savedTextMatrix = CGContextGetTextMatrix(m_ctx);
            CGContextSetTextMatrix(m_ctx, CGAffineTransformMakeScale(1.0, -1.0));
            CGContextSetTextPosition(m_ctx, position.x, position.y);
            CTLineDraw(line, m_ctx);
            CGContextSetTextMatrix(m_ctx, savedTextMatrix);
            CFRelease(line);
        }

        CFRelease(attrStr);
        CFRelease(string);
        CFRelease(attrs);
        CGColorRelease(cgColor);
        // H2: font is owned by cache — do NOT CFRelease here.
    }

    void SetAlpha(float alpha) override {
        CGContextSetAlpha(m_ctx, alpha);
    }

    float MeasureText(const char* text, float fontSize) override {
        // H2: Same font cache as DrawText.
        if (!text || !*text) return 0.0f;
        CTFontRef font = GetCachedFont(fontSize);
        if (!font) return 0.0f;
        CFStringRef string = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
        CFTypeRef keys[] = { kCTFontAttributeName };
        CFTypeRef values[] = { font };
        CFDictionaryRef attrs = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 1,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attrs);
        CTLineRef line = CTLineCreateWithAttributedString(attrStr);
        double width = 0;
        if (line) {
            CFIndex len = CTLineGetStringRange(line).length;
            width = CTLineGetOffsetForStringIndex(line, len, NULL);
            CFRelease(line);
        }
        CFRelease(attrStr);
        CFRelease(string);
        CFRelease(attrs);
        // H2: font is owned by cache — do NOT CFRelease here.
        return (float)width;
    }

    void* nativeContext() const override {
        return (void*)m_ctx;
    }

private:
    CGContextRef m_ctx;

    // H2: Thread-local CTFont cache keyed by quantized fontSize.
    // CTFontCreateWithName is ~100µs; reuse fonts across frames.
    static CTFontRef GetCachedFont(float fontSize) {
        // Quantize to 0.5pt granularity to collapse near-identical sizes.
        float quantized = std::round(fontSize * 2.0f) * 0.5f;

        static constexpr int kMaxFonts = 16;
        struct FontEntry { float size; CTFontRef font; };
        thread_local FontEntry s_cache[kMaxFonts] = {};
        thread_local int s_count = 0;

        for (int i = 0; i < s_count; ++i) {
            if (s_cache[i].size == quantized) return s_cache[i].font;
        }

        CTFontRef f = CTFontCreateWithName(CFSTR("Helvetica"), quantized, NULL);
        if (!f) return nullptr;

        if (s_count < kMaxFonts) {
            s_cache[s_count++] = { quantized, f };
        } else {
            // Cache full: evict slot 0 (LRU-0 approximation; fonts rarely change)
            CFRelease(s_cache[0].font);
            s_cache[0] = { quantized, f };
        }
        return f;
    }
};

#endif // __APPLE__
