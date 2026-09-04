#include <gtest/gtest.h>
#include "ring_buffer.h"

TEST(RingBuffer, PushAndFront) {
    RingBuffer<int, 4> buf;
    buf.push(10);
    EXPECT_EQ(buf.front(), 10);
    EXPECT_EQ(buf.back(), 10);
}

TEST(RingBuffer, PushMultiple) {
    RingBuffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    EXPECT_EQ(buf.front(), 1);
    EXPECT_EQ(buf.back(), 2);
}

TEST(RingBuffer, Pop) {
    RingBuffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.pop();
    EXPECT_EQ(buf.front(), 2);
}

TEST(RingBuffer, EmptyAfterPop) {
    RingBuffer<int, 4> buf;
    buf.push(1);
    buf.pop();
    EXPECT_TRUE(buf.empty());
}

TEST(RingBuffer, Overflow) {
    RingBuffer<int, 3> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    EXPECT_TRUE(buf.isFull());
    buf.push(4);
    EXPECT_TRUE(buf.isFull());
    EXPECT_EQ(buf.front(), 2);
    EXPECT_EQ(buf.back(), 4);
}

TEST(RingBuffer, Size) {
    RingBuffer<int, 5> buf;
    EXPECT_EQ(buf.size(), 0u);
    buf.push(1);
    EXPECT_EQ(buf.size(), 1u);
    buf.push(2);
    EXPECT_EQ(buf.size(), 2u);
    buf.pop();
    EXPECT_EQ(buf.size(), 1u);
}

TEST(RingBuffer, FullSize) {
    RingBuffer<int, 3> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    EXPECT_EQ(buf.size(), 3u);
}

TEST(RingBuffer, Clear) {
    RingBuffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.clear();
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
}

TEST(RingBuffer, IndexAccess) {
    RingBuffer<int, 4> buf;
    buf.push(10);
    buf.push(20);
    EXPECT_EQ(buf[0], 10);
    EXPECT_EQ(buf[1], 20);
}

TEST(RingBuffer, ConstFrontBack) {
    RingBuffer<int, 4> buf;
    buf.push(42);
    const auto& cbuf = buf;
    EXPECT_EQ(cbuf.front(), 42);
    EXPECT_EQ(cbuf.back(), 42);
}

TEST(RingBuffer, Iteration) {
    RingBuffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    int sum = 0;
    for (auto it = buf.begin(); it != buf.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 6);
}

TEST(RingBuffer, ConstIteration) {
    RingBuffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    const auto& cbuf = buf;
    int sum = 0;
    for (auto it = cbuf.begin(); it != cbuf.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 6);
}

TEST(RingBuffer, Capacity) {
    EXPECT_EQ((RingBuffer<int, 4>::capacity()), 4u);
    EXPECT_EQ((RingBuffer<int, 8>::capacity()), 8u);
}

// ── Oracle: edge cases ──

TEST(RingBuffer, FrontAndBackOnEmptyIsSafe) {
    RingBuffer<int, 4> buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
    // back() and front() on empty access stale slots — the call itself
    // doesn't crash (no allocation), but the value is indeterminate.
    // Callers MUST guard with !empty() first.
    (void)buf.front();
    (void)buf.back();
}

TEST(RingBuffer, PopOnEmptyIsNoOp) {
    RingBuffer<int, 4> buf;
    buf.pop();
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
}

TEST(RingBuffer, FullBufferBackDoesNotStraddle) {
    RingBuffer<int, 3> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    EXPECT_TRUE(buf.isFull());
    EXPECT_EQ(buf.back(), 3);
    EXPECT_EQ(buf.front(), 1);
}

TEST(RingBuffer, WrapBackAccessesCorrectSlot) {
    RingBuffer<int, 4> buf;
    buf.push(10);
    buf.push(20);
    buf.push(30);
    buf.push(40);  // full: head=0, tail=0
    buf.push(50);  // overwrites buf[0] → head=1, tail=1
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf.front(), 20);  // oldest surviving
    EXPECT_EQ(buf.back(), 50);   // newest
}

TEST(RingBuffer, PopFromFullPreservesRemaining) {
    RingBuffer<int, 3> buf;
    buf.push(10);
    buf.push(20);
    buf.push(30);
    buf.pop();
    EXPECT_EQ(buf.size(), 2u);
    EXPECT_EQ(buf.front(), 20);
    EXPECT_EQ(buf.back(), 30);
}

TEST(RingBuffer, WrapAroundSizeIsAccurate) {
    RingBuffer<int, 4> buf;
    for (int i = 1; i <= 6; i++) {
        buf.push(i * 10);
    }
    // 6 pushes into capacity 4 = 4 elements: [30, 40, 50, 60]
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf.front(), 30);
    EXPECT_EQ(buf[0], 30);
    EXPECT_EQ(buf[1], 40);
    EXPECT_EQ(buf[2], 50);
    EXPECT_EQ(buf[3], 60);
}

TEST(RingBuffer, WrapThenPopAndPush) {
    RingBuffer<int, 4> buf;
    for (int i = 1; i <= 6; i++) buf.push(i);
    EXPECT_EQ(buf.front(), 3);
    buf.pop();                // removes 3
    EXPECT_EQ(buf.front(), 4);
    buf.push(7);              // adds 7
    EXPECT_EQ(buf.back(), 7);
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf[0], 4);
    EXPECT_EQ(buf[1], 5);
    EXPECT_EQ(buf[2], 6);
    EXPECT_EQ(buf[3], 7);
}

TEST(RingBuffer, WrapIterator) {
    RingBuffer<int, 3> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    buf.push(4);  // wrap: [2, 3, 4]
    int sum = 0;
    for (auto& v : buf) sum += v;
    EXPECT_EQ(sum, 2 + 3 + 4);
}

TEST(RingBuffer, ClearAfterWrap) {
    RingBuffer<int, 3> buf;
    for (int i = 1; i <= 4; i++) buf.push(i);
    buf.clear();
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
    buf.push(42);
    EXPECT_EQ(buf.front(), 42);
    EXPECT_EQ(buf.back(), 42);
}

TEST(RingBuffer, SizeNeverExceedsCapacity) {
    RingBuffer<int, 5> buf;
    for (int i = 0; i < 100; i++) {
        buf.push(i);
        EXPECT_LE(buf.size(), 5u);
    }
}

TEST(RingBuffer, BackOnSingleElementAfterWrap) {
    RingBuffer<int, 4> buf;
    for (int i = 1; i <= 5; i++) buf.push(i);
    buf.pop();
    buf.pop();
    buf.pop();
    // buf: [5] only
    EXPECT_EQ(buf.size(), 1u);
    EXPECT_EQ(buf.front(), 5);
    EXPECT_EQ(buf.back(), 5);
}
