# CadGoose Architecture: Opportunities for Improvement

Based on a deep scan of the CadGoose codebase, the project is currently in a transitional state—moving from a monolithic, global-state-heavy architecture towards a more modular, Actor-based system. While solid foundations exist (like `EventBus`, `IRenderer`, and `ActorManager`), there are several high-impact opportunities to improve the structure and build robust systems.

## 1. Complete the Actor/ECS Migration
The codebase currently suffers from "dual-tracking" and fragmented logic. For example, `world.h` still maintains legacy global lists like `g_geese` and `g_droppedItems`, while an `ActorManager` (`include/actor.h`) is being introduced. 

**Opportunity:** Fully migrate all entities (Geese, Items, Particles) into a unified Entity-Component-System (ECS) or complete the Actor model. This would eliminate the parallel tracking systems and centralize lifecycle management.

### Detailed Execution Plan:
*   **Phase 1 [DONE]:** Audit complete — `ActorManager` supports typed iteration via `getGeese()`, generic add/remove, and a new `destroyAllOfType(type)` for owning cleanup. Each game entity has an Actor subclass (BallActor, ToyActor, FlowerActor, JailActor, BreadcrumbActor, LeafPileActor, PortalActor).
*   **Phase 2 [DONE]:** Goose instantiation goes through `ActorManager::Instance().add(new Goose(...))` — call sites in app_actions.cpp, ui_callbacks.cpp, and tests updated. Other actor types already used this pattern.
*   **Phase 3 [DONE]:** All systems / call sites that iterated `g_world.geese` now query `ActorManager::Instance().getGeese()` — app_actions, config_save, config_gui_detail, main.mm, ui_callbacks, ui_escape, tests.
*   **Phase 4 [DONE]:** `std::list<Goose> geese` removed from `WorldContext`. `destroyAllOfType("goose")` deletes the heap-allocated Goose instances on clear. 722 tests pass.

Remaining dual-tracking in WorldContext (smaller scope, lower urgency): `droppedItems`, `footprints`, `crumbs`, `leafPiles` are still WorldContext-owned. LeafPile already has an Actor mirror (`actor_leafpile.mm`); the others are simpler value collections and may not need Actor wrapping.

---

## 2. Deconstruct the `Goose` Monolith
The `Goose` class (`src/common/goose.cpp`) is currently acting as a "God object" that handles physics, animation, state logic, and rendering updates all at once.

**Opportunity:** Break `Goose` down into distinct components (e.g., `PhysicsComponent`, `AnimationComponent`, `AIStateComponent`). If you move towards an ECS, the "Goose" simply becomes an entity ID with these components attached to it, drastically improving testability and maintainability.

### Detailed Execution Plan:
*   **Phase 1: Component Definition:** Define clean structs/classes for isolated domains: `Transform` (position, rotation, scale), `Physics` (velocity, acceleration, mass), and `Animation` (current frame, sprite sheet, state).
*   **Phase 2: Extract Physics & Movement:** Move the physics tick logic out of `Goose::tick` and into a dedicated `PhysicsSystem` or external update function that operates on the `Physics` and `Transform` components.
*   **Phase 3: Extract Rendering:** Move the drawing logic out of `Goose::render` into a dedicated `RenderSystem` that uses the `Animation` and `Transform` components to interface with the `IRenderer`.
*   **Phase 4: Refactor Goose:** Reduce the `Goose` class to a simple container of these components, or fully convert it into an Entity ID within the new Actor/ECS system.

---

## 3. Strictly Enforce the `IRenderer` Abstraction
While an `IRenderer` interface (`include/renderer_interface.h`) exists for cross-platform rendering, the scan revealed that it's being bypassed in certain behaviors (like `behavior_rainbow.cpp`), where code casts down to native platform contexts (e.g., macOS-specific contexts in `goose_drawing.mm`).

**Opportunity:** Expand the `IRenderer` API to natively support the advanced drawing features needed by behaviors (like custom shaders, gradients, or primitive drawing) so that platform-specific code is completely isolated behind the interface. 

