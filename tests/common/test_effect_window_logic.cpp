// test_effect_window_logic.cpp — the pure layer lifted out of EffectWindow and
// EffectWindowManager (effect_window.mm sat at 0%: NSWindow subclass plus a
// fixed-capacity ring of them, none of it constructible headlessly).

#include <gtest/gtest.h>
#include <cmath>
#include <set>
#include <vector>

#include "effect_window_logic.h"

using namespace effect_window_logic;

namespace {
constexpr float kEps = 0.01f;
}

// ── Window sizing ──────────────────────────────────────────

TEST(EffectWindowLogic, SizeIsTheScaledDiameter) {
    EXPECT_NEAR(EffectWindowSize(50.0f, 1.0f), 100.0f, kEps);
    EXPECT_NEAR(EffectWindowSize(50.0f, 2.0f), 200.0f, kEps);
    EXPECT_NEAR(EffectWindowSize(50.0f, 0.5f), 50.0f, kEps);
}

TEST(EffectWindowLogic, SizeIsClampedToTheMinimum) {
    // A 1px-radius effect would otherwise get a 2px window.
    EXPECT_NEAR(EffectWindowSize(1.0f, 1.0f), kEffectWindowMinSize, kEps);
    EXPECT_NEAR(EffectWindowSize(0.0f, 1.0f), kEffectWindowMinSize, kEps);
    // The clamp applies AFTER scaling, so shrinking can hit the floor too.
    EXPECT_NEAR(EffectWindowSize(50.0f, 0.01f), kEffectWindowMinSize, kEps);
}

TEST(EffectWindowLogic, SizeIsNeverBelowTheFloorForAnyInput) {
    for (float r : {0.0f, 0.5f, 5.0f, 19.0f, 20.0f, 21.0f, 100.0f}) {
        for (float s : {0.1f, 0.5f, 1.0f, 3.0f}) {
            EXPECT_GE(EffectWindowSize(r, s), kEffectWindowMinSize)
                << "radius=" << r << " scale=" << s;
        }
    }
}

// ── Frame computation ──────────────────────────────────────

TEST(EffectWindowLogic, FrameIsSquareAndCentredOnTheEffect) {
    constexpr float screenH = 1000.0f;
    auto f = ComputeEffectFrame(500.0f, 400.0f, 50.0f, 1.0f, screenH);

    EXPECT_NEAR(f.size, 100.0f, kEps);
    // Origin is bottom-left, so the centre sits half a size up and right.
    EXPECT_NEAR(f.origin.x + f.size * 0.5f, 500.0f, kEps)
        << "frame should be horizontally centred on the effect";
}

TEST(EffectWindowLogic, FrameFlipsDeviceYIntoScreenY) {
    constexpr float screenH = 1000.0f;
    auto top = ComputeEffectFrame(500.0f, 10.0f, 50.0f, 1.0f, screenH);
    auto bottom = ComputeEffectFrame(500.0f, 990.0f, 50.0f, 1.0f, screenH);

    EXPECT_GT(top.origin.y, bottom.origin.y)
        << "DEVICE y grows downward, SCREEN y grows upward";
}

TEST(EffectWindowLogic, FrameGrowsWithScale) {
    auto small = ComputeEffectFrame(500.0f, 500.0f, 50.0f, 1.0f, 1000.0f);
    auto large = ComputeEffectFrame(500.0f, 500.0f, 50.0f, 2.0f, 1000.0f);

    EXPECT_NEAR(large.size, small.size * 2.0f, kEps);
    // Growing around a fixed centre must move the origin back, not forward.
    EXPECT_LT(large.origin.x, small.origin.x);
}

TEST(EffectWindowLogic, TinyEffectStillGetsTheMinimumSizedFrame) {
    auto f = ComputeEffectFrame(100.0f, 100.0f, 0.5f, 1.0f, 1000.0f);
    EXPECT_NEAR(f.size, kEffectWindowMinSize, kEps);
    EXPECT_NEAR(f.origin.x + f.size * 0.5f, 100.0f, kEps)
        << "the clamped frame must still be centred on the effect";
}

