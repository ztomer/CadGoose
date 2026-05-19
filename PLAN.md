# Plan — May 18, 2026

## Completed
1. **Double meme on second drag** — Fixed by always calling `syncWindows` in renderer.mm.
2. **Crash (EXC_BAD_ACCESS in AutoreleasePoolPop)** — Root cause: `DaemonizeProcess()` used `fork()` on macOS. Fixed by replacing `fork()` with `posix_spawn()`.
3. **Ball behavior → own window** — Ball now uses `BehaviorElementWindow`.
4. **Crumb → EffectWindow** — Migrated to independent `EffectWindow` windows.
5. **Portals → EffectWindow** — Migrated to independent `EffectWindow` windows.
6. **Ball cursor hit-test** — Ball window sized to `ball->radius * 2`, cursor kick uses circle hit-test.
7. **Autumn leaves interaction** — Leaf pile kick proximity uses `g_config.render.footSize`.
8. **NSWindow crash fix** — `releasedWhenClosed = NO` set on all dictionary-managed windows.
9. **Window migration audit** — Full audit of 20 behaviors: 4 candidates for independent windows identified.
10. **Jails → EffectWindow** — Migrated to independent `EffectWindow` windows.
11. **Pomodoro bed → EffectWindow** — Bed migrates to independent `EffectWindow`.
12. **Toys → EffectWindow** — Migrated to independent `EffectWindow` windows.
13. **Flowers → EffectWindow** — Migrated to independent `EffectWindow` windows.
14. **Pomodoro timer centering** — Timer text uses precise centering.
15. **UI polish** — N1-N5 completed.
16. **Actor System (R1)** — All 9 actors migrated: Ball, Toy, Flower, Jail, Portal, Breadcrumb, LeafPile, Goose, DroppedItem.
17. **EffectWindow Registration (R2)** — Each effect type self-registers from its own file.
18. **Goose as Actor** — `Goose` extends `Actor`, `renderer.mm` uses `ActorManager::tickAll()` and `ActorManager::renderAll()`.
19. **Phase 2 Cleanup** — All legacy stubs, dead effect_reg files, `effect_window.mm` dead code, and `g_geese` iterations migrated to `ActorManager::getGeese()`.
20. **Deep Code Review** — Reviewed 6 files >500 LOC, wrote findings to `docs/CODE_REVIEW.md`.
21. **item_window.mm refactoring** — Extracted `IsItemValid()` and `GetMouseDeviceCoords()` helpers. ~110 LOC reduction (17%).
22. **Architecture Phase 2 audit** — All 5 opportunities from `ARCHITECTURE_OPPORTUNITIES.md` verified at Phase 2+.
23. **behavior.h split** — Split into 4 files: `behavior_state.h`, `behavior_manager.h`, `behavior_registry.h`, `behavior_api.h`.
24. **Goose monolith deconstructed** — `Update()` split into `UpdatePhysics()`, `UpdateDetection()`, `UpdateAnimation()`, `UpdateDebug()`.
25. **State includes fixed** — 25 source/test files updated with missing behavior state header includes.
26. **g_world.droppedItems → DroppedItemActor** — Removed `droppedItems` from `WorldContext`. All 20+ files migrated to `ActorManager::getDroppedItems()` / `ActorManager::removeAllDroppedItems()`. Zero references remaining.

## Pending

### High Priority
(none)

## New Files
- `include/actor.h` — Actor base class + ActorManager
- `src/common/actor.cpp` — ActorManager implementation
- `include/actor_ball.h`, `src/common/actor_ball.mm` — BallActor
- `include/actor_toy.h`, `src/common/actor_toy.mm` — ToyActor
- `include/actor_flower.h`, `src/common/actor_flower.mm` — FlowerActor
- `include/actor_jail.h`, `src/common/actor_jail.mm` — JailActor
- `include/actor_portal.h`, `src/common/actor_portal.mm` — PortalActor
- `include/actor_breadcrumb.h`, `src/common/actor_breadcrumb.mm` — BreadcrumbActor
- `include/actor_leafpile.h`, `src/common/actor_leafpile.mm` — LeafPileActor
- `include/actor_dropped_item.h`, `src/common/actor_dropped_item.mm` — DroppedItemActor
- `include/effect_registration.h`, `src/platform/macos/effect_registration.mm` — Effect registration protocol
- `include/behavior_element_window.h`, `src/platform/macos/behavior_element_window.mm` — Behavior element window system
- `include/behavior_state.h` — BehaviorContext, BehaviorStats, BehaviorState base
- `include/behavior_manager.h` — BehaviorStateManager
- `include/behavior_registry.h` — Behavior, BehaviorRegistry, macros
- `include/behavior_api.h` — API function declarations
- `include/behaviors/states/*.h` — 15 individual behavior state headers
- `docs/CODE_REVIEW.md` — Deep code review findings

## Changed Files
- `include/goose.h` — `Goose` extends `Actor`
- `include/renderer_interface.h` — added `nativeContext()` method
- `include/cg_renderer.h` — implemented `nativeContext()`
- `src/common/goose.cpp` — `Goose::tick()`, `Goose::render()`, `UpdatePhysics/Detection/Animation/Debug()`
- `src/common/app_actions.cpp` — `SpawnGoose` adds to ActorManager, `ClearGeese` removes from ActorManager
- `src/common/behaviors/behavior_ball.mm` — delegates to BallActor
- `src/common/behaviors/behavior_toys.cpp` — delegates to ToyActor
- `src/common/behaviors/behavior_interactive_drops.cpp` — delegates to FlowerActor
- `src/common/behaviors/behavior_jail.cpp` — delegates to JailActor
- `src/common/behaviors/behavior_portal.cpp` — delegates to PortalActor
- `src/common/behaviors/behavior_breadcrumbs.cpp` — delegates to BreadcrumbActor
- `src/common/world_utils.mm` — delegates to LeafPileActor
- `src/platform/macos/renderer.mm` — uses `ActorManager::tickAll()` and `ActorManager::renderAll()`
- `src/platform/macos/effect_window.mm` — generic sync via registrations, cleaned dead code
- `src/platform/macos/item_window.mm` — extracted `IsItemValid()` and `GetMouseDeviceCoords()` helpers
- `include/item_window.h` — added `ItemWindowManager` actor window methods
- `CMakeLists.txt` — added all actor files, removed effect_reg_*.mm from test build
- `include/behavior.h` — now includes 4 split headers
- 25 source/test files — added missing behavior state header includes
