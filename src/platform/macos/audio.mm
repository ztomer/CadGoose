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
static std::atomic<bool> g_audioInitialized{false};
static std::atomic<bool> g_audioMuted{false};
// Mirrors g_config.general.audioEnabled. macOS previously gated playback on the
// mute flag only, so audio_enabled=false (with audio_muted=false) still played —
// and [AVAudioPlayer play] showed up at ~3.7% of main-thread time in profiling.
static std::atomic<bool> g_audioEnabled{true};

// Single playback gate: suppressed when muted OR not enabled.
static inline bool AudioSuppressed() {
    return g_audioMuted.load() || !g_audioEnabled.load();
}

// Debug printouts: compiled out in release (CG_DISABLE_DEBUG_LOG), else gated
// on g_debugMode at runtime.
#if defined(CG_DISABLE_DEBUG_LOG)
#define DEBUG_LOG(fmt, ...) ((void)0)
#else
#define DEBUG_LOG(fmt, ...) do { \
    if (g_debugMode) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)
#endif

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
    if (g_audioInitialized.load()) return;

    if (ASSET_ROOT.empty()) {
        fprintf(stderr, "[AUDIO] ASSET_ROOT not initialized — sounds unavailable\n");
        g_audioInitialized.store(true);
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

    g_audioInitialized.store(true);
    g_audioMuted.store(g_config.general.audioMuted);
    g_audioEnabled.store(g_config.general.audioEnabled);
    DEBUG_LOG("Audio initialized");
}

// ── The output device, and the test binary's silent stand-in for it ──────
//
// The test binary (CG_TEST_SEAMS) has no route to the speakers at all: a clip
// "starts" by joining a set the tests read and finish, so the pool logic
// below runs for real -- gate, lazy init, first-idle pick, drop when busy --
// and the suite stays silent whatever the mute flag says. Muting alone kept
// the suite quiet by never reaching that logic, which is how it fell out of
// the coverage measure (ROADMAP G0.16).
#ifdef CG_TEST_SEAMS
#include "test_seams.h"

static NSMutableSet<AVAudioPlayer*>* SilentSink() {
    static NSMutableSet<AVAudioPlayer*>* started = [NSMutableSet set];
    return started;
}
static bool IsPlaying(AVAudioPlayer* p) { return [SilentSink() containsObject:p]; }
static void StartPlaying(AVAudioPlayer* p) { [SilentSink() addObject:p]; }
#else
static bool IsPlaying(AVAudioPlayer* p) { return p.isPlaying; }
static void StartPlaying(AVAudioPlayer* p) { [p play]; }
#endif

// Find the first idle player in the pool and start it. No currentTime=0 seek
// (that's the expensive step). If everything's busy, drop the trigger --
// sub-perceptible at the cadences we fire at.
static void PlayFromPool(AVAudioPlayer* const* pool, int count) {
    if (AudioSuppressed()) return;
    if (!g_audioInitialized.load()) Audio_Init();
    if (AudioSuppressed()) return;  // Init may have muted (no assets)
    for (int i = 0; i < count; ++i) {
        AVAudioPlayer* p = pool[i];
        if (p && !IsPlaying(p)) { StartPlaying(p); return; }
    }
}

void Audio_PlayHonk() { PlayFromPool(g_honkPool, kHonkPoolDepth); }
void Audio_PlayGulag() { PlayFromPool(g_gulagPool, 2); }
void Audio_PlayPat() { PlayFromPool(g_patPool, kPatPoolDepth); }
void Audio_PlayBite() { PlayFromPool(&g_bitePlayer, 1); }
void Audio_PlayMudSquish() { PlayFromPool(&g_mudPlayer, 1); }

#ifdef CG_TEST_SEAMS
int Audio_StartedForTest(AudioClip clip) {
    AVAudioPlayer* const* pool = nullptr;
    int count = 0;
    switch (clip) {
        case AudioClip::Honk: pool = g_honkPool; count = kHonkPoolDepth; break;
        case AudioClip::Gulag: pool = g_gulagPool; count = 2; break;
        case AudioClip::Pat: pool = g_patPool; count = kPatPoolDepth; break;
        case AudioClip::Bite: pool = &g_bitePlayer; count = 1; break;
        case AudioClip::MudSquish: pool = &g_mudPlayer; count = 1; break;
    }
    int started = 0;
    for (int i = 0; i < count; ++i) {
        if (pool[i] && IsPlaying(pool[i])) ++started;
    }
    return started;
}

void Audio_FinishAllForTest() { [SilentSink() removeAllObjects]; }
#endif

void Audio_SetMuted(bool muted) {
    g_audioMuted.store(muted);
}

void Audio_SetEnabled(bool enabled) {
    g_audioEnabled.store(enabled);
}

void Audio_Cleanup() {
    for (int i = 0; i < kPatPoolDepth; i++) {
        g_patPool[i] = nil;
    }
    for (int i = 0; i < kHonkPoolDepth; i++) {
        g_honkPool[i] = nil;
    }
    for (int i = 0; i < 2; i++) {
        g_gulagPool[i] = nil;
    }
    g_bitePlayer = nil;
    g_mudPlayer = nil;
    g_audioInitialized = false;
}