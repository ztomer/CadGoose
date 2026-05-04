#import "audio.h"
#import <AVFoundation/AVFoundation.h>

static AVAudioPlayer* g_honkPlayer = nullptr;

void Audio_Init() {
    // macOS doesn't use AVAudioSession like iOS
    // Audio playback is handled automatically by the system
}

void Audio_PlayHonk() {
    // Placeholder - would load from bundle
}