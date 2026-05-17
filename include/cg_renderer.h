// CGRenderer — CoreGraphics implementation of IRenderer
// Used on macOS to provide platform-agnostic rendering to behaviors.

#pragma once

#include "renderer_interface.h"

#ifdef __APPLE__
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>

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
        CGContextSetRGBFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);
        CGContextFillEllipseInRect(m_ctx, CGRectMake(center.x - rx, center.y - ry, rx * 2, ry * 2));
    }

    void DrawEllipseOutline(RenderPoint center, float rx, float ry, RenderColor stroke, float lineWidth) override {
        CGContextSetRGBStrokeColor(m_ctx, stroke.r, stroke.g, stroke.b, stroke.a);
        CGContextSetLineWidth(m_ctx, lineWidth);
        CGContextStrokeEllipseInRect(m_ctx, CGRectMake(center.x - rx, center.y - ry, rx * 2, ry * 2));
    }

    void DrawLine(RenderPoint a, RenderPoint b, RenderColor color, float lineWidth) override {
        CGContextSetRGBStrokeColor(m_ctx, color.r, color.g, color.b, color.a);
        CGContextSetLineWidth(m_ctx, lineWidth);
        CGContextSetLineCap(m_ctx, kCGLineCapRound);
        CGContextMoveToPoint(m_ctx, a.x, a.y);
        CGContextAddLineToPoint(m_ctx, b.x, b.y);
        CGContextStrokePath(m_ctx);
    }

    void DrawRect(RenderRect rect, RenderColor fill) override {
        CGContextSetRGBFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);
        CGContextFillRect(m_ctx, CGRectMake(rect.x, rect.y, rect.w, rect.h));
    }

    void DrawRectOutline(RenderRect rect, RenderColor stroke, float lineWidth) override {
        CGContextSetRGBStrokeColor(m_ctx, stroke.r, stroke.g, stroke.b, stroke.a);
        CGContextSetLineWidth(m_ctx, lineWidth);
        CGContextStrokeRect(m_ctx, CGRectMake(rect.x, rect.y, rect.w, rect.h));
    }

    void DrawRoundedRect(RenderRect rect, float cornerRadius, RenderColor fill) override {
        CGContextSetRGBFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);
        CGMutablePathRef path = CGPathCreateMutable();
        CGRect r = CGRectMake(rect.x, rect.y, rect.w, rect.h);
        CGFloat radius = cornerRadius;
        // Clamp radius to half the smallest dimension
        CGFloat minDim = (rect.w < rect.h ? rect.w : rect.h) * 0.5f;
        if (radius > minDim) radius = minDim;
        // Build rounded rect path using explicit arc segments
        CGFloat x = r.origin.x;
        CGFloat y = r.origin.y;
        CGFloat w = r.size.width;
        CGFloat h = r.size.height;
        CGPathMoveToPoint(path, NULL, x + radius, y);
        CGPathAddLineToPoint(path, NULL, x + w - radius, y);
        CGPathAddArc(path, NULL, x + w - radius, y + radius, radius, -M_PI_2, 0, false);
        CGPathAddLineToPoint(path, NULL, x + w, y + h - radius);
        CGPathAddArc(path, NULL, x + w - radius, y + h - radius, radius, 0, M_PI_2, false);
        CGPathAddLineToPoint(path, NULL, x + radius, y + h);
        CGPathAddArc(path, NULL, x + radius, y + h - radius, radius, M_PI_2, M_PI, false);
        CGPathAddLineToPoint(path, NULL, x, y + radius);
        CGPathAddArc(path, NULL, x + radius, y + radius, radius, M_PI, 3 * M_PI_2, false);
        CGPathCloseSubpath(path);
        CGContextAddPath(m_ctx, path);
        CGContextFillPath(m_ctx);
        CGPathRelease(path);
    }

    void DrawPolygon(const RenderPoint* points, int count, RenderColor fill) override {
        if (count < 3) return;
        CGContextSetRGBFillColor(m_ctx, fill.r, fill.g, fill.b, fill.a);
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

    void DrawText(const char* text, RenderPoint position, RenderColor color, float fontSize) override {
        CTFontRef font = CTFontCreateWithName(CFSTR("Helvetica"), fontSize, NULL);
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
            CGContextSetTextPosition(m_ctx, position.x, position.y);
            CTLineDraw(line, m_ctx);
            CFRelease(line);
        }

        CFRelease(attrStr);
        CFRelease(string);
        CFRelease(attrs);
        CGColorRelease(cgColor);
        CFRelease(font);
    }

    void SetAlpha(float alpha) override {
        CGContextSetAlpha(m_ctx, alpha);
    }

private:
    CGContextRef m_ctx;
};

#endif // __APPLE__
