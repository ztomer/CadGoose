#ifndef CADGOOSE_TEST_SEAMS_H
#define CADGOOSE_TEST_SEAMS_H

// Reset seams for state that lives inside production modules (ROADMAP G0.13).
//
// They exist so the test harness can put the process back to its start state
// between tests (tests/global_state_reset.cpp calls every one after every
// test). They are compiled only where CG_TEST_SEAMS is defined, which
// cmake/Tests.cmake sets for CadGooseTests alone: the app binary does not
// contain them, so no production path can reach one. A module that grows new
// file-static state adds its reset here and to GlobalState_Restore(); the
// shuffle step in .gatesrc is what goes red when that is forgotten.

#ifndef CG_TEST_SEAMS
#error "test_seams.h is for the CadGooseTests target only (CG_TEST_SEAMS)"
#endif

#include <optional>

void Jail_ResetForTest();
void Portal_ResetForTest();
void Presence_ResetForTest();
void Breadcrumbs_ResetForTest();
void Honcker_ResetForTest();
void Toys_ResetForTest();
void GooseSeparationCache_ResetForTest();

// Audio (src/platform/macos/audio.mm). The test binary has no output device:
// every clip starts in a silent sink instead, which these read and drain.
enum class AudioClip { Honk, Gulag, Pat, Bite, MudSquish };
int Audio_StartedForTest(AudioClip clip);  // started and not yet finished
void Audio_FinishAllForTest();             // every started clip ends

// Night (src/common/goose.cpp: geese walk slower from 23:00 to 06:00). A
// value pins it; nullopt reads the wall clock again, as the app does.
void Nighttime_SetForTest(std::optional<bool> night);
bool Nighttime_IsNowForTest();

#endif
