#pragma once

// Lightweight timer for behavior state tracking.
// All times are in seconds (same as ctx.time / g_time).
//
// Usage:
//   Timer timer;
//   timer.Start(ctx.time);
//   if (timer.IsExpired(ctx.time, 5.0)) { ... }
//   float elapsed = timer.Elapsed(ctx.time);
//   timer.Reset();
struct Timer {
    double startTime = 0.0;
    bool running = false;

    void Start(double currentTime) {
        startTime = currentTime;
        running = true;
    }

    void Stop() {
        running = false;
    }

    void Reset() {
        startTime = 0.0;
        running = false;
    }

    double Elapsed(double currentTime) const {
        if (!running) return 0.0;
        return currentTime - startTime;
    }

    bool IsExpired(double currentTime, double duration) const {
        return running && (currentTime - startTime) >= duration;
    }

    double Remaining(double currentTime, double duration) const {
        if (!running) return 0.0;
        double elapsed = currentTime - startTime;
        return elapsed >= duration ? 0.0 : duration - elapsed;
    }

    // Progress from 0.0 to 1.0 over the duration
    float Progress(double currentTime, double duration) const {
        if (!running || duration <= 0.0) return 1.0f;
        double elapsed = currentTime - startTime;
        if (elapsed >= duration) return 1.0f;
        return static_cast<float>(elapsed / duration);
    }
};
