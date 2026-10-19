// trail_analysis.h — the pixel analysis behind trail_detection_test, as pure
// C++ with no capture or AppKit dependency, so the detector itself is tested
// headless (tests/common/test_trail_analysis.cpp) and the live binary and the
// tests run the SAME code (ROADMAP M1.3).
//
// A "trail" is a single-frame blink around a drop: pixels that change from
// the baseline A into the drop frame B, change again into C, and are then
// stable C -> D. A dropped item that appears and STAYS (changes A -> B, then
// stable) is not a trail, and neither is anything that was already on screen.
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

struct CaptureResult {
    std::vector<uint8_t> pixels;  // BGRA, row-major
    int width = 0;
    int height = 0;
    double captureMs = 0;
};


// Squared colour distance of two BGRA pixels: bytes 0-2 (B, G, R). Alpha (byte
// 3) is not colour; until 2026-10-18 this read bytes 1-3, so it ignored blue
// and counted alpha, and a blue-only trail was invisible to the detector.
inline float PixelDelta(const uint8_t* a, const uint8_t* b) {
    float db = (float)a[0] - (float)b[0];
    float dg = (float)a[1] - (float)b[1];
    float dr = (float)a[2] - (float)b[2];
    return dr*dr + dg*dg + db*db;
}

// Build a trail mask: non-zero = suspected trail pixel
// Trail = single-frame blink: changes from baseline, resolves next frame, then stable
inline std::vector<uint8_t> BuildTrailMask(
    const CaptureResult& A,   // baseline (before drop)
    const CaptureResult& B,   // drop frame (trail visible here)
    const CaptureResult& C,   // post-drop + 1 (trail resolves)
    const CaptureResult& D,   // post-drop + 2 (stable)
    float itemX, float itemY, // drop position
    float stableThresh = 20.0f,
    float changeThresh = 80.0f,
    float trailRadius = 100.0f
) {
    int w = A.width;
    int h = A.height;
    std::vector<uint8_t> mask((size_t)w * h, 0);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (itemX >= 0) {
                float dx = (float)x - itemX;
                float dy = (float)y - itemY;
                if (dx*dx + dy*dy > trailRadius*trailRadius) continue;
            }

            size_t idx = (size_t)(y * w + x) * 4;

            float dAB = PixelDelta(&A.pixels[idx], &B.pixels[idx]);
            float dBC = PixelDelta(&B.pixels[idx], &C.pixels[idx]);
            float dCD = PixelDelta(&C.pixels[idx], &D.pixels[idx]);

            // Single-frame blink: appears in B, gone by C, stable C→D
            if (dAB > changeThresh && dBC > changeThresh && dCD < stableThresh) {
                mask[(size_t)(y * w + x)] = 255;
            }
        }
    }
    return mask;
}

inline int AnalyzeTrailMask(const std::vector<uint8_t>& mask,
                             int w, int h,
                             int* outClusterCount = nullptr,
                             int* outPixelCount = nullptr,
                             int* outPeakClusterSize = nullptr) {
    int total = 0;
    for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i] > 0) total++;
    }
    if (outPixelCount) *outPixelCount = total;
    if (outClusterCount) *outClusterCount = 0;
    if (outPeakClusterSize) *outPeakClusterSize = 0;
    if (total == 0) return total;

    std::vector<int> labels(mask.size(), 0);
    int nextLabel = 1;

    auto getLabel = [&](int x, int y) -> int {
        if (x < 0 || x >= w || y < 0 || y >= h) return 0;
        return labels[(size_t)(y * w + x)];
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t idx = (size_t)(y * w + x);
            if (mask[idx] == 0) continue;

            int leftL = getLabel(x-1, y);
            int topL  = getLabel(x, y-1);

            if (leftL == 0 && topL == 0) {
                labels[idx] = nextLabel++;
            } else if (leftL != 0 && topL == 0) {
                labels[idx] = leftL;
            } else if (leftL == 0 && topL != 0) {
                labels[idx] = topL;
            } else if (leftL == topL) {
                labels[idx] = leftL;
            } else {
                int minL = std::min(leftL, topL);
                int maxL = std::max(leftL, topL);
                labels[idx] = minL;
                for (size_t j = 0; j < labels.size(); ++j) {
                    if (labels[j] == maxL) labels[j] = minL;
                }
            }
        }
    }

    std::map<int, int> sizes;
    for (size_t i = 0; i < labels.size(); ++i) {
        if (labels[i] > 0) sizes[labels[i]]++;
    }

    int clusters = 0;
    int peak = 0;
    int largeClusters = 0;
    for (auto& kv : sizes) {
        if (kv.second > 0) clusters++;
        if (kv.second > peak) peak = kv.second;
        if (kv.second > 10) largeClusters++;
    }

    if (outClusterCount) *outClusterCount = largeClusters;
    if (outPeakClusterSize) *outPeakClusterSize = peak;
    return total;
}

