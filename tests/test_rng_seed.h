#ifndef CADGOOSE_TESTS_TEST_RNG_SEED_H
#define CADGOOSE_TESTS_TEST_RNG_SEED_H

// Every test starts from the same random sequence, whatever ran before it.
//
// rng_util's engine is seeded from std::random_device, so any test whose path
// passes through RandRange was a coin toss: GooseStateMachine.FetchTimeoutWorks
// failed 2 times in 5000 because, in the same tick as the timeout, the goose
// picked a random wander target that happened to be "reached" and rolled a new
// TEXT fetch. Seeding from the test's own name (FNV-1a, stable across runs and
// orders) makes each test's draws a pure function of the test: a test that
// passes, passes every time, in any order. A test that seeds itself still can.

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

uint32_t TestRngSeedFor(const std::string& fullName);

class TestRngSeeder : public ::testing::EmptyTestEventListener {
public:
    void OnTestStart(const ::testing::TestInfo& info) override;
};

#endif
