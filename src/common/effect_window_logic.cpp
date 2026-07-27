#include "effect_window_logic.h"

#include <algorithm>
#include <cmath>

namespace effect_window_logic {

float EffectWindowSize(float radius, float globalScale) {
    const float size = radius * 2.0f * globalScale;
    return std::max(size, kEffectWindowMinSize);
}

EffectFrame ComputeEffectFrame(float posX, float posY, float radius,
                               float globalScale, float screenHeight) {
    const float size = EffectWindowSize(radius, globalScale);
    const ScreenPoint centre = CoordTransform::DeviceToScreenMacOS({posX, posY}, screenHeight);
    // The frame origin is the bottom-left, so step back by half the size from
    // the effect's centre.
    return {{centre.x - size * 0.5f, centre.y - size * 0.5f}, size};
}

bool ShouldUpdatePosition(DevicePoint lastPos, DevicePoint newPos, bool hasLastPosition) {
    // Without a previous frame there is nothing to compare against, so the
    // first update must always run. Skipping this check is what made an effect
    // at DEVICE (0,0) match the zero-initialized "last position" and stall.
    if (!hasLastPosition) return true;
    constexpr float kEpsilon = 0.1f;
    return std::abs(newPos.x - lastPos.x) >= kEpsilon ||
           std::abs(newPos.y - lastPos.y) >= kEpsilon;
}

bool PositionsMatch(DevicePoint a, DevicePoint b) {
    constexpr float kTolerance = 1.0f;
    return std::abs(a.x - b.x) < kTolerance && std::abs(a.y - b.y) < kTolerance;
}

std::size_t SlotAt(std::size_t head, std::size_t i, std::size_t capacity) {
    return (head + i) % capacity;
}

Insertion PlanInsertion(std::size_t head, std::size_t count, std::size_t capacity) {
    if (count >= capacity) {
        // Full: the oldest window lives at head. Overwrite it and step head on,
        // so the ring keeps the most recent `capacity` effects.
        return {head, (head + 1) % capacity, capacity, true};
    }
    return {(head + count) % capacity, head, count + 1, false};
}

}  // namespace effect_window_logic
