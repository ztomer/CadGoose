#import "audio.h"
#import <AVFoundation/AVFoundation.h>

static AVAudioPlayer* g_honkPlayer = nullptr;

void Audio_Init() {
    NSError* error = nil;
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&error];
    [[AVAudioSession sharedInstance] setActive:YES error:&error];
}

void Audio_PlayHonk() {
    // Placeholder - would load from bundle
}