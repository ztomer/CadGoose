#import "audio.h"
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import "config.h"

extern bool g_debugMode;

static AVAudioPlayer* g_honkPlayers[4] = {nullptr, nullptr, nullptr, nullptr};
static AVAudioPlayer* g_patPlayers[3] = {nullptr, nullptr, nullptr};
static AVAudioPlayer* g_bitePlayer = nullptr;
static AVAudioPlayer* g_mudPlayer = nullptr;
static bool g_audioInitialized = false;

#define DEBUG_LOG(fmt, ...) do { \
    if (g_debugMode) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)

static NSString* GetAssetsPath() {
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* execPath = [bundle executablePath];
    NSString* buildDir = [execPath stringByDeletingLastPathComponent];
    NSString* projectDir = [buildDir stringByDeletingLastPathComponent];
    return [projectDir stringByAppendingPathComponent:@"Assets"];
}

void Audio_Init() {
    if (g_audioInitialized) return;
    
    NSString* assetsPath = GetAssetsPath();
    DEBUG_LOG("Assets path: %s", [assetsPath UTF8String]);
    
    NSArray* honkFiles = @[@"Honk1", @"Honk2", @"Honk3", @"Honk4"];
    NSError* error = nil;
    
    for (int i = 0; i < 4; i++) {
        NSString* path = [assetsPath stringByAppendingPathComponent:
                          [NSString stringWithFormat:@"Sound/NotEmbedded/%@.mp3", honkFiles[i]]];
        
        if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
            NSURL* url = [NSURL fileURLWithPath:path];
            g_honkPlayers[i] = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
            if (g_honkPlayers[i]) {
                g_honkPlayers[i].numberOfLoops = 0;
                [g_honkPlayers[i] prepareToPlay];
                DEBUG_LOG("Loaded honk %d: %s", i + 1, [path UTF8String]);
            }
        } else {
            DEBUG_LOG("Audio not found: %s", [path UTF8String]);
        }
    }
    
    NSString* bitePath = [assetsPath stringByAppendingPathComponent:@"Sound/NotEmbedded/BITE.mp3"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:bitePath]) {
        g_bitePlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:bitePath] error:nil];
        [g_bitePlayer prepareToPlay];
    }
    
    NSString* mudPath = [assetsPath stringByAppendingPathComponent:@"Sound/NotEmbedded/MudSquith.mp3"];
    if ([[NSFileManager defaultManager] fileExistsAtPath:mudPath]) {
        g_mudPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:mudPath] error:nil];
        [g_mudPlayer prepareToPlay];
    }
    
    NSArray* patFiles = @[@"Pat1", @"Pat2", @"Pat3"];
    for (int i = 0; i < 3; i++) {
        NSString* path = [assetsPath stringByAppendingPathComponent:
                          [NSString stringWithFormat:@"Sound/NotEmbedded/%@.wav", patFiles[i]]];
        if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
            NSURL* url = [NSURL fileURLWithPath:path];
            g_patPlayers[i] = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:nil];
            if (g_patPlayers[i]) {
                g_patPlayers[i].numberOfLoops = 0;
                [g_patPlayers[i] prepareToPlay];
            }
        }
    }
    
    g_audioInitialized = true;
    DEBUG_LOG("Audio initialized");
}

// Setting AVAudioPlayer.currentTime = 0 on an already-playing player forces a
// synchronous seek that re-decodes the mp3 frame and can take ~5-10ms. Called
// every footstep at 12Hz per goose, this was the dominant CPU consumer (~30%
// of the steady-state load on the 2026-05-19 sample). Restart by stop+play
// only when the player is idle, and pick a non-playing slot from the pool
// when one's available so overlapping sounds still work.
static void PlayFromPool(AVAudioPlayer* const* pool, int count) {
    if (!pool || count <= 0) return;
    // Prefer a player that's not already playing — those don't need the
    // expensive currentTime=0 re-decode.
    AVAudioPlayer* idle = nullptr;
    for (int i = 0; i < count; ++i) {
        AVAudioPlayer* p = pool[i];
        if (!p) continue;
        if (!p.isPlaying) { idle = p; break; }
    }
    if (idle) {
        [idle play];
        return;
    }
    // All slots busy: skip this trigger rather than yank an in-flight clip.
    // For footsteps at 12Hz this is inaudible; the dropped trigger costs 0
    // CPU vs ~5ms for a forced restart.
}

void Audio_PlayHonk() {
    if (g_config.general.audioMuted) return;
    if (!g_audioInitialized) Audio_Init();
    PlayFromPool(g_honkPlayers, 4);
    DEBUG_LOG("Playing honk");
}

void Audio_PlayPat() {
    if (g_config.general.audioMuted) return;
    if (!g_audioInitialized) Audio_Init();
    PlayFromPool(g_patPlayers, 3);
}

void Audio_PlayBite() {
    if (g_config.general.audioMuted) return;
    if (!g_audioInitialized) Audio_Init();
    if (g_bitePlayer && !g_bitePlayer.isPlaying) {
        [g_bitePlayer play];
    }
}

void Audio_PlayMudSquish() {
    if (g_config.general.audioMuted) return;
    if (!g_audioInitialized) Audio_Init();
    if (g_mudPlayer && !g_mudPlayer.isPlaying) {
        [g_mudPlayer play];
    }
}