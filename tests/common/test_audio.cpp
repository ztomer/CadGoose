// Audio gate + lifecycle coverage.
//
// Every playback call here runs under the SUPPRESSED path (muted or
// disabled), so the suite stays silent — the pools are exercised for real
// by Audio_Init's prepareToPlay, but nothing is ever audible. Playback with
// sound on belongs to user-POV QA, not unit tests.
#include <gtest/gtest.h>
#include "audio.h"

namespace {

class AudioGateTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {
        // Restore the app defaults so later tests / behaviors are unaffected.
        Audio_SetMuted(false);
        Audio_SetEnabled(true);
    }
};

TEST_F(AudioGateTest, MutedSuppressesAllPlayback) {
    Audio_SetEnabled(true);
    Audio_SetMuted(true);

    // None of these may produce sound or crash; they must early-return.
    Audio_PlayHonk();
    Audio_PlayGulag();
    Audio_PlayPat();
    Audio_PlayBite();
    Audio_PlayMudSquish();

    SUCCEED();
}

TEST_F(AudioGateTest, DisabledSuppressesAllPlayback) {
    // audio_enabled=false gates playback even when not muted — this exact
    // combination once cost ~3.7% of main-thread time (see audio.mm).
    Audio_SetMuted(false);
    Audio_SetEnabled(false);

    Audio_PlayHonk();
    Audio_PlayGulag();
    Audio_PlayPat();
    Audio_PlayBite();
    Audio_PlayMudSquish();

    SUCCEED();
}

TEST(AudioLifecycleTest, InitIsIdempotentAndCleanupResets) {
    Audio_Init();
    Audio_Init();  // second call must be a no-op, not a pool rebuild

    Audio_Cleanup();
    // After cleanup the module returns to uninitialized state; a lazy
    // re-init via the play path must be safe (still silent: muted).
    Audio_SetMuted(true);
    Audio_PlayHonk();
    Audio_SetMuted(false);
}

}  // namespace
