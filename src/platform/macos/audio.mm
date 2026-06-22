#import "audio.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import "config.h"
#include "assets.h"
#include <atomic>

extern bool g_debugMode;

// For footsteps and honks we want overlapping playback without the per-call
// expense of [player play] on a busy player (which triggers a synchronous
// audio-graph setup ~5ms long). Strategy: keep a deep pool of pre-prepared
// AVAudioPlayers per sound; when triggered, pick the first idle player. If
// they're all playing, we drop the trigger rather than yank an in-flight
// clip (cheaper, and inaudible at footstep cadence).
//
// AudioServicesPlaySystemSound is tempting (constant-time) but it respects
// the system "play user interface sound effects" toggle and ignores the
// app's volume — so users with UI sounds off get a silent goose, which
// happened on 2026-05-19.
static constexpr int kPatPoolDepth   = 6;  // ~25Hz peak (2 feet x 12Hz) needs room
static constexpr int kHonkPoolDepth  = 4;

static AVAudioPlayer* g_patPool[kPatPoolDepth]   = {nullptr};
static AVAudioPlayer* g_honkPool[kHonkPoolDepth]  = {nullptr};
static AVAudioPlayer* g_gulagPool[2]               = {nullptr};
static AVAudioPlayer* g_bitePlayer = nullptr;
static AVAudioPlayer* g_mudPlayer = nullptr;
static bool g_audioInitialized = false;
static std::atomic<bool> g_audioMuted{false};

#define DEBUG_LOG(fmt, ...) do { \
    if (g_debugMode) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)

static NSString* GetAssetsPath() {
    std::string path = (ASSET_ROOT / "Assets").string();
    return [NSString stringWithUTF8String:path.c_str()];
}

static AVAudioPlayer* MakePlayer(NSString* path) {
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) return nullptr;
    NSError* err = nil;
    AVAudioPlayer* p = [[AVAudioPlayer alloc]
        initWithContentsOfURL:[NSURL fileURLWithPath:path] error:&err];
    if (!p) {
        DEBUG_LOG("AVAudioPlayer init failed: %s err=%s",
                  [path UTF8String], [[err localizedDescription] UTF8String]);
        return nullptr;
    }
    p.numberOfLoops = 0;
    [p prepareToPlay];
    return p;
}

void Audio_Init() {
    if (g_audioInitialized) return;

    if (ASSET_ROOT.empty()) {
        fprintf(stderr, "[AUDIO] ASSET_ROOT not initialized — sounds unavailable\n");
        g_audioInitialized = true;
        g_audioMuted.store(true);
        return;
    }

    NSString* assetsPath = GetAssetsPath();
    DEBUG_LOG("Assets path: %s", [assetsPath UTF8String]);

    // Fill pat pool by cycling through the 3 source files. Pool depth >
    // expected simultaneous footsteps so we always find an idle slot.
    NSArray* patFiles  = @[@"Pat1",  @"Pat2",  @"Pat3"];
    for (int i = 0; i < kPatPoolDepth; i++) {
        NSString* path = [assetsPath stringByAppendingPathComponent:
            [NSString stringWithFormat:@"Sound/NotEmbedded/%@.wav",
                                       patFiles[i % patFiles.count]]];
        g_patPool[i] = MakePlayer(path);
    }

    NSArray* honkFiles = @[@"Honk1", @"Honk2", @"Honk3", @"Honk4"];
    for (int i = 0; i < kHonkPoolDepth; i++) {
        NSString* path = [assetsPath stringByAppendingPathComponent:
            [NSString stringWithFormat:@"Sound/NotEmbedded/%@.mp3",
                                       honkFiles[i % honkFiles.count]]];
        g_honkPool[i] = MakePlayer(path);
    }

    // Gulag pool (2 players for BabyStalin)
    for (int i = 0; i < 2; i++) {
        NSString* path = [assetsPath stringByAppendingPathComponent:
                          @"Sound/NotEmbedded/Gulag.mp3"];
        g_gulagPool[i] = MakePlayer(path);
    }

    g_bitePlayer = MakePlayer([assetsPath stringByAppendingPathComponent:
                               @"Sound/NotEmbedded/BITE.mp3"]);
    g_mudPlayer  = MakePlayer([assetsPath stringByAppendingPathComponent:
                               @"Sound/NotEmbedded/MudSquith.mp3"]);

    g_audioInitialized = true;
    g_audioMuted.store(g_config.general.audioMuted);
    DEBUG_LOG("Audio initialized");
}

// Find the first idle player in the pool and call [play] on it. No
// currentTime=0 seek (that's the expensive step). If everything's busy,
// drop the trigger — sub-perceptible at the cadences we fire at.
static void PlayFromPool(AVAudioPlayer* const* pool, int count) {
    for (int i = 0; i < count; ++i) {
        AVAudioPlayer* p = pool[i];
        if (p && !p.isPlaying) { [p play]; return; }
    }
}

void Audio_PlayHonk() {
    if (g_audioMuted.load()) return;
    if (!g_audioInitialized) Audio_Init();
    if (g_audioMuted.load()) return;
    PlayFromPool(g_honkPool, kHonkPoolDepth);
}

void Audio_PlayGulag() {
    if (g_audioMuted.load()) return;
    if (!g_audioInitialized) Audio_Init();
    if (g_audioMuted.load()) return;
    PlayFromPool(g_gulagPool, 2);
}

void Audio_PlayPat() {
    if (g_audioMuted.load()) return;
    if (!g_audioInitialized) Audio_Init();
    if (g_audioMuted.load()) return;
    PlayFromPool(g_patPool, kPatPoolDepth);
}

void Audio_PlayBite() {
    if (g_audioMuted.load()) return;
    if (!g_audioInitialized) Audio_Init();
    if (g_audioMuted.load()) return;
    if (g_bitePlayer && !g_bitePlayer.isPlaying) {
        [g_bitePlayer play];
    }
}

void Audio_PlayMudSquish() {
    if (g_audioMuted.load()) return;
    if (!g_audioInitialized) Audio_Init();
    if (g_audioMuted.load()) return;
    if (g_mudPlayer && !g_mudPlayer.isPlaying) {
        [g_mudPlayer play];
    }
}

void Audio_SetMuted(bool muted) {
    g_audioMuted.store(muted);
}