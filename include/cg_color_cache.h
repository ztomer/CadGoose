#pragma once
#import <CoreGraphics/CoreGraphics.h>
#include <cstdint>

// Cached CGColor setters — avoid per-draw CGColorCreate churn.
//
// CGContextSetRGB{Fill,Stroke}Color allocates a fresh CGColor on every call
// (colorspace + HDR content-headroom setup), and CoreGraphics then deep-compares
// those colors while coalescing its display list (CGColorCompare /
// CGColorGetContentHeadroom showed up hot in the time profiler). Reusing cached
// CGColorRefs keyed by 8-bit RGBA turns each color change into a small array
// scan plus a pointer-identity hit in the display list.
//
// The cache is thread_local, so the per-frame drawing path needs no locking; any
// thread that draws gets its own table. The CGColorSpace is immutable and shared.

namespace cgcolorcache {

inline CGColorSpaceRef DeviceRGB() {
    static CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    return cs;
}

inline uint32_t PackRGBA(float r, float g, float b, float a) {
    auto q = [](float v) -> uint32_t {
        if (v <= 0.0f) return 0;
        if (v >= 1.0f) return 255;
        return static_cast<uint32_t>(v * 255.0f + 0.5f);
    };
    return (q(r) << 24) | (q(g) << 16) | (q(b) << 8) | q(a);
}

// Returns a cached CGColorRef (owned by the cache; do NOT release) for the given
// 8-bit-quantized color. Quantization is imperceptible on an 8-bit display and
// keeps continuously-varying values (e.g. alpha fades) from thrashing the cache.
inline CGColorRef Lookup(float r, float g, float b, float a) {
    static constexpr int kCap = 64;
    struct Entry { uint32_t key; CGColorRef color; };
    thread_local Entry s_entries[kCap] = {};
    thread_local int s_count = 0;
    thread_local int s_next = 0;  // ring cursor for eviction once full

    const uint32_t key = PackRGBA(r, g, b, a);
    for (int i = 0; i < s_count; i++) {
        if (s_entries[i].key == key) return s_entries[i].color;
    }

    const CGFloat comps[4] = {
        static_cast<CGFloat>((key >> 24) & 0xff) / 255.0,
        static_cast<CGFloat>((key >> 16) & 0xff) / 255.0,
        static_cast<CGFloat>((key >> 8) & 0xff) / 255.0,
        static_cast<CGFloat>(key & 0xff) / 255.0,
    };
    CGColorRef c = CGColorCreate(DeviceRGB(), comps);

    int slot;
    if (s_count < kCap) {
        slot = s_count++;
    } else {
        slot = s_next;
        s_next = (s_next + 1) % kCap;
        if (s_entries[slot].color) CGColorRelease(s_entries[slot].color);
    }
    s_entries[slot].key = key;
    s_entries[slot].color = c;  // cache owns this reference for the thread's lifetime
    return c;
}

}  // namespace cgcolorcache

inline void CGCtx_SetFillColor(CGContextRef ctx, float r, float g, float b, float a) {
    CGContextSetFillColorWithColor(ctx, cgcolorcache::Lookup(r, g, b, a));
}

inline void CGCtx_SetStrokeColor(CGContextRef ctx, float r, float g, float b, float a) {
    CGContextSetStrokeColorWithColor(ctx, cgcolorcache::Lookup(r, g, b, a));
}
