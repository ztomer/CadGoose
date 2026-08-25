#import "soak_capture_infra.h"

// test_soak_fetch_visibility.mm
// Soak test: repeatedly fetch test images via MCP, verify visibility via SCStream.
// Runs until failure detected (item not visible during carry) or 10 minutes elapsed.
//
// Usage: launch CadGoose, then run this test.
//   Exit 0 = all cycles passed (item visible during every carry)
//   Exit 1 = connection / socket error
//   Exit 2 = SCStream permission error
//   Exit 11 = item not visible during a carry phase

static const int kTestImageW = 100;
static const int kTestImageH = 50;
static const int kExpectedCyanPixels = kTestImageW * kTestImageH;
static const int kCyanThreshold = kExpectedCyanPixels / 2;
static const double kCycleTimeoutMs = 20000.0;
static const double kSoakDurationMs = 600000.0;
static const int kMaxCycles = 1000;

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