// ── Movement gate ──────────────────────────────────────────

TEST(EffectWindowLogic, FirstUpdateAlwaysRuns) {
    // The regression this pins: effect_window.mm tracked hasLastPosition but
    // never read it, comparing against zero-initialized ivars instead. An
    // effect spawning at DEVICE (0,0) therefore skipped its first reposition.
    EXPECT_TRUE(ShouldUpdatePosition({0.0f, 0.0f}, {0.0f, 0.0f},
                                     /*hasLastPosition=*/false))
        << "an effect at the origin must still get its first frame update";
    EXPECT_TRUE(ShouldUpdatePosition({0.0f, 0.0f}, {500.0f, 500.0f}, false));
}

TEST(EffectWindowLogic, UnmovedEffectSkipsTheUpdate) {
    EXPECT_FALSE(ShouldUpdatePosition({100.0f, 200.0f}, {100.0f, 200.0f}, true));
    EXPECT_FALSE(ShouldUpdatePosition({100.0f, 200.0f}, {100.05f, 199.95f}, true))
        << "sub-0.1px drift must not trigger a reposition";
}

TEST(EffectWindowLogic, MovementBeyondEpsilonTriggersTheUpdate) {
    EXPECT_TRUE(ShouldUpdatePosition({100.0f, 200.0f}, {100.5f, 200.0f}, true));
    EXPECT_TRUE(ShouldUpdatePosition({100.0f, 200.0f}, {100.0f, 199.0f}, true))
        << "either axis alone, in either direction, is enough";
}

// ── Identity matching ──────────────────────────────────────

TEST(EffectWindowLogic, PositionsMatchWithinOnePixel) {
    EXPECT_TRUE(PositionsMatch({100.0f, 200.0f}, {100.0f, 200.0f}));
    EXPECT_TRUE(PositionsMatch({100.0f, 200.0f}, {100.9f, 199.1f}));
    EXPECT_FALSE(PositionsMatch({100.0f, 200.0f}, {101.5f, 200.0f}));
    EXPECT_FALSE(PositionsMatch({100.0f, 200.0f}, {100.0f, 202.0f}));
}

TEST(EffectWindowLogic, MatchToleranceIsLooserThanTheMovementGate) {
    // Identity matching ("same effect?") must not be as twitchy as motion
    // detection, or syncWindows would spawn duplicate windows on tiny drift.
    const DevicePoint a{100.0f, 100.0f};
    const DevicePoint b{100.5f, 100.0f};
    EXPECT_TRUE(PositionsMatch(a, b)) << "still the same effect";
    EXPECT_TRUE(ShouldUpdatePosition(a, b, true)) << "but it has moved enough to reposition";
}

// ── Ring bookkeeping ───────────────────────────────────────

TEST(EffectWindowLogic, SlotAtWalksFromTheOldest) {
    EXPECT_EQ(SlotAt(0, 0, 50), 0u);
    EXPECT_EQ(SlotAt(0, 5, 50), 5u);
    EXPECT_EQ(SlotAt(10, 3, 50), 13u);
}

TEST(EffectWindowLogic, SlotAtWrapsAroundCapacity) {
    EXPECT_EQ(SlotAt(48, 5, 50), 3u) << "48+5 = 53 -> wraps to 3";
    EXPECT_EQ(SlotAt(49, 1, 50), 0u);
}

TEST(EffectWindowLogic, InsertionAppendsWhileThereIsRoom) {
    auto p = PlanInsertion(/*head=*/0, /*count=*/0, 50);
    EXPECT_EQ(p.slot, 0u);
    EXPECT_EQ(p.newHead, 0u);
    EXPECT_EQ(p.newCount, 1u);
    EXPECT_FALSE(p.evictsOldest);

    auto q = PlanInsertion(0, 7, 50);
    EXPECT_EQ(q.slot, 7u);
    EXPECT_EQ(q.newCount, 8u);
    EXPECT_FALSE(q.evictsOldest);
}