### Detailed Execution Plan:
*   **Phase 1: Audit Abstraction Leaks:** Search the codebase for any casting of `IRenderer` to platform-specific types, or direct inclusions of platform rendering APIs (like CoreGraphics or GDI+) within common behavior code.
*   **Phase 2: Extend API:** Identify the specific visual effects being achieved by these leaks (e.g., gradient drawing, custom blending). Add virtual methods to `IRenderer` to support these natively (e.g., `DrawGradientRect`, `SetBlendMode`).
*   **Phase 3: Platform Implementation:** Implement the newly added `IRenderer` methods in all platform-specific backend classes (macOS, Windows, Linux).
*   **Phase 4: Behavior Refactoring:** Update the behaviors to use the new `IRenderer` methods, removing all platform-specific casts and includes from the common code.

---

## 4. Overhaul Global State (`world.h`)
The `world.h` file acts as a dumping ground for global variables and tight coupling.

**Opportunity:** Encapsulate the global state into a `WorldContext` or `EngineState` object that is passed down to systems via dependency injection, or managed strictly by the `EventBus`. This eliminates hidden dependencies and makes unit testing significantly easier.

### Detailed Execution Plan:
*   **Phase 1 [DONE]:** `WorldContext` struct defined in [world.h](include/world.h); legacy globals (`g_geese`, `g_droppedItems`, monitors, footprints, crumbs, leaf piles, screen dims, id counters, ui log) moved inside as members.
*   **Phase 2 [DONE]:** Single `g_world` instance instantiated in [world.cpp](src/common/world.cpp); all 239 call sites updated to `g_world.X`.
*   **Phase 2.5 [DONE]:** Restored build — committed previously-untracked Actor/behavior_manager/state files and added missing per-behavior state-header includes ([fix commit](#)). 723 tests pass.
*   **Phase 3 [DONE — partial]:** Dependency Injection — `Actor::tick` virtual now takes `WorldContext&`; `ActorManager::tickAll` forwards it; `Goose::tick` stashes it on `BehaviorContext::world` for behaviors. Leaf-level `g_world.X` reads in deep helpers are deferred — they migrate opportunistically as touched.
*   **Phase 4 [DONE — partial]:** Event-Driven Refactoring — first publishers wired up: `GooseHonkedEvent` (triggerHonk + behavior_honcker), `GooseJailedEvent` / `GooseFreedEvent` (behavior_jail edge detect), `PomodoroPhaseChangedEvent` (behavior_pomodoro). Remaining events left for opportunistic follow-up: `ItemDropped`, `ItemEaten`, `ToySpawned`, `BallKicked`, `BreadcrumbDropped`, `GooseStuck`, `CursorFastMove`, `GooseTeleported`.

---

## 5. Decouple Behavior State
The behavior system (`include/behavior_state.h`) contains tightly coupled state structs for every possible behavior loaded into memory.

**Opportunity:** Implement a more flexible, data-driven Behavior Tree or State Machine system where behaviors allocate their specific state dynamically or manage it within their own isolated memory pools, rather than bloating a central header file.

### Detailed Execution Plan:
*   **Phase 1 [DONE]:** [behavior_state.h](include/behavior_state.h) is now a lightweight base — `BehaviorState` is just `{ virtual ~BehaviorState(); virtual void Reset(); }`. No union/bloat.
*   **Phase 2 [DONE]:** Concrete states live in their own headers under [include/behaviors/states/](include/behaviors/states/) (one per behavior: honcker, jail, pomodoro, portal, anger, ball, breadcrumb, drag, health, peeking, rainbow, acid, toys, boredom, interactive_drops).
*   **Phase 3 [DONE]:** [BehaviorStateManager](include/behavior_manager.h) holds `unordered_map<key, unique_ptr<BehaviorState>>`; concrete states are allocated on first `GetOrCreate<T>(gooseId, behaviorId)`.
*   **Phase 4 [DONE]:** Behaviors access their state via the templated `GetOrCreate<T>` / `Get<T>` which internally `dynamic_cast`s — see e.g. behavior_honcker.cpp, behavior_jail.cpp.

Section 5 was effectively complete before this audit pass; no code changes needed.