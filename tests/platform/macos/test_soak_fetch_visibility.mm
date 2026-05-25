// test_soak_fetch_visibility.mm
// Soak test: repeatedly fetch test images via MCP, verify visibility via SCStream.
// Runs until failure detected (item not visible during carry) or 10 minutes elapsed.
//
// Usage: launch CadGoose, then run this test.
//   Exit 0 = all cycles passed (item visible during every carry)
//   Exit 1 = connection / socket error
//   Exit 2 = SCStream permission error
//   Exit 11 = item not visible during a carry phase

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreMedia/CoreMedia.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include "command_socket.h"
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

static const int kTestImageW = 100;
static const int kTestImageH = 50;
static const int kExpectedCyanPixels = kTestImageW * kTestImageH;
static const int kCyanThreshold = kExpectedCyanPixels / 2;
static const double kCycleTimeoutMs = 20000.0;
static const double kSoakDurationMs = 600000.0;
static const int kMaxCycles = 1000;

static double GetNowMs() {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ============================================================
// Frame capture — appends to a vector (no ring buffer)
// ============================================================

struct CaptureResult {
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    double captureMs = 0;
};

static std::vector<CaptureResult> g_frames;
static std::mutex g_frameMutex;
static std::atomic<bool> g_shouldStop{false};
static dispatch_semaphore_t g_captureDone = nil;
static dispatch_queue_t g_captureQueue = nil;

@interface SoakCaptureDelegate : NSObject <SCStreamOutput>
@property (atomic) BOOL shouldStop;
@end

// Thread-safe single-frame result for format probe
static CaptureResult g_probeResult;
static std::mutex g_probeMutex;
static dispatch_semaphore_t g_probeDone = nil;

@implementation SoakCaptureDelegate
- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sb
         ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeScreen) return;
    @autoreleasepool {
        CVPixelBufferRef pb = CMSampleBufferGetImageBuffer(sb);
        if (!pb) return;

        // ---- Format probe (only on first frame) ----
        static std::once_flag s_formatProbed;
        std::call_once(s_formatProbed, [pb] {
            OSType fmt = CVPixelBufferGetPixelFormatType(pb);
            int w = (int)CVPixelBufferGetWidth(pb);
            int h = (int)CVPixelBufferGetHeight(pb);
            size_t bpr = CVPixelBufferGetBytesPerRow(pb);
            char fcc[5] = { (char)(fmt>>24), (char)(fmt>>16), (char)(fmt>>8), (char)(fmt), 0 };
            fprintf(stderr, "\n  [FORMAT] CVPixelBuffer format: 0x%x (%s)\n", (unsigned)fmt, fcc);
            fprintf(stderr, "  [FORMAT] Dimensions: %dx%d\n", w, h);
            fprintf(stderr, "  [FORMAT] BytesPerRow: %zu (computed stride: %d)\n", bpr, w*4);
            if (bpr != (size_t)w * 4) {
                fprintf(stderr, "  [FORMAT] WARNING: bpr != w*4 (padding: %zu bytes)\n", bpr - (size_t)w * 4);
            }

            CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
            uint8_t* src = (uint8_t*)CVPixelBufferGetBaseAddress(pb);
            int nw = w > 10 ? 10 : w;
            int nh = h > 10 ? 10 : h;

            // Count cyan with different byte order assumptions
            auto countCyan = [&](int bOff, int gOff, int rOff) -> int {
                int cnt = 0;
                for (int yy = 0; yy < nh; ++yy)
                    for (int xx = 0; xx < nw; ++xx) {
                        const uint8_t* p = src + yy * bpr + xx * 4;
                        if (p[gOff] > 200 && p[bOff] > 200 && p[rOff] < 50) cnt++;
                    }
                return cnt;
            };

            int cyanBGRA = countCyan(0, 1, 2);
            int cyanRGBA = countCyan(2, 1, 0);
            int cyanARGB = countCyan(3, 2, 1);
            int cyanABGR = countCyan(1, 2, 3);
            fprintf(stderr, "  [FORMAT] Cyan count (top-left 10x10):\n");
            fprintf(stderr, "  [FORMAT]   as BGRA (b[0] g[1] r[2]): %d\n", cyanBGRA);
            fprintf(stderr, "  [FORMAT]   as RGBA (r[0] g[1] b[2]): %d\n", cyanRGBA);
            fprintf(stderr, "  [FORMAT]   as ARGB (a[0] r[1] g[2]): %d\n", cyanARGB);
            fprintf(stderr, "  [FORMAT]   as ABGR (a[0] b[1] g[2]): %d\n", cyanABGR);

            // Print first 4 pixels in raw bytes
            fprintf(stderr, "  [FORMAT] First 4 pixels (bytes 0..3):\n");
            for (int i = 0; i < 4; ++i) {
                const uint8_t* p = src + i * 4;
                fprintf(stderr, "    Pixel %d: [0]=%d [1]=%d [2]=%d [3]=%d\n",
                        i, p[0], p[1], p[2], p[3]);
            }

            // Check for uniform-color regions (might be test image or background)
            // Vertical strip at x=0: check bytes across rows
            int by[3] = {0,0,0}, bg[3] = {0,0,0}, br[3] = {0,0,0}, ba[3] = {0,0,0};
            for (int yy = 0; yy < nh && yy < 50; ++yy) {
                const uint8_t* p = src + yy * bpr;
                by[p[0]/85]++; bg[p[1]/85]++; br[p[2]/85]++; ba[p[3]/85]++;
            }
            fprintf(stderr, "  [FORMAT] Byte distribution at x=0 (y=0..%d):\n", nh < 50 ? nh-1 : 49);
            fprintf(stderr, "    byte[0] (low): lo=%d med=%d hi=%d\n", by[0], by[1], by[2]);
            fprintf(stderr, "    byte[1]:       lo=%d med=%d hi=%d\n", bg[0], bg[1], bg[2]);
            fprintf(stderr, "    byte[2]:       lo=%d med=%d hi=%d\n", br[0], br[1], br[2]);
            fprintf(stderr, "    byte[3] (high): lo=%d med=%d hi=%d\n", ba[0], ba[1], ba[2]);

            // Save probe frame as PNG
            {
                CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
                CGContextRef ctx = CGBitmapContextCreate(
                    (void*)src, w, h, 8, bpr, cs,
                    (CGBitmapInfo)kCGImageAlphaPremultipliedFirst |
                        kCGBitmapByteOrder32Little);
                CGColorSpaceRelease(cs);
                if (ctx) {
                    CGImageRef img = CGBitmapContextCreateImage(ctx);
                    if (img) {
                        NSString* path = @"/tmp/soak_fetch_test/format_probe.png";
                        NSURL* url = [NSURL fileURLWithPath:path];
                        CGImageDestinationRef dest = CGImageDestinationCreateWithURL(
                            (__bridge CFURLRef)url, (__bridge CFStringRef)@"public.png", 1, NULL);
                        if (dest) {
                            CGImageDestinationAddImage(dest, img, NULL);
                            CGImageDestinationFinalize(dest);
                            CFRelease(dest);
                        }
                        CGImageRelease(img);
                    }
                    CGContextRelease(ctx);
                }
                fprintf(stderr, "  [FORMAT] Probe PNG: /tmp/soak_fetch_test/format_probe.png\n");
            }

            CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
        });

        CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
        CaptureResult frame;
        frame.width = (int)CVPixelBufferGetWidth(pb);
        frame.height = (int)CVPixelBufferGetHeight(pb);
        frame.captureMs = GetNowMs();
        size_t bpr = CVPixelBufferGetBytesPerRow(pb);
        uint8_t* src = (uint8_t*)CVPixelBufferGetBaseAddress(pb);
        frame.pixels.resize(frame.height * frame.width * 4);
        for (int y = 0; y < frame.height; ++y) {
            memcpy(&frame.pixels[y * frame.width * 4], src + y * bpr, frame.width * 4);
        }
        CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
        {
            std::lock_guard<std::mutex> lock(g_frameMutex);
            g_frames.push_back(std::move(frame));
        }
    }
    if (self.shouldStop && g_captureDone) {
        dispatch_semaphore_signal(g_captureDone);
    }
}
@end

