#pragma once

// tick_manager_logic — the pure decision layer behind TickManager.
//
// TickManager (src/platform/macos/tick_manager.mm) drives CADisplayLink and
// AppKit windows, so nothing inside its -tick is reachable from a headless
// test. The cadence/state decisions it makes each frame are lifted here so
// tests pin them directly; tick_manager.mm keeps only the AppKit execution.

namespace tick_manager_logic {

// World_CleanupExpired runs on this cadence, not every frame.
bool ShouldRunWorldCleanup(int tickCount);

// Leaf-spawn state machine. On the FIRST enabled frame after launch, three
// piles burst in; afterwards each enabled frame rolls 1-in-N for a new pile.
//
// `initialized`  — in/out flag mirroring TickManager's static
// `enabled`      — autumnLeaves config on this frame
// Returns how many piles to spawn this frame (0, 1, or the first-run burst).
int NextLeafSpawn(bool& initialized, bool enabled, int roll);

}  // namespace tick_manager_logic
