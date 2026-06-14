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
