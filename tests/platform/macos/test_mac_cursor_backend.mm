// MacCursorBackend — the real CGEvent-based cursor backend.
//
// The Move* paths post real CGEvents; to keep the suite from yanking the
// user's pointer, they are exercised with NO-OP events only: an absolute
// move to the cursor's CURRENT position, and a relative move of (0,0).
// That executes the full create/post/release path without disturbing
// anyone. Real movement is covered by the display-dependent drag suites.
#import <gtest/gtest.h>
#import "mac_cursor_backend.h"
#import <CoreGraphics/CoreGraphics.h>

namespace {

// The pointer is wherever the user left it, and a display left of or above
// the main one has negative global coordinates: x=-231 on a two-display desk
// failed the old ">= 0" check under --gtest_repeat (ROADMAP G0.13). What a
// working read guarantees is a point on some active display.
::testing::AssertionResult OnSomeDisplay(Vector2 p) {
    uint32_t displays = 0;
    CGGetActiveDisplayList(0, nullptr, &displays);
    if (displays == 0) return ::testing::AssertionSuccess() << "no display to check against";
    uint32_t under = 0;
    CGGetDisplaysWithPoint(CGPointMake(p.x, p.y), 0, nullptr, &under);
    if (under > 0) return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure()
           << "(" << p.x << ", " << p.y << ") is on none of " << displays << " displays";
}

// The check above can say no.
TEST(MacCursorBackendTest, OnSomeDisplayRefusesAPointOffEveryScreen) {
    uint32_t displays = 0;
    CGGetActiveDisplayList(0, nullptr, &displays);
    if (displays == 0) GTEST_SKIP() << "no display";
    EXPECT_FALSE(OnSomeDisplay({-1.0e6f, -1.0e6f}));
}

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

    EXPECT_TRUE(OnSomeDisplay(backend.GetCursorPos()));
}

TEST(MacCursorBackendTest, GetCursorPosWorksWithoutExplicitInit) {
    // Exercises the lazily-created-source fallback path.
    MacCursorBackend backend;
    EXPECT_TRUE(OnSomeDisplay(backend.GetCursorPos()));
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
