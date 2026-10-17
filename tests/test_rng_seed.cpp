#include "test_rng_seed.h"

#include "random_util.h"

uint32_t TestRngSeedFor(const std::string& fullName) {
    uint32_t h = 2166136261u;  // FNV-1a, 32-bit
    for (unsigned char c : fullName) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

void TestRngSeeder::OnTestStart(const ::testing::TestInfo& info) {
    rng_util::Seed(TestRngSeedFor(std::string(info.test_suite_name()) + "." + info.name()));
}
