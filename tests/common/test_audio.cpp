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

// ── Playback through the silent sink (CG_TEST_SEAMS) ──────────────────────
//
// The test binary cannot reach the speakers: a clip "starts" in audio.mm's
// silent sink, which these tests read. So the playback path -- gate, lazy
// init, first-idle pick, drop when every player is busy -- is asserted, not
// skipped, and the suite stays silent. The listener re-mutes after each test.

#include "assets.h"
#include "test_seams.h"

namespace {

class AudioPlaybackTest : public ::testing::Test {
protected:
    void SetUp() override {
        Audio_Cleanup();  // the first play below builds the pools lazily
        Audio_SetEnabled(true);
        Audio_SetMuted(false);
    }
};

TEST_F(AudioPlaybackTest, EachSoundStartsOneClipOfItsOwn) {
    Audio_PlayHonk();
    Audio_PlayGulag();
    Audio_PlayPat();
    Audio_PlayBite();
    Audio_PlayMudSquish();

    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Gulag), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Pat), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Bite), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::MudSquish), 1);
}

TEST_F(AudioPlaybackTest, OverlappingTriggersFillThePoolThenDrop) {
    // Four honk players: four overlapping honks each get one, the fifth is
    // dropped rather than cutting one off.
    for (int i = 0; i < 5; ++i) Audio_PlayHonk();
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 4);

    // A single-player sound is busy until its clip ends.
    Audio_PlayBite();
    Audio_PlayBite();
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Bite), 1);

    // Once the clips end the players are idle again.
    Audio_FinishAllForTest();
    Audio_PlayHonk();
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 1);
}

TEST_F(AudioPlaybackTest, MutedOrDisabledStartsNothing) {
    Audio_SetMuted(true);
    Audio_PlayHonk();
    Audio_SetMuted(false);
    Audio_SetEnabled(false);
    Audio_PlayPat();

    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 0);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Pat), 0);
}

TEST_F(AudioPlaybackTest, TheAssetManagerPlaysThroughTheSamePools) {
    g_assets.Honk();
    g_assets.Gulag();
    g_assets.Pat();
    g_assets.Bite();
    g_assets.MudSquish();

    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Gulag), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Pat), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Bite), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::MudSquish), 1);
}

TEST_F(AudioPlaybackTest, NoAssetRootMutesInsteadOfPlaying) {
    // A missing asset root is a stated degradation: Init mutes, and the play
    // that triggered it starts nothing.
    const fs::path saved = ASSET_ROOT;
    ASSET_ROOT.clear();
    Audio_PlayHonk();
    ASSET_ROOT = saved;

    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 0);
    Audio_PlayHonk();  // still muted by Init until something unmutes
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 0);
}

}  // namespace