static bool StartSCStream(SCStream** outStream, SoakCaptureDelegate** outDelegate) {
    __block SCShareableContent* content = nil;
    dispatch_semaphore_t contentReady = dispatch_semaphore_create(0);
    [SCShareableContent getShareableContentWithCompletionHandler:
        ^(SCShareableContent* _Nullable sc, NSError* _Nullable error) {
            content = sc;
            dispatch_semaphore_signal(contentReady);
        }];
    dispatch_semaphore_wait(contentReady, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
    if (!content || content.displays.count == 0) return false;

    SCDisplay* display = content.displays.firstObject;
    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
    config.width = display.width;
    config.height = display.height;
    config.pixelFormat = kCVPixelFormatType_32BGRA;
    config.showsCursor = NO;
    config.capturesAudio = NO;
    config.minimumFrameInterval = CMTimeMake(1, 60);

    NSMutableArray* excludeWindows = [NSMutableArray array];
    for (SCWindow* win in content.windows) {
        NSString* appName = win.owningApplication.applicationName;
        if ([appName isEqualToString:@"soak_fetch_test"] ||
            [appName isEqualToString:@"Ghostty"]) {
            [excludeWindows addObject:win];
        }
    }

    SCContentFilter* filter = [[SCContentFilter alloc]
        initWithDisplay:display excludingWindows:excludeWindows];
    SoakCaptureDelegate* delegate = [[SoakCaptureDelegate alloc] init];
    delegate.shouldStop = NO;
    SCStream* stream = [[SCStream alloc] initWithFilter:filter
                                         configuration:config
                                              delegate:nil];
    g_captureQueue = dispatch_queue_create("capture", DISPATCH_QUEUE_SERIAL);
    NSError* addErr = nil;
    if (![stream addStreamOutput:delegate
                            type:SCStreamOutputTypeScreen
              sampleHandlerQueue:g_captureQueue
                          error:&addErr]) {
        fprintf(stderr, "  addStreamOutput error: %s\n",
                addErr.localizedDescription.UTF8String);
        return false;
    }
    dispatch_semaphore_t ready = dispatch_semaphore_create(0);
    [stream startCaptureWithCompletionHandler:^(NSError* error) {
        if (error) fprintf(stderr, "  SCStream start error: %s\n", error.localizedDescription.UTF8String);
        dispatch_semaphore_signal(ready);
    }];
    dispatch_semaphore_wait(ready, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
    *outStream = stream;
    *outDelegate = delegate;
    return true;
}

static void StopSCStream(SCStream* stream, SoakCaptureDelegate* delegate) {
    delegate.shouldStop = YES;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    g_captureDone = done;
    [stream stopCaptureWithCompletionHandler:^(NSError* error) {
        if (error) fprintf(stderr, "  SCStream stop error: %s\n", error.localizedDescription.UTF8String);
        dispatch_semaphore_signal(done);
    }];
    dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
}

// ============================================================
// Cyan pixel counting
// ============================================================

static int CountCyanPixels(const uint8_t* pixels, int w, int h) {
    int count = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t idx = (size_t)(y * w + x) * 4;
            uint8_t b = pixels[idx + 0];
            uint8_t g = pixels[idx + 1];
            uint8_t r = pixels[idx + 2];
            if (g > 200 && b > 200 && r < 50) count++;
        }
    }
    return count;
}

