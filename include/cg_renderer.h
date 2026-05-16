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
