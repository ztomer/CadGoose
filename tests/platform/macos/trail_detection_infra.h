// test_window_trail_detection.mm
// Frame-by-frame trail detector using SCStream (ScreenCaptureKit).
// Captures at display vsync rate (~60fps), detects 1-frame trail artifacts.
//
// Usage:
//   1. Launch CadGoose
//   2. ./build/release/trail_detection_test
//   Exit 0 = no trail, 10 = trail detected

#import <Cocoa/Cocoa.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreMedia/CoreMedia.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <QuartzCore/QuartzCore.h>
#include "command_socket.h"
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <thread>
#include <chrono>
#include <sstream>
#include <cstring>
#include <sys/stat.h>

// ============================================================
// Frame capture (SCStream-based, vsync rate)
// ============================================================
//
// Included by (and private to) test_window_trail_detection.mm — this
// program is a single translation unit, so the capture/analysis
// infrastructure lives here as internally-linked definitions.

struct CaptureResult {
    std::vector<uint8_t> pixels;  // BGRA, row-major
    int width = 0;
    int height = 0;
    double captureMs = 0;
};

static double GetNowMs() {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static bool SaveFramePNG(const uint8_t* pixels, int w, int h,
                          const char* path) {
    if (!pixels || !w || !h) return false;

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        (void*)pixels, w, h, 8, w * 4, cs,
        (CGBitmapInfo)kCGImageAlphaPremultipliedFirst |
            kCGBitmapByteOrder32Little);
    CGColorSpaceRelease(cs);
    if (!ctx) return false;

    CGImageRef cgImage = CGBitmapContextCreateImage(ctx);
    CGContextRelease(ctx);
    if (!cgImage) return false;

    NSString* nsPath = [NSString stringWithUTF8String:path];
    NSURL* url = [NSURL fileURLWithPath:nsPath];
    CGImageDestinationRef dest = CGImageDestinationCreateWithURL(
        (__bridge CFURLRef)url, (__bridge CFStringRef)@"public.png", 1, NULL);
    if (!dest) { CGImageRelease(cgImage); return false; }

    CGImageDestinationAddImage(dest, cgImage, NULL);
    bool ok = CGImageDestinationFinalize(dest);
    CFRelease(dest);
    CGImageRelease(cgImage);
    return ok;
}

// ============================================================
// Socket helpers
// ============================================================

static std::string SendCmd(const std::string& cmd) {
    std::vector<std::string> args;
    std::istringstream iss(cmd);
    std::string token;
    while (iss >> token) args.push_back(token);
    if (args.empty()) return "";
    std::string response, error;
    CommandSocket_Send(args, &response, &error);
    return response;
}

static std::map<std::string, std::string> ParseStatus(const std::string& s) {
    std::map<std::string, std::string> kv;
    std::istringstream iss(s);
    std::string line;
    while (std::getline(iss, line)) {
        size_t eq = line.find('=');
        if (eq != std::string::npos)
            kv[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return kv;
}

// ============================================================
// SCStream capture manager
// ============================================================

static const int kRingSize = 300; // ~5 seconds at 60fps
static CaptureResult s_ring[kRingSize];
static int s_ringPos = 0;
static int s_captureCount = 0;
static dispatch_semaphore_t s_captureDone = nil;
static dispatch_queue_t s_captureQueue = nil;

@interface TrailCaptureDelegate : NSObject <SCStreamOutput>
@property (atomic) BOOL shouldStop;
@end

@implementation TrailCaptureDelegate

- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sb
         ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeScreen) return;

    @autoreleasepool {
        CVPixelBufferRef pb = CMSampleBufferGetImageBuffer(sb);
        if (!pb) return;

        CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);

        int slot = s_ringPos;
        CaptureResult& frame = s_ring[slot];
        frame.width = (int)CVPixelBufferGetWidth(pb);
        frame.height = (int)CVPixelBufferGetHeight(pb);
        frame.captureMs = GetNowMs();

        size_t bpr = CVPixelBufferGetBytesPerRow(pb);
        uint8_t* src = (uint8_t*)CVPixelBufferGetBaseAddress(pb);
        frame.pixels.resize(frame.height * frame.width * 4);

        for (int y = 0; y < frame.height; ++y) {
            memcpy(&frame.pixels[y * frame.width * 4],
                   src + y * bpr, frame.width * 4);
        }

        CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
    }

    s_ringPos = (s_ringPos + 1) % kRingSize;
    s_captureCount++;

    if (self.shouldStop && s_captureDone) {
        dispatch_semaphore_signal(s_captureDone);
    }
}

