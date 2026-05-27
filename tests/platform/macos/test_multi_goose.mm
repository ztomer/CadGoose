// test_multi_goose.mm
// Multi-goose regression test: spawns 3 geese, verifies all functional via command socket.
//
// Spawns geese "Alpha", "Beta", "Gamma". Verifies:
//   1. goose_count=3 in status
//   2. Each goose can complete a forced fetch cycle (fetch → carry → drop)
//
// Does NOT require Screen Recording permission (unlike SCStream-based tests).
// For pixel-level visibility in multi-goose, use the GTest goose_render tests.
//
// Usage: launch CadGoose, then run this test.
//   Exit 0  = all 3 geese functional
//   Exit 1  = connection / socket error
//   Exit 11+ = one or more geese failed (exit code = 11 + failure count)

#include "command_socket.h"
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <chrono>
#include <thread>

static const double kCycleTimeoutMs = 25000.0;
static const int kNumGeese = 3;
static const char* kGooseNames[kNumGeese] = {"Alpha", "Beta", "Gamma"};

static double GetNowMs() {
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

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

static int GetDroppedItemCount(const std::string& status) {
    size_t pos = status.find("dropped_items=");
    if (pos == std::string::npos) return -1;
    return std::stoi(status.substr(pos + 14));
}

int main() {
    mkdir("/tmp/multi_goose_test", 0755);

    fprintf(stderr, "=======================================================\n");
    fprintf(stderr, "  Multi-Goose Regression Test\n");
    fprintf(stderr, "  Geese: %d (%s, %s, %s)\n",
            kNumGeese, kGooseNames[0], kGooseNames[1], kGooseNames[2]);
    fprintf(stderr, "=======================================================\n");

    // ---- Connect ----
    fprintf(stderr, "\n[Connect] ");
    std::string resp = SendCmd("status");
    if (resp.empty() || resp.find("running=1") == std::string::npos) {
        fprintf(stderr, "FAIL: CadGoose not reachable.\n");
        return 1;
    }
    fprintf(stderr, "OK.\n");

    // ---- Clear and spawn 3 geese ----
    fprintf(stderr, "\n[Spawn] Clearing all...\n");
    SendCmd("clear");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    fprintf(stderr, "  Spawning 3 geese...\n");
    for (int i = 0; i < kNumGeese; ++i) {
        resp = SendCmd(std::string("spawn ") + kGooseNames[i]);
        if (resp.find("ok id=") == std::string::npos) {
            fprintf(stderr, "    %s -> FAIL: %s", kGooseNames[i], resp.c_str());
            SendCmd("clear");
            return 1;
        }
        fprintf(stderr, "    %s -> %s", kGooseNames[i], resp.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ---- Verify goose_count ----
    resp = SendCmd("status");
    size_t gcountPos = resp.find("goose_count=");
    int gooseCount = -1;
    if (gcountPos != std::string::npos) {
        gooseCount = std::stoi(resp.substr(gcountPos + 12));
    }
    if (gooseCount != kNumGeese) {
        fprintf(stderr, "\nFAIL: Expected goose_count=%d, got %d\n", kNumGeese, gooseCount);
        SendCmd("clear");
        return 1;
    }
    fprintf(stderr, "  goose_count=%d (verified)\n", gooseCount);

    // ---- Disable non-fetch behaviors ----
    static const char* kDisable[] = {
        "ball", "breadcrumbs", "anger", "health", "jail", "portal",
        "pomodoro", "presence", "rainbow", "toys", "interactive_drops",
        "hats", "boredom", "peeking", "acid", "nametag", "honcker",
        nullptr
    };
    fprintf(stderr, "\n[Setup] Disabling non-fetch behaviors...\n");
    for (int i = 0; kDisable[i]; ++i)
        SendCmd(std::string("disable ") + kDisable[i]);
    fprintf(stderr, "  Done.\n");

    // ---- Test each goose ----
    double startTime = GetNowMs();
    int passCount = 0;
    int failCount = 0;

    for (int gooseIdx = 0; gooseIdx < kNumGeese; ++gooseIdx) {
        fprintf(stderr, "\n--- Goose %d (%s) ---\n", gooseIdx, kGooseNames[gooseIdx]);

        // Clear dropped items from previous cycles
        SendCmd("clear_dropped");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Note current dropped items count
        resp = SendCmd("status");
        int initialDropped = GetDroppedItemCount(resp);
        fprintf(stderr, "  initial dropped=%d\n", initialDropped);

        // Trigger fetch for this specific goose
        std::string fetchCmd = "fetch " + std::to_string(gooseIdx) + " test";
        resp = SendCmd(fetchCmd);
        fprintf(stderr, "  fetch: %s", resp.c_str());

        // Wait for drop (poll dropped_items count)
        bool dropped = false;
        double cycleStart = GetNowMs();
        for (int poll = 0; poll < 10000; ++poll) {
            resp = SendCmd("status");
            int current = GetDroppedItemCount(resp);
            if (current > initialDropped) {
                dropped = true;
                fprintf(stderr, "  -> Drop at poll %d (dropped=%d)\n", poll, current);
                break;
            }
            if (GetNowMs() - cycleStart > kCycleTimeoutMs) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        if (dropped) {
            passCount++;
            fprintf(stderr, "  PASS\n");
        } else {
            failCount++;
            fprintf(stderr, "  TIMEOUT — goose did not complete fetch cycle\n");
        }
    }

    // ---- Cleanup ----
    SendCmd("clear_dropped");

    // ---- Report ----
    fprintf(stderr, "\n\n=======================================================\n");
    fprintf(stderr, "  RESULTS\n");
    fprintf(stderr, "=======================================================\n");
    fprintf(stderr, "  Duration: %.1fs\n", (GetNowMs() - startTime) / 1000.0);
    fprintf(stderr, "  Passed:   %d\n", passCount);
    fprintf(stderr, "  Failed:   %d\n", failCount);
    for (int i = 0; i < kNumGeese; ++i) {
        fprintf(stderr, "    %s: %s\n", kGooseNames[i],
                (i < passCount) ? "PASS" : "FAIL");
    }

    SendCmd("clear");

    if (failCount > 0) {
        int exitCode = 11 + failCount;
        fprintf(stderr, "  EXIT: %d (goose failure)\n", exitCode);
        return exitCode;
    }
    fprintf(stderr, "  EXIT: 0 (all geese functional)\n");
    return 0;
}
