#include "timer.h"
#include <gtest/gtest.h>

TEST(Timer, InitialState) {
    Timer t;
    EXPECT_FALSE(t.running);
    EXPECT_EQ(t.startTime, 0.0);
    EXPECT_EQ(t.Elapsed(10.0), 0.0);
    EXPECT_FALSE(t.IsExpired(10.0, 5.0));
}

TEST(Timer, StartAndElapsed) {
    Timer t;
    t.Start(10.0);
    EXPECT_TRUE(t.running);
    EXPECT_EQ(t.startTime, 10.0);
    EXPECT_EQ(t.Elapsed(12.0), 2.0);
    EXPECT_EQ(t.Elapsed(15.5), 5.5);
}

TEST(Timer, IsExpired) {
    Timer t;
    t.Start(10.0);
    EXPECT_FALSE(t.IsExpired(12.0, 5.0));
    EXPECT_TRUE(t.IsExpired(15.0, 5.0));
    EXPECT_TRUE(t.IsExpired(20.0, 5.0));
}

TEST(Timer, Stop) {
    Timer t;
    t.Start(10.0);
    t.Stop();
    EXPECT_FALSE(t.running);
    EXPECT_EQ(t.Elapsed(20.0), 0.0);
    EXPECT_FALSE(t.IsExpired(20.0, 5.0));
}

TEST(Timer, Reset) {
    Timer t;
    t.Start(10.0);
    t.Reset();
    EXPECT_FALSE(t.running);
    EXPECT_EQ(t.startTime, 0.0);
}

TEST(Timer, Remaining) {
    Timer t;
    t.Start(10.0);
    EXPECT_EQ(t.Remaining(12.0, 5.0), 3.0);
    EXPECT_EQ(t.Remaining(15.0, 5.0), 0.0);
    EXPECT_EQ(t.Remaining(20.0, 5.0), 0.0);
}

TEST(Timer, Progress) {
    Timer t;
    t.Start(10.0);
    EXPECT_FLOAT_EQ(t.Progress(10.0, 10.0), 0.0f);
    EXPECT_FLOAT_EQ(t.Progress(15.0, 10.0), 0.5f);
    EXPECT_FLOAT_EQ(t.Progress(20.0, 10.0), 1.0f);
    EXPECT_FLOAT_EQ(t.Progress(25.0, 10.0), 1.0f);
}

TEST(Timer, ProgressNotRunning) {
    Timer t;
    EXPECT_FLOAT_EQ(t.Progress(10.0, 5.0), 1.0f);
}

TEST(Timer, ZeroDuration) {
    Timer t;
    t.Start(10.0);
    EXPECT_FLOAT_EQ(t.Progress(10.0, 0.0), 1.0f);
    EXPECT_TRUE(t.IsExpired(10.0, 0.0));
}

TEST(Timer, Restart) {
    Timer t;
    t.Start(10.0);
    EXPECT_TRUE(t.IsExpired(15.0, 5.0));
    t.Start(20.0);
    EXPECT_FALSE(t.IsExpired(22.0, 5.0));
    EXPECT_TRUE(t.IsExpired(25.0, 5.0));
}
