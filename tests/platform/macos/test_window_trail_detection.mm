#import "trail_detection_infra.h"

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