struct FrameDelta {
    int changedPixels = 0;
    float meanDelta = 0;
    float maxDelta = 0;
    double timeMs = 0;
};

inline FrameDelta ComputeDelta(const CaptureResult& a,
                                const CaptureResult& b) {
    FrameDelta d;
    int w = std::min(a.width, b.width);
    int h = std::min(a.height, b.height);
    if (w < 1 || h < 1) return d;

    double sum = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t idx = (size_t)(y * w + x) * 4;
            float pd = PixelDelta(&a.pixels[idx], &b.pixels[idx]);
            if (pd > 5.0f) {
                d.changedPixels++;
                sum += pd;
            }
            if (pd > d.maxDelta) d.maxDelta = pd;
        }
    }
    if (d.changedPixels > 0)
        d.meanDelta = sum / d.changedPixels;
    return d;
}

// The whole step-5 analysis the live binary runs over its captured frames.
struct TrailReport {
    int dropFrame = 1;          // frame with the largest delta in the 2nd half
    int dropChangedPixels = 0;
    float itemX = -1, itemY = -1;  // centroid of the drop's change, or -1
    std::vector<uint8_t> mask;     // trail pixels of frame B (empty if not run)
    int trailPixels = 0;
    int trailClusters = 0;         // clusters larger than 10 pixels
    int peakCluster = 0;
    bool analyzed = false;         // enough frames around the drop to judge
    bool trailDetected = false;
};

inline TrailReport AnalyzeTrailFrames(const std::vector<CaptureResult>& frames) {
    TrailReport r;
    const int nFrames = static_cast<int>(frames.size());

    // The drop frame: the largest frame-to-frame delta, searched only in the
    // second half of the buffer (the drop happens after the trigger).
    int searchStart = std::max(2, nFrames / 2);
    for (int i = searchStart; i < nFrames; ++i) {
        FrameDelta d = ComputeDelta(frames[i - 1], frames[i]);
        if (d.changedPixels > r.dropChangedPixels) {
            r.dropChangedPixels = d.changedPixels;
            r.dropFrame = i;
        }
    }
    if (r.dropFrame < 2 || r.dropFrame + 3 > nFrames) return r;

    const CaptureResult& A = frames[r.dropFrame - 2];
    const CaptureResult& prev = frames[r.dropFrame - 1];
    const CaptureResult& B = frames[r.dropFrame];
    const CaptureResult& C = frames[r.dropFrame + 1];
    const CaptureResult& D = frames[r.dropFrame + 2];

    // Item position: centroid of the strongly changed pixels at the drop.
    if (ComputeDelta(prev, B).changedPixels > 100) {
        double cx = 0, cy = 0;
        int count = 0;
        for (int y = 0; y < B.height; y += 4) {
            for (int x = 0; x < B.width; x += 4) {
                size_t idx = (size_t)(y * B.width + x) * 4;
                if (PixelDelta(&prev.pixels[idx], &B.pixels[idx]) > 500.0f) {
                    cx += x; cy += y;
                    count++;
                }
            }
        }
        if (count > 10) {
            r.itemX = (float)(cx / count);
            r.itemY = (float)(cy / count);
        }
    }

    r.mask = BuildTrailMask(A, B, C, D, r.itemX, r.itemY);
    r.trailPixels = AnalyzeTrailMask(r.mask, B.width, B.height,
                                     &r.trailClusters, nullptr, &r.peakCluster);
    r.analyzed = true;
    r.trailDetected = (r.trailPixels > 20 && r.trailClusters > 0);
    return r;
}
