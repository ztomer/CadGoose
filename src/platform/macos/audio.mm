#import "audio.h"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>
#import "config.h"

extern bool g_debugMode;

// AVAudioPlayer is fine for "play this once, in a while" sounds, but each
// [AVAudioPlayer play] costs ~5ms of main-thread work setting up an audio
// node — too expensive for sounds we trigger 12 times/sec per goose
// (footsteps, honks). For those we use AudioServicesPlaySystemSound, which
// is the OS's purpose-built path for short sound effects and is essentially
// free at the call site.
static SystemSoundID g_patSounds[3] = {0, 0, 0};
static SystemSoundID g_honkSounds[4] = {0, 0, 0, 0};

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

static SystemSoundID LoadSystemSound(NSString* path) {
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) return 0;
    SystemSoundID sid = 0;
    OSStatus status = AudioServicesCreateSystemSoundID(
        (__bridge CFURLRef)[NSURL fileURLWithPath:path], &sid);
    if (status != 0) {
        DEBUG_LOG("AudioServicesCreateSystemSoundID failed (%d) for %s",
                  (int)status, [path UTF8String]);
        return 0;
    }
    return sid;
}

void Audio_Init() {
    if (g_audioInitialized) return;

    NSString* assetsPath = GetAssetsPath();
    DEBUG_LOG("Assets path: %s", [assetsPath UTF8String]);

    // Footsteps and honks fire frequently — use SystemSound (cheap path).
    NSArray* patFiles  = @[@"Pat1",  @"Pat2",  @"Pat3"];
    for (int i = 0; i < 3; i++) {
        g_patSounds[i] = LoadSystemSound([assetsPath stringByAppendingPathComponent:
            [NSString stringWithFormat:@"Sound/NotEmbedded/%@.wav", patFiles[i]]]);
    }

    NSArray* honkFiles = @[@"Honk1", @"Honk2", @"Honk3", @"Honk4"];
    for (int i = 0; i < 4; i++) {
        g_honkSounds[i] = LoadSystemSound([assetsPath stringByAppendingPathComponent:
            [NSString stringWithFormat:@"Sound/NotEmbedded/%@.mp3", honkFiles[i]]]);
    }

    // Bite and mud are infrequent — keep AVAudioPlayer for simplicity.
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

    g_audioInitialized = true;
    DEBUG_LOG("Audio initialized");
}

void Audio_PlayHonk() {
    if (g_config.general.audioMuted) return;
    if (!g_audioInitialized) Audio_Init();
    SystemSoundID sid = g_honkSounds[arc4random_uniform(4)];
    if (sid) AudioServicesPlaySystemSound(sid);
}

void Audio_PlayPat() {
    if (g_config.general.audioMuted) return;
    if (!g_audioInitialized) Audio_Init();
    SystemSoundID sid = g_patSounds[arc4random_uniform(3)];
    if (sid) AudioServicesPlaySystemSound(sid);
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