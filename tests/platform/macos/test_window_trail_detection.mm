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

int main() {
    fprintf(stderr, "==================================================\n");
    fprintf(stderr, "  Window Trail Detection Test v3 (SCStream)\n");
    fprintf(stderr, "==================================================\n");

    mkdir("/tmp/trail_test_frames", 0755);

    // ---- Step 1: Connect ----
    fprintf(stderr, "\n[1/5] Connecting to CadGoose...\n");
    std::string resp = SendCmd("status");
    if (resp.empty() || resp.find("running=1") == std::string::npos) {
        fprintf(stderr, "  FAIL: CadGoose not reachable via socket.\n");
        return 1;
    }
    fprintf(stderr, "  Connected.\n");

    auto kv = ParseStatus(resp);
    if (kv["goose_count"] == "0") {
        SendCmd("spawn");
        fprintf(stderr, "  Spawned goose.\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // ---- Step 2: Disable non-fetch behaviors ----
    static const char* kDisable[] = {
        "ball", "breadcrumbs", "anger", "health", "jail", "portal",
        "pomodoro", "presence", "rainbow", "toys", "interactive_drops",
        "hats", "boredom", "peeking", "acid", "nametag", "honcker",
        nullptr
    };
    fprintf(stderr, "\n[2/5] Disabling non-fetch behaviors...\n");
    for (int i = 0; kDisable[i]; ++i) {
        std::string r = SendCmd(std::string("disable ") + kDisable[i]);
    }
    fprintf(stderr, "  Done.\n");

    // ---- Step 3: Start SCStream capture ----
    fprintf(stderr, "\n[3/5] Starting SCStream capture...\n");

    SCStream* stream = nil;
    TrailCaptureDelegate* delegate = nil;
    if (!StartSCStream(&stream, &delegate)) {
        fprintf(stderr, "  FAIL: Could not start SCStream.\n");
        fprintf(stderr, "  Ensure Terminal has Screen Recording permission.\n");
        return 2;
    }
    fprintf(stderr, "  SCStream running (capturing at vsync rate).\n");

    // Wait for warm-up
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ---- Step 4: Trigger fetch and wait for drop ----
    fprintf(stderr, "\n[4/5] Triggering fetch...\n");
    resp = SendCmd("fetch\ttext");
    fprintf(stderr, "  => %s", resp.c_str());

    // Poll status until drop detected (fast polling, ~2ms)
    fprintf(stderr, "  Waiting for drop...\n");
    bool dropped = false;
    double dropTime = 0;
    int totalPolls = 0;

    for (int iter = 0; iter < 6000; ++iter) {
        resp = SendCmd("status");
        auto s = ParseStatus(resp);
        totalPolls++;

        if (!dropped &&
            s["goose_state"] == "wander" &&
            s["goose_heldItem"] == "no" &&
            s["dropped_items"] != "0") {
            dropped = true;
            dropTime = GetNowMs();
            fprintf(stderr, "  -> Drop detected at poll %d!\n", iter);
        }

        if (dropped) {
            // Wait for ~30 more vsync frames (~500ms) for post-drop data
            if (GetNowMs() - dropTime > 500.0) {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    if (!dropped) {
        fprintf(stderr, "\n  FAIL: No drop detected within 6000 polls.\n");
        StopSCStream(stream);
        return 1;
    }

    // Stop capture
    fprintf(stderr, "  Stopping capture...\n");
    delegate.shouldStop = YES;
    StopSCStream(stream);

    int nFrames = std::min(s_captureCount, kRingSize);
    fprintf(stderr, "  Captured %d frames (%d in ring buffer).\n",
            s_captureCount, nFrames);

    // ---- Step 5: Analyze frames ----
    fprintf(stderr, "\n[5/5] Analyzing frames...\n");

    // Linearize ring buffer
    std::vector<CaptureResult> frames(nFrames);
    int startIdx = (s_ringPos - nFrames + kRingSize) % kRingSize;
    for (int i = 0; i < nFrames; ++i) {
        int idx = (startIdx + i) % kRingSize;
        frames[i] = s_ring[idx];
    }

    // Find the drop frame: look for the largest frame-to-frame delta
    // within the last ~30 frames (the drop should create a big pixel change
    // as the ItemWindow appears with its dropped item content).
    int dropFrame = 1;
    FrameDelta maxDelta;
    maxDelta.changedPixels = 0;

    // Only search the second half of the buffer (drop happens after trigger)
    int searchStart = nFrames / 2;
    if (searchStart < 2) searchStart = 2;

    for (int i = searchStart; i < nFrames; ++i) {
        FrameDelta d = ComputeDelta(frames[i-1], frames[i]);
        if (d.changedPixels > maxDelta.changedPixels) {
            maxDelta = d;
            dropFrame = i;
        }
    }

    fprintf(stderr, "  Drop frame (max delta): %d (%d changed pixels)\n",
            dropFrame, maxDelta.changedPixels);

    // Save all frames
    int saveFrom = std::max(0, dropFrame - 10);
    int saveTo = std::min(nFrames, dropFrame + 10);
    for (int i = saveFrom; i < saveTo; ++i) {
        char path[256];
        const char* label = (i == dropFrame) ? "DROP" :
                            (i < dropFrame) ? "pre" : "post";
        snprintf(path, sizeof(path), "/tmp/trail_test_frames/%02d_%s.png",
                 i - dropFrame + 10, label);
        SaveFramePNG(frames[i].pixels.data(), frames[i].width,
                     frames[i].height, path);
    }

    // Print frame deltas around drop
    fprintf(stderr, "  Frame deltas around drop:\n");
    int deltaFrom = std::max(1, dropFrame - 5);
    int deltaTo = std::min(nFrames, dropFrame + 6);
    for (int i = deltaFrom; i < deltaTo; ++i) {
        FrameDelta d = ComputeDelta(frames[i-1], frames[i]);
        fprintf(stderr, "    %d->%d: changed=%d meanDelta=%.1f maxDelta=%.0f %s\n",
                i-1, i, d.changedPixels, d.meanDelta, d.maxDelta,
                (i == dropFrame) ? " <-- DROP" : "");
    }

    // Trail analysis: temporal 4-frame comparison around drop
    int trailPixels = 0;
    int trailClusters = 0;
    int peakCluster = 0;
    bool trailDetected = false;

    if (dropFrame >= 2 && dropFrame + 3 <= nFrames) {
        int A = dropFrame - 2;
        int B = dropFrame;
        int C = dropFrame + 1;
        int D = dropFrame + 2;

        // Find drop position: look for the largest cluster of changed pixels
        // in the drop frame (this is where the ItemWindow appeared)
        float itemX = -1, itemY = -1;

        // Estimate item position: centroid of changed pixels at drop
        FrameDelta dropDelta = ComputeDelta(frames[B-1], frames[B]);
        if (dropDelta.changedPixels > 100) {
            // Compute centroid of changed pixels
            double cx = 0, cy = 0;
            int count = 0;
            int w = frames[B].width;
            CaptureResult& prev = frames[B-1];
            CaptureResult& cur = frames[B];
            for (int y = 0; y < cur.height; y += 4) {
                for (int x = 0; x < w; x += 4) {
                    size_t idx = (size_t)(y * w + x) * 4;
                    float pd = PixelDelta(&prev.pixels[idx], &cur.pixels[idx]);
                    if (pd > 500.0f) {
                        cx += x; cy += y;
                        count++;
                    }
                }
            }
            if (count > 10) {
                itemX = (float)(cx / count);
                itemY = (float)(cy / count);
            }
        }

        fprintf(stderr, "\n  Temporal 4-frame analysis (A=%d B=%d C=%d D=%d):\n",
                A, B, C, D);
        fprintf(stderr, "    estimated item pos: (%.0f, %.0f)\n", itemX, itemY);

        auto mask = BuildTrailMask(
            frames[A], frames[B], frames[C], frames[D],
            itemX, itemY);

        trailPixels = AnalyzeTrailMask(mask,
            frames[B].width, frames[B].height,
            &trailClusters, &trailPixels, &peakCluster);

        // Save trail mask
        std::vector<uint8_t> maskRGB(frames[B].pixels.size(), 0);
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] > 0) {
                size_t p = i * 4;
                maskRGB[p + 0] = 255;
                maskRGB[p + 1] = 0;
                maskRGB[p + 2] = 0;
                maskRGB[p + 3] = 255;
            }
        }
        SaveFramePNG(maskRGB.data(), frames[B].width, frames[B].height,
                     "/tmp/trail_test_frames/trail_mask.png");

        // Save overlay: frame B with trail pixels highlighted
        std::vector<uint8_t> overlay = frames[B].pixels;
        for (size_t i = 0; i < mask.size(); ++i) {
            if (mask[i] > 0) {
                size_t p = i * 4;
                overlay[p + 0] = (uint8_t)((float)overlay[p + 0] * 0.5f + 128);
                overlay[p + 1] = (uint8_t)((float)overlay[p + 1] * 0.5f);
                overlay[p + 2] = 255;
            }
        }
        SaveFramePNG(overlay.data(), frames[B].width, frames[B].height,
                     "/tmp/trail_test_frames/trail_overlay.png");

        fprintf(stderr, "    trail pixels: %d in %d clusters (peak %d)\n",
                trailPixels, trailClusters, peakCluster);

        trailDetected = (trailPixels > 20 && trailClusters > 0);
    }

    // ---- Report ----
    fprintf(stderr, "\n==================================================\n");
    fprintf(stderr, "  RESULTS\n");
    fprintf(stderr, "==================================================\n");
    fprintf(stderr, "  Drop frame: %d / %d\n", dropFrame, nFrames);
    fprintf(stderr, "  Frames captured: %d\n", s_captureCount);
    fprintf(stderr, "  Not enough active polls: %d\n", totalPolls);
    if (trailDetected) {
        fprintf(stderr, "  TRAIL DETECTED: %d pixels in %d clusters (peak %d)\n",
                trailPixels, trailClusters, peakCluster);
        fprintf(stderr, "  Output: /tmp/trail_test_frames/\n");
        fprintf(stderr, "    trail_mask.png       — trail pixels in red\n");
        fprintf(stderr, "    trail_overlay.png    — frame B with trail overlay\n");
        return 10;
    } else {
        fprintf(stderr, "  No trail pattern detected.\n");
        fprintf(stderr, "  Output: /tmp/trail_test_frames/\n");
        return 0;
    }
}
