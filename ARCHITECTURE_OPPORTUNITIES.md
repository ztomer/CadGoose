# CadGoose Architecture: Opportunities for Improvement

This document tracked a planned migration away from a global-state-heavy, "god-object" design toward a modular Actor-based system built on `EventBus`, `IRenderer`, and `ActorManager`. As of 2026-05-18, four of the five sections are complete; one is intentionally deferred with rationale.

A separate code-review pass ([docs/ARCHITECTURE_REVIEW.md](docs/ARCHITECTURE_REVIEW.md)) added a list of concrete bugs and smells; the actionable items are tracked at the bottom of this doc under [Review follow-ups](#review-follow-ups).

## Status summary

| §  | Topic                                  | Status                                                  |
|---:|----------------------------------------|---------------------------------------------------------|
| 1  | Complete Actor/ECS migration (geese)   | ✅ Done                                                  |
| 2  | Deconstruct the `Goose` monolith       | ⏸ Deferred — impl already file-decomposed; see §2 audit |
| 3  | Strictly enforce `IRenderer`           | ✅ Done                                                  |
| 4  | Overhaul global state (`world.h`)      | ✅ Done (partial-DI policy documented)                   |
| 5  | Decouple behavior state                | ✅ Done (pre-existing; verified by audit)                |

Tests: 722 passing, 32 GUI-only skipped, 1 pre-existing flaky LLM test (`LocalLLMTest.GenerateWithHighTemperatureDoesNotCrash`) — same as baseline.

---

## 1. Complete the Actor/ECS Migration

**Was:** `world.h` owned legacy lists (`g_geese`, `g_droppedItems`) while an `ActorManager` was being introduced — both tracking the same Geese in parallel.

**Now:** `ActorManager` owns every Goose. `g_world.geese` is gone.

- [actor.h](include/actor.h) supports typed iteration (`getGeese()`), generic add/remove, and `destroyAllOfType(type)` for owning cleanup.
- Each game entity has an Actor subclass: BallActor, ToyActor, FlowerActor, JailActor, BreadcrumbActor, LeafPileActor, PortalActor.
- Spawn sites (`app_actions.cpp`, `ui_callbacks.cpp`, tests) heap-allocate Goose via `new` and register with `ActorManager::add`.
- Clear sites (`app_actions.cpp`, `ui_escape.cpp`, tests) call `destroyAllOfType("goose")`.
- Read sites (app_actions, config_save, config_gui_detail, main.mm, ui_callbacks, ui_escape, tests) query `ActorManager::Instance().getGeese()`.
- `std::list<Goose> geese` removed from `WorldContext`.

**Remaining dual-tracking in `WorldContext`**: only `droppedItems`, `footprints`, `crumbs` now. `leafPiles` was removed — it was vestigial dead state after the LeafPileActor migration, and `World_TickLeafPiles` was redundantly second-ticking each pile (now fixed). The remaining three are intentionally kept as value-typed collections because their access patterns (RingBuffer for footprints/crumbs particles; `std::list` with `splice`/`erase-by-iterator` for droppedItems) don't fit the `vector<Actor*>` shape — Actor wrapping would add complexity without benefit.

---

## 2. Deconstruct the `Goose` Monolith — ⏸ DEFERRED

**Original framing:** `Goose` is a "God object" handling physics, animation, state logic, and rendering all at once.

**Audit (2026-05-18):** The `Goose` *class* declaration is still wide (~140 lines, ~50 members) but the *implementation* is already split by responsibility across 8 translation units totalling ~1700 lines:

| File | LOC | Responsibility |
|---|---:|---|
| [goose.cpp](src/common/goose.cpp) | 585 | Lifecycle, tick orchestration, drag, render entry |
| [goose_drawing.mm](src/common/goose_drawing.mm) | 349 | Rendering (rig, held items, eyes) |
| [goose_behaviors_fetch.cpp](src/common/goose_behaviors_fetch.cpp) | 206 | Fetch/return AI |
| [goose_behaviors_interact.cpp](src/common/goose_behaviors_interact.cpp) | 206 | Cursor chase/snatch, joy dodge |
| [goose_behaviors_wander.cpp](src/common/goose_behaviors_wander.cpp) | 153 | Wander/target picking |
| [goose_forces.cpp](src/common/goose_forces.cpp) | 120 | Seek/separation/curve forces |
| [goose_behaviors_internal.cpp](src/common/goose_behaviors_internal.cpp) | 51 | Honk helpers, rig forward |
| [goose_debug.cpp](src/common/goose_debug.cpp) | 41 | Debug logging |

The file split already matches the proposed component domains (physics ≈ `goose_forces.cpp` + `goose_behaviors_*`; rendering ≈ `goose_drawing.mm`; AI ≈ `goose_behaviors_*`).

**What's *not* done:** the class-level field decomposition into `Transform` / `Physics` / `Animation` structs. Doing that means renaming `goose->pos` → `goose->transform.pos`, `goose->vel` → `goose->physics.vel`, `goose->rig` → `goose->animation.rig` across all 8 impl files, the platform layer, every behavior, and tests — hundreds of edits for a structural change with no immediate functional payoff.

**Recommendation:** Hold §2 until there's a concrete motivating need — wanting to share components across non-Goose entities, or building a true ECS where systems iterate over component arrays. Until then, the churn doesn't earn its keep; the implementation-level decomposition is already in place.

---

## 3. Strictly Enforce the `IRenderer` Abstraction

**Was:** Behaviors received a raw `void* renderCtx` (a `CGContextRef` on macOS), cast it back, and constructed a `CGRenderer` locally — the abstraction was bypassed at the behavior boundary. Several behaviors then reached past `IRenderer` into CoreGraphics directly (image size queries, CoreText positioning) wrapped in `#ifdef __APPLE__`.

**Now:** Every behavior `render` function takes `IRenderer*` and uses it directly; no production behavior calls `nativeContext()`.

### API extensions
Added to [renderer_interface.h](include/renderer_interface.h):
- `GetImageSize(image, *w, *h)` — query loaded image dimensions
- `MeasureText(text, fontSize)` — measure text width for centering

Implemented in `CGRenderer` (CoreGraphics/CoreText) and `CairoRenderer` (`cairo_image_surface_get_*` / Pango).

### Migrations
| Behavior | Method |
|---|---|
| boredom, peeking, health, anger | Dropped CG cast; pure IRenderer was already sufficient |
| honcker | Uses `GetImageSize` for honk-bubble PNG sizing |
| hats | Uses `GetImageSize`; dropped its `CGBitmapContextCreate` pre-scaled cache (`DrawImage` destRect already handles scaling) |
| nametag | Uses `MeasureText` + `DrawText`; dropped local `CTFontRef` / `CGColorRef` cache and manual `CGContextSetTextMatrix` flip |
| pomodoro | Uses `MeasureText` + `DrawText` for the phase timer; `GetImageSize` for Zzz frame images; same font-cache cleanup |

All `#ifdef __APPLE__` guards inside behavior `render` bodies are gone.

**Remaining `nativeContext()` callers (intentional):** `Goose::render` itself dispatches to the macOS-specific `DrawGoose` rig — a legitimate platform-specific drawing path implementing the Goose sprite, not a behavior leak.

---

## 4. Overhaul Global State (`world.h`)

**Was:** `world.h` held loose `extern` globals (`g_geese`, `g_droppedItems`, monitors, footprints, crumbs, leaf piles, screen dims, id counters, ui log) — hidden dependencies and untestable systems.

**Now:** Encapsulated in [WorldContext](include/world.h), instantiated once as `g_world`, threaded into systems via dependency injection at entry points.

### What landed

- **Context struct + single instance:** `WorldContext` defined in [world.h](include/world.h); single `g_world` in [world.cpp](src/common/world.cpp); all 239 call sites updated to `g_world.X`.
- **Build restored:** the original §4 commit had referenced 38 previously-untracked Actor/behavior-manager/state files; those are now committed and per-behavior state headers are correctly included.
- **Dependency injection:** `Actor::tick` virtual takes `WorldContext&`; `ActorManager::tickAll` receives `g_world` from the renderer-side caller and forwards to every actor. `Goose::tick` stashes it on `BehaviorContext::world` so behaviors can read context without a global.
- **Event-driven side effects:** First EventBus publishers wired up.
  - `GooseHonkedEvent` — `triggerHonk` (chase/wander/idle) + `behavior_honcker` (F-key, programmatic)
  - `GooseJailedEvent` / `GooseFreedEvent` — `behavior_jail` edge-detected on the `isJailed` transition
  - `PomodoroPhaseChangedEvent` — `behavior_pomodoro` on phase rotation

### Partial-DI policy

The original plan called for refactoring "all systems, update functions, and behavior ticks" to take `WorldContext&`. That's 239 call sites; full mechanical conversion of leaf-level reads has low ROI vs. structural risk. Adopted policy: **top-level system entry points take `WorldContext&` explicitly; deep helpers continue reading `g_world` and migrate opportunistically as they get touched**. This keeps the DI value (testability of system entry points, no hidden tick-time mutation) while avoiding gratuitous churn.

### Event publishers — all 13 wired

All EventBus types defined in `event_bus.h` now have at least one production publisher:

| Event | Publisher |
|---|---|
| `GooseHonkedEvent` | `triggerHonk` (chase/wander/idle), `behavior_honcker` (F-key + programmatic) |
| `GooseDamagedEvent` | `behavior_health` when the high-speed-motion damage rule fires |
| `ItemDroppedEvent` | `goose_behaviors_fetch.cpp` `handleReturning` |
| `ItemEatenEvent` | `behavior_breadcrumbs` (when a crumb is eaten) |
| `GooseJailedEvent` / `GooseFreedEvent` | `behavior_jail` (rising/falling edge of `isJailed`) |
| `PomodoroPhaseChangedEvent` | `behavior_pomodoro` on phase rotation |
| `GooseStuckEvent` | `Goose::Update` stuck detector |
| `CursorFastMoveEvent` | `goose_behaviors_interact.cpp` avoidance check |
| `ToySpawnedEvent` | `behavior_toys.cpp` on `new ToyActor` |
| `BallKickedEvent` | `actor_ball.mm` `onGooseKick` / `onCursorKick` |
| `BreadcrumbDroppedEvent` | `behavior_breadcrumbs` on each drop |
| `GooseTeleportedEvent` | `behavior_portal.cpp` after position swap |

---

## 5. Decouple Behavior State

**Original framing:** A single `behavior_state.h` was suspected to be a god-struct/union holding state for every possible behavior at once.

**Audit:** The refactor was already complete before this work began.

- [behavior_state.h](include/behavior_state.h) is a lightweight base: `BehaviorState { virtual ~BehaviorState(); virtual void Reset(); }`. No union, no per-behavior bloat.
- Concrete states live in dedicated headers under [include/behaviors/states/](include/behaviors/states/) — one per behavior: honcker, jail, pomodoro, portal, anger, ball, breadcrumb, drag, health, peeking, rainbow, acid, toys, boredom, interactive_drops.
- [BehaviorStateManager](include/behavior_manager.h) holds `unordered_map<key, unique_ptr<BehaviorState>>`; concrete states are heap-allocated on first `GetOrCreate<T>(gooseId, behaviorId)`.
- Behaviors access their state via the templated `GetOrCreate<T>` / `Get<T>` which internally `dynamic_cast`s.

No code changes needed; this entry stays in the doc as a reference to the existing design.

---

## Review follow-ups (from [docs/ARCHITECTURE_REVIEW.md](docs/ARCHITECTURE_REVIEW.md))

### Fixed
| # | Item | What was done |
|---|---|---|
| C1 | Duplicate `draw_overlay` ODR | Deleted stale `src/platform/linux/ui_drawing.{cpp,h}`; ui.cpp's version is the canonical one registered with GTK |
| C2 | ui_drawing.cpp uses dead config fields | Covered by deletion above |
| C3 | ui_tick.cpp `g_config.mudLifetime` | → `g_config.mud.lifetime` |
| C4 | ui.cpp `g_config.multiMonitorEnabled` | → `g_config.cursor.multiMonitorEnabled` |
| C5 | ui.cpp `g_config.globalScale` | → `g_config.general.globalScale` |
| C6 | ui_callbacks.cpp flat config refs | Each rewritten to its nested home; also fixed unqualified `GooseState::` enum cases and `g.X`→`gp->X` style |
| C7 | renderer.mm deep-copying Goose every tick | `updateWindowPositionsForGeese` / `DrawDebugOverlay` now take `const std::vector<Goose*>&` |
| C8 | `ActorManager::cleanup()` leaks dead actors | Now `delete`s before erase |
| N1 | `DrawingInit()` undefined `cr` | Covered by ui_drawing.cpp removal |
| N4 | excessive `[FETCH]` stderr noise | Wrapped 16 callsites in `FETCH_LOG` gated on `g_config.debug.toTerminal` |
| N5 | `EventBus::Publish` iterating under shared_lock | Snapshot the handler list, drop the lock, then invoke — safe for re-entrant publish / (un)subscribe |
| A1 | `coordinate_system.h` implicit `operator Vector2()` defeats typed coords | Made `explicit`; added `toVector2()` named accessor; fixed 9 leaf-level callers |
| A3 | `getGeese()` allocated a vector every call | Cached behind a dirty flag invalidated by `add/remove/cleanup/destroyAllOfType` |
| A4 | CMakeLists.txt duplicate `APPLICATIONSERVICES_LIBRARY` | Removed |
| A5 | `stateNames[]` no bounds check | Both sites in goose.cpp now bounds-check before indexing |

### Reviewed — stale or "preference, not bug"
| # | Item | Resolution |
|---|---|---|
| C9 | "extern static" triggerHonk | Stale — wander's `extern triggerHonk` already links to the non-static def in `goose_behaviors_internal.cpp`; the static in `fetch.cpp` is `triggerHonkLocal` (different name) |
| C10 | Dead structs in world.h | Stale — `InteractivePuddle`, `InteractiveFlower`, `Toy` are referenced by `interactive_drops_state.h` and `toys_state.h` |
| A7 | ui_drawing.cpp / ui.cpp duplicate drawing | Resolved by C1 deletion |
| A2 | Public mutable Actor fields | Direct field access is intentional POD-style; encapsulating would be ceremony without payoff (same justification as §2) |
| A6 | world.h is a God header | Same content as §2 audit; deferred |
| A8 | 79 `rand() % N` calls with modulo bias | Game RNG; the bias is irrelevant at game-scale Ns. Skipped |
| N2 | `config.h` 517 lines | Cosmetic; large but readable single source of truth |
| N3 | `item_window.mm` 4 responsibilities | Cosmetic; tightly coupled by intent (one window's lifecycle) |
