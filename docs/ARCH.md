# Internal Architecture — CadGoose

## Project Structure

```
src/
  platform/
    macos/                # macOS-specific (AppKit, CoreGraphics)
    linux/                # Linux-specific (GTK4, Wayland/X11)
      protocols/          # Wayland protocol definitions
  common/                 # Shared game logic
    behaviors/            # Behavior implementations (19 behaviors)
  include/                # Headers
    linux/                # Linux-specific headers (CairoRenderer, etc.)
Assets/
  Images/Memes/          # PNG images geese can pick up
  Sound/NotEmbedded/     # Sound effects
  Text/NotepadMessages/  # Text files for notes
docs/
  ARCH.md                # Architecture overview (this file)
  CHANGELOG.md           # Completed work log
  PLAN.md                # Forward-looking work
  MCP.md                 # MCP protocol and AI integration
  PROTOCOL.md            # Unix socket command protocol
  HOMEBREW.md            # Homebrew Cask install docs
  README_LINUX.md        # Linux build instructions
tests/                   # Test suite (1520+ tests)
tools/
  generate_config.py     # Config code generator (schema → registry)
  config_schema.yaml     # Single source of truth for config options
vendor/                  # Third-party libraries (toml11 via FetchContent)
CMakeLists.txt
```

## Core Architecture

### Actor System

All world entities use the **Actor pattern** — each entity is an independent object with its own lifecycle (tick/render/cleanup), managed by `ActorManager` singleton.

**Actor base class** (`include/actor.h`):
```cpp
class Actor {
    virtual const char* type() const = 0;
    virtual ActorType actorType() const = 0;
    virtual void tick(double dt, double time) = 0;
    virtual void render(IRenderer* renderer) = 0;
    virtual bool isAlive() const = 0;
};
```

**10 Actor types**:
| Actor | File | Description |
|-------|------|-------------|
| `Goose` | `goose.h/cpp` | Main character, extends Actor |
| `BabyStalinActor` | `baby_stalin_actor.h/mm` | Stalin-mode goose variant, plays Gulag |
| `BallActor` | `actor_ball.h/mm` | Pushable ball with physics |
| `ToyActor` | `actor_toy.h/mm` | Stick/ball toys on ground |
| `FlowerActor` | `actor_flower.h/mm` | Growing flowers |
| `JailActor` | `actor_jail.h/mm` | Cage trap |
| `PortalActor` | `actor_portal.h/mm` | Portal A/B teleportation |
| `BreadcrumbActor` | `actor_breadcrumb.h/mm` | Fade + expiry crumbs |
| `LeafPileActor` | `actor_leafpile.h/mm` | 128-leaf particle piles |
| `DroppedItemActor` | `actor_dropped_item.h/mm` | Memes/text/toys on ground |

**ActorManager** (`include/actor_manager.h`):
- `add(Actor*)` / `remove(Actor*)` — lifecycle management
- `tickAll(dt, time)` — ticks all actors
- `renderAll(renderer)` — renders all actors
- `getGeese()` — returns `std::vector<Goose*>` for goose-specific queries
- `getByIndex(i)` / `totalCount()` — iteration support
- `findByType(ActorType)` / `findByType(ActorType, int id)` — typed queries

**Goose lifecycle**: `Goose` extends `Actor`. `TickManager` calls `ActorManager::tickAll()` and `ActorManager::renderAll()` for unified lifecycle. `g_geese` list retained for spawn/clear/config persistence only.

### Behavior System

Behaviors are **logic attached to Goose actors** — they modify goose state/appearance but don't manage entity state directly.

**19 behaviors** use `BEHAVIOR_DEF*` macros (`include/behavior.h`) that enforce `enabledPtr == configPtr` pointing to the same config bool, preventing toggle-desync bugs.

| Macro | Use Case |
|-------|----------|
| `BEHAVIOR_DEF` | Standard (init, tick, render) |
| `BEHAVIOR_DEF_STARTER` | Enabled by default |
| `BEHAVIOR_DEF_GROUND` | Ground-pass rendering |
| `BEHAVIOR_DEF_CUSTOM` | Custom cleanup/priority/renderOnGround |

**Two-pass rendering**: `RenderPass()` — ground pass before goose, overlay after.

**BehaviorStateManager**: Per-goose keyed state storage (`GetOrCreate<T>(gooseId, "behaviorId")`). `RemoveForGoose(id)` called in `~Goose()` to prevent leaks.

### State Machine

Each goose runs an independent state machine (`GooseState`):

| State | Description |
|-------|-------------|
| `WANDER` | Default — walks randomly, picks new targets |
| `FETCHING` | Walking toward item to pick up |
| `RETURNING` | Carrying item to drop location |
| `CHASE_CURSOR` | Pursuing cursor position |
| `SNATCH_CURSOR` | Controlling pointer movement |

Transitions evaluated every tick. External events can interrupt `WANDER`. `FETCHING` → `RETURNING` → `WANDER` cycle. `CHASE_CURSOR` timeout falls back to `WANDER`.

### EffectWindow System

Lightweight floating windows for environmental effects that persist independently of goose lifecycle.

**2 effect types** (all others migrated to Actors):
- `EffectTypeFootprint` — muddy footprints with fade + auto-cleanup
- `EffectTypePomodoroBed` — bed image for pomodoro rest phase

**Effect registration** (`include/effect_registration.h`): Each effect type self-registers via `EffectRegister()` with callbacks for position, radius, existence, and window configuration. `EffectWindowManager::syncWindows` iterates registrations generically.

### EventBus

Type-safe event bus (`include/event_bus.h`) for decoupled behavior signaling.

