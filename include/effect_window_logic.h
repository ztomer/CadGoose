#pragma once

// effect_window_logic — the pure decision/geometry layer behind EffectWindow
// and EffectWindowManager.
//
// EffectWindow is an NSWindow subclass and EffectWindowManager owns a fixed
// capacity ring of them, so none of it is reachable from a headless test.
// Everything here is the part that is just arithmetic — window sizing, the
// DEVICE->SCREEN frame, the movement gate, and the ring's slot bookkeeping —
// lifted out so it can be tested directly. effect_window.mm keeps the AppKit.

#include "coordinate_system.h"
#include <cstddef>

namespace effect_window_logic {

// Ring capacity for live effect windows, and the floor on window size. Both
// were file-local constants in effect_window.mm.
inline constexpr std::size_t kMaxEffectWindows = 50;
inline constexpr float kEffectWindowMinSize = 40.0f;

// Effect windows are square. Diameter scaled by globalScale, clamped so tiny
// effects still get a usable window.
float EffectWindowSize(float radius, float globalScale);

struct EffectFrame {
    ScreenPoint origin;  // bottom-left, SCREEN space
    float size;          // square: width == height
};

// The window frame for an effect at a DEVICE position.
EffectFrame ComputeEffectFrame(float posX, float posY, float radius,
                               float globalScale, float screenHeight);

// -updatePosition skips work when the effect has not meaningfully moved.
//
// NOTE: `hasLastPosition` is load-bearing. effect_window.mm previously tracked
// that flag but never READ it, comparing instead against zero-initialized
// last-position ivars — so an effect spawning near DEVICE (0,0) silently
// skipped its first reposition. Callers must pass the flag through.
bool ShouldUpdatePosition(DevicePoint lastPos, DevicePoint newPos, bool hasLastPosition);

// Whether an existing window already covers this effect position. -syncWindows
// uses a looser tolerance than the movement gate: it is matching identity
// ("is this the same effect?"), not detecting motion.
bool PositionsMatch(DevicePoint a, DevicePoint b);

// ── Ring bookkeeping ──────────────────────────────────────
// The manager stores windows in a fixed-size array used as a circular buffer.
// These are the index computations; the manager does the AppKit work.

// Slot for the i-th live window, counting from the oldest.
std::size_t SlotAt(std::size_t head, std::size_t i, std::size_t capacity);

struct Insertion {
    std::size_t slot;        // where the new window goes
    std::size_t newHead;     // head after the insertion
    std::size_t newCount;    // count after the insertion
    bool evictsOldest;       // true when the ring was full and slot held a live window
};

// Where the next window goes. When the ring is full this evicts the oldest and
// advances head; otherwise it appends and grows count.
Insertion PlanInsertion(std::size_t head, std::size_t count, std::size_t capacity);

}  // namespace effect_window_logic
