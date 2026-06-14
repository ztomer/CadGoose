#include <gtest/gtest.h>
#include "random_util.h"

TEST(RandomUtil, SeedDeterministic) {
    rng_util::Seed(42);
    int a = rng_util::RandRange(10000);
    rng_util::Seed(42);
    int b = rng_util::RandRange(10000);
    EXPECT_EQ(a, b);
}

TEST(RandomUtil, RandRangeZero) {
    EXPECT_EQ(rng_util::RandRange(0), 0);
}

TEST(RandomUtil, RandRangeNegative) {
    EXPECT_EQ(rng_util::RandRange(-5), 0);
}

TEST(RandomUtil, RandRangeOne) {
    EXPECT_EQ(rng_util::RandRange(1), 0);
}

TEST(RandomUtil, RandRangeBounds) {
    for (int i = 0; i < 100; i++) {
        int r = rng_util::RandRange(10);
        EXPECT_GE(r, 0);
        EXPECT_LT(r, 10);
    }
}

TEST(RandomUtil, RandIntRangeNormal) {
    for (int i = 0; i < 100; i++) {
        int r = rng_util::RandIntRange(5, 10);
        EXPECT_GE(r, 5);
        EXPECT_LE(r, 10);
    }
}

TEST(RandomUtil, RandIntRangeReversed) {
    EXPECT_EQ(rng_util::RandIntRange(10, 5), 10);
}

TEST(RandomUtil, RandIntRangeSingle) {
    EXPECT_EQ(rng_util::RandIntRange(7, 7), 7);
}

TEST(RandomUtil, Rand01Bounds) {
    for (int i = 0; i < 100; i++) {
        double r = rng_util::Rand01();
        EXPECT_GE(r, 0.0);
        EXPECT_LT(r, 1.0);
    }
}

TEST(RandomUtil, RandBool) {
    int trues = 0;
    for (int i = 0; i < 1000; i++) {
        if (rng_util::RandBool()) trues++;
    }
    EXPECT_GT(trues, 100);
    EXPECT_LT(trues, 900);
}

TEST(RandomUtil, RandFloatRange) {
    for (int i = 0; i < 100; i++) {
        float r = rng_util::RandFloatRange(5.0f, 10.0f);
        EXPECT_GE(r, 5.0f);
        EXPECT_LT(r, 10.0f);
    }
}

TEST(RandomUtil, RandFloatRangeReversed) {
    EXPECT_FLOAT_EQ(rng_util::RandFloatRange(10.0f, 5.0f), 10.0f);
}
