// CairoRenderer — Cairo implementation of IRenderer
// Used on Linux to provide platform-agnostic rendering to behaviors.

#pragma once

#include "renderer_interface.h"

#ifndef __APPLE__
#include <cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

class CairoRenderer : public IRenderer {
public:
    explicit CairoRenderer(cairo_t* cr) : m_cr(cr) {}

    void SaveState() override {
        cairo_save(m_cr);
    }

    void RestoreState() override {
        cairo_restore(m_cr);
    }

    void Translate(float x, float y) override {
        cairo_translate(m_cr, x, y);
    }

    void Scale(float sx, float sy) override {
        cairo_scale(m_cr, sx, sy);
    }

    void Rotate(float radians) override {
        cairo_rotate(m_cr, radians);
    }

    void DrawEllipse(RenderPoint center, float rx, float ry, RenderColor fill) override {
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, fill.r, fill.g, fill.b, fill.a * m_alpha);
        cairo_translate(m_cr, center.x, center.y);
        cairo_scale(m_cr, rx, ry);
        cairo_arc(m_cr, 0, 0, 1, 0, 2 * M_PI);
        cairo_fill(m_cr);
        cairo_restore(m_cr);
    }

    void DrawEllipseOutline(RenderPoint center, float rx, float ry, RenderColor stroke, float lineWidth) override {
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, stroke.r, stroke.g, stroke.b, stroke.a * m_alpha);
        cairo_set_line_width(m_cr, lineWidth);
        cairo_translate(m_cr, center.x, center.y);
        cairo_scale(m_cr, rx, ry);
        cairo_arc(m_cr, 0, 0, 1, 0, 2 * M_PI);
        cairo_stroke(m_cr);
        cairo_restore(m_cr);
    }

    void DrawLine(RenderPoint a, RenderPoint b, RenderColor color, float lineWidth) override {
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, color.r, color.g, color.b, color.a * m_alpha);
        cairo_set_line_width(m_cr, lineWidth);
        cairo_set_line_cap(m_cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(m_cr, a.x, a.y);
        cairo_line_to(m_cr, b.x, b.y);
        cairo_stroke(m_cr);
        cairo_restore(m_cr);
    }

    void DrawRect(RenderRect rect, RenderColor fill) override {
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, fill.r, fill.g, fill.b, fill.a * m_alpha);
        cairo_rectangle(m_cr, rect.x, rect.y, rect.w, rect.h);
        cairo_fill(m_cr);
        cairo_restore(m_cr);
    }

    void DrawRectOutline(RenderRect rect, RenderColor stroke, float lineWidth) override {
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, stroke.r, stroke.g, stroke.b, stroke.a * m_alpha);
        cairo_set_line_width(m_cr, lineWidth);
        cairo_rectangle(m_cr, rect.x, rect.y, rect.w, rect.h);
        cairo_stroke(m_cr);
        cairo_restore(m_cr);
    }

    void DrawRoundedRect(RenderRect rect, float cornerRadius, RenderColor fill) override {
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, fill.r, fill.g, fill.b, fill.a * m_alpha);
        double radius = cornerRadius;
        double minDim = (rect.w < rect.h ? rect.w : rect.h) * 0.5;
        if (radius > minDim) radius = minDim;
        double x = rect.x, y = rect.y, w = rect.w, h = rect.h;
        cairo_new_sub_path(m_cr);
        cairo_arc(m_cr, x + w - radius, y + radius, radius, -M_PI / 2, 0);
        cairo_arc(m_cr, x + w - radius, y + h - radius, radius, 0, M_PI / 2);
        cairo_arc(m_cr, x + radius, y + h - radius, radius, M_PI / 2, M_PI);
        cairo_arc(m_cr, x + radius, y + radius, radius, M_PI, 3 * M_PI / 2);
        cairo_close_path(m_cr);
        cairo_fill(m_cr);
        cairo_restore(m_cr);
    }

    void DrawPolygon(const RenderPoint* points, int count, RenderColor fill) override {
        if (count < 3) return;
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, fill.r, fill.g, fill.b, fill.a * m_alpha);
        cairo_move_to(m_cr, points[0].x, points[0].y);
        for (int i = 1; i < count; ++i) {
            cairo_line_to(m_cr, points[i].x, points[i].y);
        }
        cairo_close_path(m_cr);
        cairo_fill(m_cr);
        cairo_restore(m_cr);
    }

    void DrawImage(void* image, RenderRect destRect) override {
        cairo_surface_t* surface = static_cast<cairo_surface_t*>(image);
        if (!surface) return;
        cairo_save(m_cr);
        cairo_set_source_surface(m_cr, surface, destRect.x, destRect.y);
        cairo_paint_with_alpha(m_cr, m_alpha);
        cairo_restore(m_cr);
    }

    void DrawText(const char* text, RenderPoint position, RenderColor color, float fontSize) override {
        cairo_save(m_cr);
        cairo_set_source_rgba(m_cr, color.r, color.g, color.b, color.a * m_alpha);

        PangoLayout* layout = pango_cairo_create_layout(m_cr);
        pango_layout_set_text(layout, text, -1);

        PangoFontDescription* desc = pango_font_description_from_string("Sans");
        pango_font_description_set_absolute_size(desc, fontSize * PANGO_SCALE);
        pango_layout_set_font_description(layout, desc);

        cairo_move_to(m_cr, position.x, position.y);
        pango_cairo_update_layout(m_cr, layout);
        pango_cairo_show_layout(m_cr, layout);

        pango_font_description_free(desc);
        g_object_unref(layout);
        cairo_restore(m_cr);
    }

    void SetAlpha(float alpha) override {
        m_alpha = alpha;
    }

private:
    cairo_t* m_cr;
    float m_alpha = 1.0f;
};

#endif // !__APPLE__
