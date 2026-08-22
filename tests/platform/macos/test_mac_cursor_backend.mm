// MacCursorBackend — the real CGEvent-based cursor backend.
//
// The Move* paths post real CGEvents; to keep the suite from yanking the
// user's pointer, they are exercised with NO-OP events only: an absolute
// move to the cursor's CURRENT position, and a relative move of (0,0).
// That executes the full create/post/release path without disturbing
// anyone. Real movement is covered by the display-dependent drag suites.
#import <gtest/gtest.h>
#import "mac_cursor_backend.h"

namespace {

TEST(MacCursorBackendTest, NameCapsInit) {
    MacCursorBackend backend;
    EXPECT_EQ(backend.Name(), "MacCGEvent");
    EXPECT_EQ(backend.Caps(), 0x1u | 0x2u | 0x4u);
    // Init creates the cached CGEventSource.
    EXPECT_TRUE(backend.Init());
}

TEST(MacCursorBackendTest, GetCursorPosReturnsScreenCoords) {
    MacCursorBackend backend;
    EXPECT_TRUE(backend.Init());

    Vector2 pos = backend.GetCursorPos();
    // A real event source on a real machine yields non-negative coords;
    // (-1,-1) is the documented failure sentinel.
    EXPECT_GE(pos.x, 0.0f);
    EXPECT_GE(pos.y, 0.0f);
}

TEST(MacCursorBackendTest, GetCursorPosWorksWithoutExplicitInit) {
    // Exercises the lazily-created-source fallback path.
    MacCursorBackend backend;
    Vector2 pos = backend.GetCursorPos();
    EXPECT_GE(pos.x, 0.0f);
}

TEST(MacCursorBackendTest, MoveAbsToCurrentPositionIsNoOp) {
    MacCursorBackend backend;
    ASSERT_TRUE(backend.Init());

    Vector2 pos = backend.GetCursorPos();
    int x = static_cast<int>(pos.x);
    int y = static_cast<int>(pos.y);

    // Post a move to where the cursor already is: full code path, zero
    // user-visible effect.
    backend.MoveCursorAbs(x, y);

    SUCCEED();
}

TEST(MacCursorBackendTest, MoveRelZeroDeltaIsNoOp) {
    MacCursorBackend backend;  // deliberately NOT Init()ed: covers the
                               // lazy-create + release branch in Move*
    backend.MoveCursorRel(0, 0);
    SUCCEED();
}

TEST(MacCursorBackendTest, DestructorReleasesEventSource) {
    {
        MacCursorBackend backend;
        EXPECT_TRUE(backend.Init());
    }  // destructor must CFRelease the cached source without crashing

    SUCCEED();
}

}  // namespace