**13 event types**: `GooseHonked`, `GooseDamaged`, `ItemDropped`, `ItemEaten`, `GooseJailed`, `GooseFreed`, `PomodoroPhaseChanged`, `GooseStuck`, `CursorFastMove`, `ToySpawned`, `BallKicked`, `BreadcrumbDropped`, `GooseTeleported`.

Thread-safe with `shared_mutex`, unique subscription IDs, unsubscribe support. 22 unit tests.

### Rendering

**IRenderer interface** (`include/renderer_interface.h`): Platform-agnostic rendering with `DrawEllipse`, `DrawEllipseOutline`, `DrawLine`, `DrawRect`, `DrawRectOutline`, `DrawRoundedRect`, `DrawPolygon`, `DrawImage`, `DrawText`, `SaveState/RestoreState`, `Translate/Scale/Rotate`, `SetAlpha`, `nativeContext()`.

**Implementations**:
- `CGRenderer` — macOS CoreGraphics wrapper (`include/cg_renderer.h`). Includes `CGColorCache` (thread-local) and cached font/path helpers.
- `CairoRenderer` — Linux Cairo + Pango (header-only, `include/linux/cairo_renderer.h`, `#ifndef __APPLE__`)

**Behavior rendering scaling**: `TickManager` applies `CGContextScaleCTM` transform around `goose->pos` before calling `RenderPass`. All behaviors use raw pixel values (no manual scaling).

**ItemRenderer strategy** (`include/item_renderer.h`): Base class + `MemeItemRenderer`, `TextItemRenderer`, `ToyItemRenderer`. Eliminates type-specific branching. `DrawHeld()` for goose-held items, `DrawDropped()` for ground items.

### Coordinate System

Typed coordinate wrappers (`include/coordinate_system.h`): `DevicePoint`, `WorldPoint`, `ScreenPoint`, `ViewPoint`. Prevents coordinate space mixing bugs. All goose pos/target/vel/item.pos are DEVICE coords. Rig parts are WORLD coords. Explicit transforms via `CoordTransform`.

### Config System

**Schema-driven code generation** (`tools/config_schema.yaml` + `tools/generate_config.py`): Single source of truth generates `config_registry_generated.cpp` with all config option registrations.

**15 config sections**: Debug, General, Screen, Asset, Cursor, Movement, Physics, Spawn, Rig, Snatch, Mud, Honk, Step, Item, Render. 175+ fields with proper min/max/step ranges.

**Config key consistency**: GUI `addRow:` calls, config registry, and behavior `configPtr` all use the same registry keys. `lookupKey` field differentiates cross-section collisions (e.g., `snap_distance` lives in both Physics and Step).

**Config self-healing**: macOS `Config_LoadAll` forces `audio_enabled = true` to repair configs poisoned by the old default.

### AI System

**Text generation backends**: Priority: (1) local CoreML LLM → (2) HTTP provider → (3) file texts.

**Local CoreML LLM** (`local_llm*.mm`): Direct `MLModel` integration, discovers Apple Intelligence FM GenerativeModels assets. Tokenizer from model package. Autoregressive top-K (K=40), 48 token limit, 512 context. Thread-safe via mutex-guarded model access.

**AI Text Memes**: Two-pool system (AI pool + file pool). `Dequeue()` prefers AI, falls back to file. Async generator builds prompts from evil level + active behaviors + color mode + random seed. Temperature 0-2 (default 1.2).

**AI chat** (`ai_http_client.mm`): HTTP-only LLM client with function calling (MCP tools as OpenAI tools), `<think>...</think>` block stripping, 4-stage fallback chain (LLM→tools→MCP bridge→keywords). Foundation provider caps evil at 72%.

**MCP server**: 12 tools, 5 resource URIs, dual transport (Unix socket + HTTP), JSON-RPC 2.0. Background-thread config access synchronized via `OnMainThread()` dispatch.

### Platform Abstraction

Core game logic, physics (steering behaviors, kinematics), and state machines live in `src/common/`.

Platform-specific backends in `src/platform/macos/` or `src/platform/linux/`:
- **Renderer**: CoreGraphics with `CGRenderer` (macOS) / Cairo + Pango (Linux)
- **CursorBackend**: Abstract class for mouse position read/write (Quartz / Wayland / X11). `g_cursorProvider` used by behaviors (breadcrumbs, ball).
- **Window management**: AppKit `NSWindow` (macOS) / GTK4 (Linux) — per-actor windows, no full-screen overlay.
- **TickManager**: `CADisplayLink`-driven 60fps tick+render loop (macOS). Owns the per-goose window update cycle.

## Key Design Principles

1. **Actors ≠ Behaviors**: Actors are entities (position, rendering, lifecycle). Behaviors are logic attached to Goose actors.
2. **No files >500 LOC**: Enforced by splitting large files.
3. **No magic numbers**: Extracted to named constants.
4. **DRY**: Eliminated through code generation, base classes, and shared utilities.
5. **Type safety**: Coordinate wrappers prevent space mixing bugs. EventBus prevents tight coupling. `ActorType` enum replaces string-based type checks. `FetchType` enum replaces int-based fetch commands.
6. **Fix the class, not the instance**: Every bug is one instance of a general failure mode — name the invariant, grep for siblings, generalize the system, add a regression test.

## Memory Profile

No full-screen overlay. Per-actor windows (BehaviorElementWindow, ItemWindow, EffectWindow) use negligible memory relative to the old single ~1GB IOSurface-backed overlay. RSS stable 150-250 MB idle. No Metal textures — all CoreGraphics.

## Test Results

**1520+ tests, 0 failures** (CI: ~62 skipped: WindowTrailTest needs display, AccessibilityGUI tests headless-incompatible, LocalLLM tests need model).