// ============================================================
// MCP helpers
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
// Main — soak loop
// ============================================================

int main() {
    fprintf(stderr, "=======================================================\n");
    fprintf(stderr, "  Fetch Visibility Soak Test\n");
    fprintf(stderr, "  Test image: %dx%d cyan (%d pixels)\n",
            kTestImageW, kTestImageH, kExpectedCyanPixels);
    fprintf(stderr, "  Threshold: %d cyan pixels = visible\n", kCyanThreshold);
    fprintf(stderr, "  Max duration: %.0fs\n", kSoakDurationMs / 1000.0);
    fprintf(stderr, "  Max cycles: %d\n", kMaxCycles);
    fprintf(stderr, "=======================================================\n");

    mkdir("/tmp/soak_fetch_test", 0755);

    // ---- Connect ----
    fprintf(stderr, "\n[Connect] ");
    std::string resp = SendCmd("status");
    if (resp.empty() || resp.find("running=1") == std::string::npos) {
        fprintf(stderr, "FAIL: CadGoose not reachable.\n");
        return 1;
    }
    fprintf(stderr, "OK.\n");

    auto kv = ParseStatus(resp);
    if (kv["goose_count"] == "0") {
        SendCmd("spawn");
        fprintf(stderr, "  Spawned goose.\n");
    }

    // ---- Disable non-fetch behaviors ----
    static const char* kDisable[] = {
        "ball", "breadcrumbs", "anger", "health", "jail", "portal",
        "pomodoro", "presence", "rainbow", "toys", "interactive_drops",
        "hats", "boredom", "peeking", "acid", "nametag", "honcker",
        nullptr
    };
    for (int i = 0; kDisable[i]; ++i)
        SendCmd(std::string("disable ") + kDisable[i]);

    // ---- Start SCStream ----
    fprintf(stderr, "\n[SCStream] Starting capture... ");
    SCStream* stream = nil;
    SoakCaptureDelegate* delegate = nil;
    if (!StartSCStream(&stream, &delegate)) {
        fprintf(stderr, "FAIL.\n  Ensure terminal has Screen Recording permission.\n");
        return 2;
    }
    fprintf(stderr, "OK (capturing at vsync rate).\n");

    // Warm up capture
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    {
        std::lock_guard<std::mutex> lock(g_frameMutex);
        g_frames.clear();
    }

    // ---- Soak loop ----
    double startTime = GetNowMs();
    int passed = 0, failed = 0;

    for (int cycle = 0; cycle < kMaxCycles; ++cycle) {
        double elapsed = GetNowMs() - startTime;
        if (elapsed > kSoakDurationMs) {
            fprintf(stderr, "\n[Soak] Time limit reached (%.0fs).\n", elapsed / 1000.0);
            break;
        }

        // Clear frame buffer before triggering fetch
        {
            std::lock_guard<std::mutex> lock(g_frameMutex);
            g_frames.clear();
        }

        // Trigger fetch with test image
        resp = SendCmd("fetch\ttest");
        if (resp.find("ok") == std::string::npos) {
            fprintf(stderr, "\n[Cycle %d] fetch command failed: %s\n", cycle, resp.c_str());
            failed++;
            continue;
        }

        // Wait for drop
        bool dropped = false;
        double cycleStart = GetNowMs();
        for (int poll = 0; poll < 10000; ++poll) {
            resp = SendCmd("status");
            auto s = ParseStatus(resp);
            if (s["goose_state"] == "wander" && s["goose_heldItem"] == "no") {
                dropped = true;
                break;
            }
            if (GetNowMs() - cycleStart > kCycleTimeoutMs) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if (!dropped) {
            fprintf(stderr, "\n[Cycle %d] TIMEOUT waiting for drop.\n", cycle);
            failed++;
            continue;
        }

        // Let frames settle after drop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Analyze frames for cyan
        int maxCyan = 0;
        int totalFrames = 0;
        {
            std::lock_guard<std::mutex> lock(g_frameMutex);
            totalFrames = (int)g_frames.size();
            for (int fi = 0; fi < totalFrames; ++fi) {
                auto& f = g_frames[fi];
                if (fi == 0) {
                    fprintf(stderr, "  [DEBUG] frame[0] w=%d h=%d pixels.size=%zu bytes\n",
                            f.width, f.height, f.pixels.size());
                    if (f.pixels.size() >= 4) {
                        fprintf(stderr, "  [DEBUG] pixel(0,0): b=%d g=%d r=%d a=%d\n",
                                f.pixels[0], f.pixels[1], f.pixels[2], f.pixels[3]);
                    }
                }
                int cyan = CountCyanPixels(f.pixels.data(), f.width, f.height);
                if (cyan > maxCyan) maxCyan = cyan;
            }
        }

        if (maxCyan >= kCyanThreshold) {
            passed++;
            fprintf(stderr, "\r[Cycle %d/%d] PASS (maxCyan=%d, frames=%d, elapsed=%.1fs)   ",
                    cycle + 1, passed + failed, maxCyan, totalFrames, (GetNowMs() - startTime) / 1000.0);
        } else {
            failed++;
            fprintf(stderr, "\n[Cycle %d] FAIL — item not visible (maxCyan=%d < %d, frames=%d, elapsed=%.1fs)\n",
                    cycle, maxCyan, kCyanThreshold, totalFrames, (GetNowMs() - startTime) / 1000.0);

            // Save diagnostic frames + detailed byte analysis
            {
                std::lock_guard<std::mutex> lock(g_frameMutex);

                // Byte-order agnostic cyan scanning on all frames
                FILE* diag = fopen("/tmp/soak_fetch_test/fail_diagnostic.txt", "w");
                if (diag) {
                    fprintf(diag, "Fail cycle %d: %d frames, %dx%d pixels\n",
                            cycle, totalFrames, totalFrames > 0 ? g_frames[0].width : 0,
                            totalFrames > 0 ? g_frames[0].height : 0);
                }

                for (int i = 0; i < (int)g_frames.size(); ++i) {
                    auto& f = g_frames[i];
                    int w = f.width, h = f.height;

                    // Try all 4 byte orders
                    auto countOrder = [&](int offB, int offG, int offR) {
                        int cnt = 0;
                        const uint8_t* dp = f.pixels.data();
                        for (int y = 0; y < h; ++y)
                            for (int x = 0; x < w; ++x) {
                                size_t idx = (size_t)(y * w + x) * 4;
                                if (dp[idx + offG] > 200 && dp[idx + offB] > 200 && dp[idx + offR] < 50)
                                    cnt++;
                            }
                        return cnt;
                    };

                    int cntBGRA = countOrder(0, 1, 2); // byte[0]=B, byte[1]=G, byte[2]=R
                    int cntRGBA = countOrder(2, 1, 0); // byte[0]=R, byte[1]=G, byte[2]=B
                    int cntARGB = countOrder(3, 2, 1); // byte[0]=A, byte[1]=R, byte[2]=G
                    int cntABGR = countOrder(1, 2, 3); // byte[0]=A, byte[1]=B, byte[2]=G

                    // Save PNG (assuming BGRA)
                    char path[512];
                    snprintf(path, sizeof(path),
                             "/tmp/soak_fetch_test/fail_cycle%d_frame%04d_w%d_h%d_bgra%d_rgba%d_argb%d_abgr%d.png",
                             cycle, i, w, h, cntBGRA, cntRGBA, cntARGB, cntABGR);
                    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
                    CGContextRef ctx = CGBitmapContextCreate(
                        (void*)f.pixels.data(), f.width, f.height, 8, f.width * 4, cs,
                        (CGBitmapInfo)kCGImageAlphaPremultipliedFirst |
                            kCGBitmapByteOrder32Little);
                    CGColorSpaceRelease(cs);
                    if (ctx) {
                        CGImageRef cgImage = CGBitmapContextCreateImage(ctx);
                        if (cgImage) {
                            NSString* nsPath = [NSString stringWithUTF8String:path];
                            NSURL* url = [NSURL fileURLWithPath:nsPath];
                            CGImageDestinationRef dest = CGImageDestinationCreateWithURL(
                                (__bridge CFURLRef)url, (__bridge CFStringRef)@"public.png", 1, NULL);
                            if (dest) {
                                CGImageDestinationAddImage(dest, cgImage, NULL);
                                CGImageDestinationFinalize(dest);
                                CFRelease(dest);
                            }
                            CGImageRelease(cgImage);
                        }
                        CGContextRelease(ctx);
                    }

                    if (diag && i < 200) {
                        if (cntBGRA > 0 || cntRGBA > 0 || cntARGB > 0 || cntABGR > 0) {
                            fprintf(diag, "  frame[%d]: BGRA=%d RGBA=%d ARGB=%d ABGR=%d\n",
                                    i, cntBGRA, cntRGBA, cntARGB, cntABGR);
                        }
                    }
                }
                if (diag) fclose(diag);
            }
            fprintf(stderr, "  Frames + diagnostic saved to /tmp/soak_fetch_test/\n");

            // Determine best byte order
            int bestOrder = -1;
            {
                std::lock_guard<std::mutex> lock(g_frameMutex);
                auto& f = g_frames[0];
                const uint8_t* dp = f.pixels.data();
                int best = 0;
                auto countOrder = [&](int bOff, int gOff, int rOff) {
                    int cnt = 0;
                    for (int y = 0; y < f.height; ++y)
                        for (int x = 0; x < f.width; ++x) {
                            size_t idx = (size_t)(y * f.width + x) * 4;
                            if (dp[idx + gOff] > 200 && dp[idx + bOff] > 200 && dp[idx + rOff] < 50) cnt++;
                        }
                    return cnt;
                };
                int c[4] = { countOrder(0,1,2), countOrder(2,1,0), countOrder(3,2,1), countOrder(1,2,3) };
                for (int k = 0; k < 4; ++k) if (c[k] > c[best]) best = k;
                if (c[best] > kCyanThreshold) bestOrder = best;
            }

            if (bestOrder >= 0) {
                const char* names[] = { "BGRA", "RGBA", "ARGB", "ABGR" };
                fprintf(stderr, "  BYTE ORDER CLUE: %s gives %d cyan pixels\n",
                        names[bestOrder], 0);
            }
            break;
        }
    }

    // ---- Cleanup ----
    StopSCStream(stream, delegate);

    // ---- Report ----
    fprintf(stderr, "\n\n=======================================================\n");
    fprintf(stderr, "  RESULTS\n");
    fprintf(stderr, "=======================================================\n");
    fprintf(stderr, "  Duration: %.1fs\n", (GetNowMs() - startTime) / 1000.0);
    fprintf(stderr, "  Passed:   %d\n", passed);
    fprintf(stderr, "  Failed:   %d\n", failed);

    if (failed > 0) {
        fprintf(stderr, "  EXIT: 11 (failure detected)\n");
        return 11;
    }
    fprintf(stderr, "  EXIT: 0 (all cycles passed)\n");
    return 0;
}