TEST(EffectWindowLogic, InsertionAppendsPastAWrappedHead) {
    // head=45, count=7 -> next free slot is (45+7)%50 = 2.
    auto p = PlanInsertion(45, 7, 50);
    EXPECT_EQ(p.slot, 2u);
    EXPECT_EQ(p.newHead, 45u) << "appending must not move head";
    EXPECT_EQ(p.newCount, 8u);
    EXPECT_FALSE(p.evictsOldest);
}

TEST(EffectWindowLogic, FullRingEvictsTheOldestAndAdvancesHead) {
    auto p = PlanInsertion(/*head=*/0, /*count=*/50, 50);
    EXPECT_TRUE(p.evictsOldest);
    EXPECT_EQ(p.slot, 0u) << "the oldest lives at head";
    EXPECT_EQ(p.newHead, 1u);
    EXPECT_EQ(p.newCount, 50u) << "count stays pinned at capacity";
}

TEST(EffectWindowLogic, FullRingWrapsHeadAtTheEnd) {
    auto p = PlanInsertion(/*head=*/49, /*count=*/50, 50);
    EXPECT_TRUE(p.evictsOldest);
    EXPECT_EQ(p.slot, 49u);
    EXPECT_EQ(p.newHead, 0u) << "head must wrap, not run off the end";
}

TEST(EffectWindowLogic, RingNeverHandsOutAnOutOfRangeSlot) {
    // Drive a full lifetime: fill, then overflow well past capacity, and check
    // every slot stays addressable and count never exceeds capacity.
    constexpr std::size_t cap = 50;
    std::size_t head = 0, count = 0;
    for (int i = 0; i < 300; ++i) {
        auto p = PlanInsertion(head, count, cap);
        ASSERT_LT(p.slot, cap) << "slot out of range at insertion " << i;
        ASSERT_LT(p.newHead, cap) << "head out of range at insertion " << i;
        ASSERT_LE(p.newCount, cap) << "count exceeded capacity at insertion " << i;
        head = p.newHead;
        count = p.newCount;
    }
    EXPECT_EQ(count, cap);
}

TEST(EffectWindowLogic, RingHoldsExactlyTheMostRecentEntries) {
    // Simulate the array the manager keeps and confirm that after overflowing,
    // walking from head yields the last `capacity` insertions in order.
    constexpr std::size_t cap = 50;
    std::vector<int> slots(cap, -1);
    std::size_t head = 0, count = 0;

    for (int value = 0; value < 130; ++value) {
        auto p = PlanInsertion(head, count, cap);
        slots[p.slot] = value;
        head = p.newHead;
        count = p.newCount;
    }

    std::vector<int> inOrder;
    for (std::size_t i = 0; i < count; ++i) inOrder.push_back(slots[SlotAt(head, i, cap)]);

    ASSERT_EQ(inOrder.size(), cap);
    EXPECT_EQ(inOrder.front(), 80) << "oldest surviving entry should be 130-50";
    EXPECT_EQ(inOrder.back(), 129) << "newest entry should be last";
    for (std::size_t i = 1; i < inOrder.size(); ++i) {
        ASSERT_EQ(inOrder[i], inOrder[i - 1] + 1) << "ring order broken at " << i;
    }
}

TEST(EffectWindowLogic, EveryLiveSlotIsDistinct) {
    constexpr std::size_t cap = 50;
    std::size_t head = 0, count = 0;
    for (int i = 0; i < 137; ++i) {
        auto p = PlanInsertion(head, count, cap);
        head = p.newHead;
        count = p.newCount;
    }
    std::set<std::size_t> seen;
    for (std::size_t i = 0; i < count; ++i) seen.insert(SlotAt(head, i, cap));
    EXPECT_EQ(seen.size(), count) << "two live windows would share a slot";
}
