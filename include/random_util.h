// random_util.h
// Unbiased game-RNG helpers.
//
// Replaces the historical `rand() % N` idiom, which is biased for any N
// that doesn't evenly divide RAND_MAX+1. The bias is small at game-scale
// Ns but the helpers are no slower than the old code (one shared engine,
// zero allocation), so there's no reason to keep using the biased form.
//
// All helpers are thread-safe (per-thread engine, lazy-seeded from
// std::random_device on first use). For deterministic tests use
// rng_util::Seed(seed) on the relevant thread.

#pragma once

#include <random>

namespace rng_util {

inline std::mt19937& Engine() {
    thread_local std::mt19937 engine{std::random_device{}()};
    return engine;
}

// Re-seed the current thread's engine. Use in tests / when reproducibility
// matters; production code can ignore this entirely.
inline void Seed(uint32_t s) {
    Engine().seed(s);
}

// Uniform integer in [0, exclusiveMax). Returns 0 if exclusiveMax <= 0.
inline int RandRange(int exclusiveMax) {
    if (exclusiveMax <= 0) return 0;
    std::uniform_int_distribution<int> dist(0, exclusiveMax - 1);
    return dist(Engine());
}

// Uniform integer in [lo, hiInclusive]. Returns lo if hi < lo.
inline int RandIntRange(int lo, int hiInclusive) {
    if (hiInclusive < lo) return lo;
    std::uniform_int_distribution<int> dist(lo, hiInclusive);
    return dist(Engine());
}

// Uniform double in [0.0, 1.0).
inline double Rand01() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(Engine());
}

// Uniform float in [lo, hi).
inline float RandFloatRange(float lo, float hi) {
    if (hi <= lo) return lo;
    std::uniform_real_distribution<float> dist(lo, hi);
    return dist(Engine());
}

// 50/50 coin flip.
inline bool RandBool() {
    return RandRange(2) == 0;
}

} // namespace rng_util