@end

static bool StartSCStream(SCStream** outStream, TrailCaptureDelegate** outDelegate) {
    // Get shareable content (displays) via async API
    __block SCShareableContent* content = nil;
    dispatch_semaphore_t contentReady = dispatch_semaphore_create(0);

    [SCShareableContent getShareableContentWithCompletionHandler:
        ^(SCShareableContent* _Nullable sc, NSError* _Nullable error) {
            content = sc;
            dispatch_semaphore_signal(contentReady);
        }];
    dispatch_semaphore_wait(contentReady,
        dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
    if (!content || content.displays.count == 0) return false;

    SCDisplay* display = content.displays.firstObject;

    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
    config.width = display.width * 2; // points to pixels (2x retina)
    config.height = display.height * 2;
    config.pixelFormat = kCVPixelFormatType_32BGRA;
    config.showsCursor = NO;
    config.capturesAudio = NO;
    config.minimumFrameInterval = CMTimeMake(1, 60);

    // Exclude our own windows
    NSMutableArray* excludeWindows = [NSMutableArray array];
    for (SCWindow* win in content.windows) {
        if ([win.owningApplication.applicationName
                isEqualToString:@"trail_detection_test"]) {
            [excludeWindows addObject:win];
        }
    }

    SCContentFilter* filter = [[SCContentFilter alloc]
        initWithDisplay:display excludingWindows:excludeWindows];

    TrailCaptureDelegate* delegate = [[TrailCaptureDelegate alloc] init];
    delegate.shouldStop = NO;

    SCStream* stream = [[SCStream alloc] initWithFilter:filter
                                         configuration:config
                                              delegate:nil];

    s_captureQueue = dispatch_queue_create("capture", DISPATCH_QUEUE_SERIAL);
    NSError* addErr = nil;
    if (![stream addStreamOutput:delegate
                            type:SCStreamOutputTypeScreen
              sampleHandlerQueue:s_captureQueue
                          error:&addErr]) {
        fprintf(stderr, "  addStreamOutput error: %s\n",
                addErr.localizedDescription.UTF8String);
        return false;
    }

    dispatch_semaphore_t ready = dispatch_semaphore_create(0);
    [stream startCaptureWithCompletionHandler:^(NSError* error) {
        if (error) {
            fprintf(stderr, "  SCStream start error: %s\n",
                    error.localizedDescription.UTF8String);
        }
        dispatch_semaphore_signal(ready);
    }];
    dispatch_semaphore_wait(ready,
        dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));

    *outStream = stream;
    *outDelegate = delegate;
    return true;
}

static void StopSCStream(SCStream* stream) {
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    [stream stopCaptureWithCompletionHandler:^(NSError* error) {
        if (error) {
            fprintf(stderr, "  SCStream stop error: %s\n",
                    error.localizedDescription.UTF8String);
        }
        dispatch_semaphore_signal(done);
    }];
    dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
}

// ============================================================
// Frame delta & trail analysis
// ============================================================

static float PixelDelta(const uint8_t* a, const uint8_t* b) {
    float dr = (float)a[1] - (float)b[1];
    float dg = (float)a[2] - (float)b[2];
    float db = (float)a[3] - (float)b[3];
    return dr*dr + dg*dg + db*db;
}

// Build a trail mask: non-zero = suspected trail pixel
// Trail = single-frame blink: changes from baseline, resolves next frame, then stable
static std::vector<uint8_t> BuildTrailMask(
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

static int AnalyzeTrailMask(const std::vector<uint8_t>& mask,
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

static FrameDelta ComputeDelta(const CaptureResult& a,
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

// ============================================================
// Main
// ============================================================
