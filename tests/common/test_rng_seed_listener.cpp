#include <gtest/gtest.h>

#include <random>

#include "../test_rng_seed.h"
#include "random_util.h"

// The seeder is a listener in main(); these tests see its effect from inside.
TEST(TestRngSeed, EachTestStartsFromTheSeedOfItsName) {
    std::mt19937 expected(TestRngSeedFor("TestRngSeed.EachTestStartsFromTheSeedOfItsName"));
    EXPECT_EQ(rng_util::Engine()(), expected());
}

TEST(TestRngSeed, SeedsDifferByName) {
    EXPECT_NE(TestRngSeedFor("A.b"), TestRngSeedFor("A.c"));
    EXPECT_EQ(TestRngSeedFor("A.b"), TestRngSeedFor("A.b"));
}
