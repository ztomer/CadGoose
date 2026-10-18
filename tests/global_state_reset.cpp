#include "global_state_reset.h"

#include <optional>

#include <CoreFoundation/CoreFoundation.h>

#include "actor.h"
#include "ai_text_meme.h"
#include "audio.h"
#include "behavior.h"
#include "behavior_manager.h"
#include "config.h"
#include "cursor_io.h"
#include "event_bus.h"
#include "hotkey_cache.h"
#include "test_seams.h"
#include "world.h"
#include "common/platform_input_mock.h"

// The test doubles' own state (tests/common/*_stub.cpp, src/common/*_mock.cpp).
void CommandSocketStub_Reset();
void ResetMockAIHttpClient();
void ResetMockLocalLLM();
void ResetMockAITextMeme();
void ResetMockWindowSystem();

namespace {

// The process-wide values as they stood once the global environment was up.
struct Baseline {
    Config config;
    WorldContext world;
    double time = 0.0;
    ICursorProvider* cursorProvider = nullptr;
};

std::optional<Baseline>& Captured() {
    static std::optional<Baseline> baseline;
    return baseline;
}

// Every actor, leaked or not, goes: the manager owns them (AGENTS.md), and a
// survivor is a goose or item the next test finds by index or by count.
void DestroyAllActors() {
    auto& mgr = ActorManager::Instance();
    while (mgr.totalCount() > 0) {
        Actor* actor = mgr.getByIndex(mgr.totalCount() - 1);
        mgr.remove(actor);
        delete actor;
    }
}

// The suite is silent (tests/common/test_audio.cpp). It was not: nothing
// muted the process, so every honk a test triggered played out loud, and a
// test that unmuted or lazily built the pools left them for the next. The
// players' finishedPlaying: callbacks then sat in the main run loop for
// whichever later test spun it first, by which time AudioLifecycleTest had
// released the players: the WindowLifecycleTest.OrderFrontShowsCloseHides
// SIGSEGV/SIGBUS seeds.
void SilenceAudio() {
    Audio_Cleanup();
    Audio_SetMuted(true);
    Audio_SetEnabled(true);
}

}  // namespace

void GlobalState_Capture() {
    SilenceAudio();
    BehaviorRegistry::Instance().SaveOriginal();
    Captured() = Baseline{g_config, g_world, g_time, g_cursorProvider};
}

void GlobalState_Restore() {
    // Actors first: a goose's destructor reaches into the behavior states.
    DestroyAllActors();
    BehaviorStateManager::Instance().ClearAll();
    EventBus::Instance().Clear();
    BehaviorRegistry::Instance().RestoreOriginal();

    if (const auto& baseline = Captured()) {
        g_config = baseline->config;
        g_world = baseline->world;
        g_time = baseline->time;
        g_cursorProvider = baseline->cursorProvider;
    }
    // The hotkey cache is derived from g_config's strings.
    Hotkey_SyncFromConfig();

    SilenceAudio();

    // State that lives inside the modules (include/test_seams.h).
    Jail_ResetForTest();
    Portal_ResetForTest();
    Presence_ResetForTest();
    Breadcrumbs_ResetForTest();
    Honcker_ResetForTest();
    Toys_ResetForTest();
    GooseSeparationCache_ResetForTest();
    AI_TextMeme_Reset();

    // ...and inside the test doubles: a canned response or failure one test
    // set up is what the next test's call gets (the MCPTools family).
    PlatformInputMock_Reset();
    CommandSocketStub_Reset();
    ResetMockAIHttpClient();
    ResetMockLocalLLM();
    ResetMockAITextMeme();
    ResetMockWindowSystem();
}

void GlobalState_DrainMainRunLoop() {
    // Bounded: a source that re-signals itself must not hang the suite.
    for (int i = 0; i < 10000; ++i) {
        if (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true) != kCFRunLoopRunHandledSource) return;
    }
}

void GlobalStateRestorer::OnEnvironmentsSetUpEnd(const ::testing::UnitTest&) {
    // Once: --gtest_repeat sets the environments up again on every iteration,
    // and a later snapshot would bless whatever the previous one leaked.
    if (!Captured()) GlobalState_Capture();
}

void GlobalStateRestorer::OnTestEnd(const ::testing::TestInfo&) {
    GlobalState_DrainMainRunLoop();
    GlobalState_Restore();
}
