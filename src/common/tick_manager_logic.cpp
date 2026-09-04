#include "tick_manager_logic.h"

namespace tick_manager_logic {

namespace {
constexpr int kWorldCleanupTickInterval = 60;
constexpr int kLeafSpawnFirstRunBurst = 3;
constexpr int kLeafSpawnProbabilityDenominator = 500;
}  // namespace

bool ShouldRunWorldCleanup(int tickCount) {
    return tickCount > 0 && tickCount % kWorldCleanupTickInterval == 0;
}

int NextLeafSpawn(bool& initialized, bool enabled, int roll) {
    if (!enabled) return 0;
    if (!initialized) {
        initialized = true;
        return kLeafSpawnFirstRunBurst;
    }
    return (roll == 0) ? 1 : 0;  // roll is caller-supplied RandRange(denominator)
}

}  // namespace tick_manager_logic
