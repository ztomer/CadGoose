# Plan — May 21, 2026

## Completed
1. **Actor System (R1)** — All 9 actors migrated: Ball, Toy, Flower, Jail, Portal, Breadcrumb, LeafPile, Goose, DroppedItem.
2. **EffectWindow Registration (R2)** — Each effect type self-registers from its own file.
3. **Goose as Actor** — `Goose` extends `Actor`, `renderer.mm` uses `ActorManager::tickAll()` and `ActorManager::renderAll()`.
4. **Phase 2 Cleanup** — All legacy stubs, dead effect_reg files, `effect_window.mm` dead code, and `g_geese` iterations migrated to `ActorManager::getGeese()`.
5. **behavior.h split** — Split into 4 files: `behavior_state.h`, `behavior_manager.h`, `behavior_registry.h`, `behavior_api.h`.
6. **Goose monolith deconstructed** — `Update()` split into `UpdatePhysics()`, `UpdateDetection()`, `UpdateAnimation()`, `UpdateDebug()`.
7. **g_world.droppedItems → DroppedItemActor** — All files migrated to `ActorManager::getDroppedItems()`.
8. **Leaf pile scaling fixed** — `actor_leafpile.mm` applies `globalScale` to window size, leaf positions, and leaf ellipse dimensions.
9. **Footprint/PomodoroBed effect registration fixed** — Used actual `EffectType` enum values instead of hardcoded magic numbers.
10. **Behavior toggle system fixed** — `BehaviorRegistry::TickAll` detects enabled↔disabled transitions at runtime.
11. **Pomodoro quiet during rest** — `triggerHonk` returns early when `goose.isResting` is true.
12. **Actor window lifecycle crashes fixed** — `__bridge_transfer` inside dispatch block in all 8 actors.
13. **Ball flicker/reset fixed** — Removed position reset in `behavior_ball.mm:init()`.
14. **ASan disabled by default** — Conflicts with ObjC runtime on macOS 26.x.
15. **UI regressions fixed** — Leaf size, breadcrumb size, preferences row height, meme fetch probability.
16. **Leaf window dynamic resizing** — Bounding box calculation per frame to prevent clipping when kicked.
17. **Drag trails fix** — `setNeedsDisplay:YES` in `ItemWindow::mouseDragged`.
18. **Window trail investigation** — Trail occurs when goose drops item window (not user drag). Root cause: `orderFront` vs `drawRect` race. Off-screen creation fix applied. See `docs/WINDOW_TRAIL_DEBUG.md`.
19. **Dead code cleanup** — 53 items removed: `behavior_mac.mm` (331 LOC), unused config fields, dead declarations, `Goose::stepTime`.
20. **Generic API adoption** — `Actor::closeWindowOnMainThread` used by all 8 actors.
21. **Compilation warnings fixed** — 5 warnings across 4 files eliminated.
22. **Autumn leaves improvements** — Spawn rate reduced 3x, lifetime 180s, fade 20s.
23. **Trail detection system** — `test_window_trail.mm` created with 5 tests to verify fixes.

## Pending

### High Priority
- **Window trail fix verification** — User needs to test off-screen window creation fix. If trails persist, see `docs/WINDOW_TRAIL_DEBUG.md` for next approaches.

### Medium Priority
- CPU profiling — 46% idle CPU usage needs Instruments investigation
- Linux build verification — config field removals may affect Linux compilation
