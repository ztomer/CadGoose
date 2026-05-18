# CadGoose Architecture: Opportunities for Improvement

Based on a deep scan of the CadGoose codebase, the project is currently in a transitional state—moving from a monolithic, global-state-heavy architecture towards a more modular, Actor-based system. While solid foundations exist (like `EventBus`, `IRenderer`, and `ActorManager`), there are several high-impact opportunities to improve the structure and build robust systems.

## 1. Complete the Actor/ECS Migration
The codebase currently suffers from "dual-tracking" and fragmented logic. For example, `world.h` still maintains legacy global lists like `g_geese` and `g_droppedItems`, while an `ActorManager` (`include/actor.h`) is being introduced. 

**Opportunity:** Fully migrate all entities (Geese, Items, Particles) into a unified Entity-Component-System (ECS) or complete the Actor model. This would eliminate the parallel tracking systems and centralize lifecycle management.

### Detailed Execution Plan:
*   **Phase 1: Feature Parity & Audit:** Audit the `ActorManager` to ensure it can support all current operations performed on `g_geese` and `g_droppedItems` (e.g., spatial querying, typed iteration, depth sorting).
*   **Phase 2: Entity Migration:** Create specific `Actor` subclasses or Entity configurations for Geese, Items, and Particles if they don't already exist. Ensure their instantiation goes through the `ActorManager`.
*   **Phase 3: System Updates:** Update all systems (physics, rendering, AI behaviors) that currently iterate over the global lists in `world.h` to instead query the `ActorManager` for relevant entities.
*   **Phase 4: Cleanup:** Remove `g_geese`, `g_droppedItems`, and related legacy tracking lists from `world.h`. Verify no memory leaks exist during entity destruction.

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
*   **Phase 1: Context Struct Definition:** Create a new `WorldContext` (or `EngineState`) struct/class and move all global variables from `world.h` into it as members.
*   **Phase 2: Context Instantiation:** Update the main application entry point to instantiate a single instance of `WorldContext`.
*   **Phase 3: Dependency Injection:** Refactor all systems, update functions, and behavior ticks that previously accessed global state to instead accept a `WorldContext&` as a parameter.
*   **Phase 4: Event-Driven Refactoring:** For state changes that trigger side effects, migrate from direct variable polling to emitting and listening for events via the `EventBus`.

---

## 5. Decouple Behavior State
The behavior system (`include/behavior_state.h`) contains tightly coupled state structs for every possible behavior loaded into memory.

**Opportunity:** Implement a more flexible, data-driven Behavior Tree or State Machine system where behaviors allocate their specific state dynamically or manage it within their own isolated memory pools, rather than bloating a central header file.

### Detailed Execution Plan:
*   **Phase 1: Base State Interface:** Refactor `BehaviorState` (which is likely a massive union or heavily bloated struct) into a lightweight base class or a type-erased wrapper (like `std::any` or a variant).
*   **Phase 2: Concrete States:** Move the specific state variables for each behavior into their own isolated classes (e.g., `RainbowBehaviorState`, `ItemDragState`) defined near the behavior implementation.
*   **Phase 3: Dynamic Allocation/Storage:** Update the `BehaviorContext` to hold a pointer (or value-semantic wrapper) to the base state, allocating specific states dynamically when a behavior is activated.
*   **Phase 4: Behavior Updates:** Refactor behavior implementation files to safely downcast or access their specific state type during their tick/update cycles.