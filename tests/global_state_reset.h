#ifndef CADGOOSE_TESTS_GLOBAL_STATE_RESET_H
#define CADGOOSE_TESTS_GLOBAL_STATE_RESET_H

// Every test starts from the same process-wide state, whatever ran before it
// (ROADMAP G0.13).
//
// The engine is a set of process singletons: the behavior registry, the actor
// manager, the per-goose behavior states, the event bus, g_world, g_config,
// g_time, the cursor provider, and file-static state inside the behavior
// modules. Tests mutate them freely, and before this listener each fixture
// restored whatever subset it remembered. The fixed order hid the rest: under
// --gtest_shuffle all 20 seeds failed (56 tests, 7 seeds by SIGSEGV/SIGBUS from
// test behaviors whose callbacks captured a dead test's stack), and under
// --gtest_repeat 12 tests failed on every iteration after the first.
//
// GlobalState_Capture() snapshots that state once the global environment is
// up; GlobalStateRestorer puts it back after EVERY test, so no fixture has to
// remember. New shared state belongs in GlobalState_Restore(), and the shuffle
// step in .gatesrc is what goes red when someone forgets.

#include <gtest/gtest.h>

void GlobalState_Capture();
void GlobalState_Restore();
void GlobalState_DrainMainRunLoop();

class GlobalStateRestorer : public ::testing::EmptyTestEventListener {
public:
    void OnEnvironmentsSetUpEnd(const ::testing::UnitTest& unit) override;
    void OnTestEnd(const ::testing::TestInfo& info) override;
};

#endif
