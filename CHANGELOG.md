# Changelog

## May 18, 2026 — Bug Fixes + Architecture Audit Complete

### Bugs Fixed (3)
- **Hash collision in BehaviorStateManager** — Replaced 32-bit truncated hash with 48-bit FNV-1a + 16-bit gooseId. Zero collision risk with ≤65535 geese and ≤15 behaviors.
- **Recursive tool call stack overflow** — Converted `ai_http_client.mm` from recursive to iterative chat loop (`completeChatLoopWithCompletion` + `onDone` callback). Call stack stays flat.
- **Linux monitor-to-window matching** — `ui_tick.cpp` now uses monitor index stored in canvas widget data. Eliminates fragile iteration that always used first monitor.

### Items Verified (Already Done)
- **DragTest** — Already in separate file (`item_window_test.mm`), not mixed with production code.
- **Config generator** — Runs correctly, generates valid `config_registry_generated.cpp`. GUI generation intentionally skipped (incompatible architecture).
- **Stale pointer risk** — Mitigated by `IsItemValid()` check before every use + `std::list` pointer stability.
- **Goose monolith** — `Update()` split into `UpdatePhysics/Detection/Animation/Debug()`.
- **IRenderer** — All 14 behaviors migrated, no abstraction leaks.
- **WorldContext** — Exists in `world.h` with all global state.
- **Behavior state** — 15 state structs in individual files under `include/behaviors/states/`.
- **AI code** — Split into 5 files (all under 323 LOC).
- **Linux UI** — Split into 5 files (all under 473 LOC).
- **BEHAVIOR_DEF* macros** — `BEHAVIOR_DEF_FULL` is single source, 4 variants are thin wrappers.

### Code Quality
- **25 state includes fixed** — Added missing `#include "behaviors/states/*.h"` to 12 behavior source files + 13 test files.
- **item_window.mm refactored** — `IsItemValid()` helper (7 repetitions eliminated), `GetMouseDeviceCoords()` helper (4 repetitions eliminated). ~110 LOC reduction (17%).
- **DroppedItemActor scaffold** — Created (`include/actor_dropped_item.h`, `src/common/actor_dropped_item.mm`). Ready for future migration of `g_world.droppedItems` (127 refs deferred).

### Build & Tests
- **755 tests, 99 suites** — 723 pass, 1 pre-existing failure, 32 skipped (no regressions)

## May 17, 2026 — Phase 2 Cleanup

### Legacy Stub Removal
- Deleted `Toys_GetInfo`, `Toys_GetActiveCount` from `behavior_toys.cpp`/`world.h`
- Deleted `Flowers_GetAll` from `behavior_interactive_drops.cpp`/`world.h`
- Deleted `Jail_GetData`, `Jail_IsActive` from `behavior_jail.cpp`/`world.h`
- Deleted `Portal_GetImages` from `behavior_portal.cpp`/`world.h`
- Removed `ToyInfo`, `FlowerInfo` structs from `world.h`

### Dead File Cleanup
- Deleted obsolete registration files: `effect_reg_toy.mm`, `effect_reg_flower.mm`, `effect_reg_jail.mm`, `effect_reg_portal.mm`, `effect_reg_breadcrumb.mm`, `effect_reg_leafpile.mm`

### EffectWindow Cleanup
- Removed dead drawing code from `effect_window.mm` (LeafPile, Breadcrumb, Portal, Jail, Toy, Flower)
- Removed unused imports (`goose_drawing.h`, `render_colors.h`, `behavior.h`, `<ctime>`)
- Removed unused `extern` declarations (`g_leafPiles`, `g_crumbs`)
- Simplified time-based redraw check to only `EffectTypeFootprint`
- Cleaned `effect_window.h`: removed unused enum values (LeafPile, Breadcrumb, Portal, Jail, Toy, Flower), removed unused properties (`jailActive`, `toyIndex`, `currentTime` on EffectWindow)

### g_geese Migration
- Migrated all `g_geese` iteration code to `ActorManager::getGeese()`:
  - `renderer.mm`, `effect_reg_pomodorobed.mm`, `goose.cpp`, `world.cpp`
  - `goose_behaviors_wander.cpp`, `goose_forces.cpp`, `goose_debug.cpp`, `behavior_ai.mm`
  - Linux: `ui.cpp`, `ui_drawing.cpp`, `ui_callbacks.cpp`, `ui_debug.cpp`, `ui_escape.cpp`
- `g_geese` retained for lifecycle management only (spawn, clear, config save/load)

### Build & Tests
- **755 tests, 99 suites** — 723 pass, 1 pre-existing failure, 31 AX tests skipped

## May 17, 2026

### Actor System Migration (R1) — COMPLETE
Migrated all world entities to Actor pattern with unified lifecycle management:

#### Stationary Element Actors
- **BallActor** — Ball physics, window, animation (`include/actor_ball.h`, `src/common/actor_ball.mm`)
- **ToyActor** — Stick/ball on ground with own window (`include/actor_toy.h`, `src/common/actor_toy.mm`)
- **FlowerActor** — Growth animation with own window (`include/actor_flower.h`, `src/common/actor_flower.mm`)
- **JailActor** — Cage with own window (`include/actor_jail.h`, `src/common/actor_jail.mm`)
- **PortalActor** — Portal A/B with image rendering (`include/actor_portal.h`, `src/common/actor_portal.mm`)
- **BreadcrumbActor** — Fade + expiry with own window (`include/actor_breadcrumb.h`, `src/common/actor_breadcrumb.mm`)
- **LeafPileActor** — 128 leaf particles with physics (`include/actor_leafpile.h`, `src/common/actor_leafpile.mm`)

#### Goose as Actor
- `Goose` extends `Actor` base class
- `Goose::tick()` calls `Update()` + behavior tick + cursor action
- `Goose::render()` calls `DrawGoose()` + held item + behavior render passes via `IRenderer`
- `ActorManager::getGeese()` returns all Goose actors
- `renderer.mm` uses `ActorManager::tickAll()` and `ActorManager::renderAll()` for unified lifecycle
- `AppActions_SpawnGoose` adds Goose to ActorManager
- `g_geese` kept for backward compatibility

#### Behavior Migration
- All behaviors now delegate to actors instead of managing state directly
- Effect registration files removed from build (toys, flowers, jails, portals, breadcrumbs, leafpiles)
- Stub functions added for legacy EffectWindow references (to be cleaned up)

### EffectWindow Registration Pattern (R2) — COMPLETE
- Each effect type self-registers via `EffectRegister()` with callbacks for position, radius, existence, and window configuration
- `EffectWindowManager::syncWindows` is now generic — iterates registrations instead of monolithic switch statements
- Created `include/effect_registration.h` and `src/platform/macos/effect_registration.mm`
- Each effect type self-registers via static initializer in `src/platform/macos/effect_reg_*.mm`

### Test Results
- **755 tests, 99 suites** — 723 pass, 1 pre-existing failure (`LocalLLMTest.GenerateWithHighTemperatureDoesNotCrash`), 31 AX tests skipped

## May 17, 2026

### Affirmations Removal
- Removed `behavior_affirmations.cpp` and all references (config, registry, GUI, tests, CMakeLists)
- Behavior count reduced from 20 to 19

### Window Migration — All Stationary Behaviors
Completed migration of all stationary behavior elements to independent windows:
- **Jails → EffectWindow** (`EffectTypeJail`): Jail cages now render as independent click-through windows with rect + label. Render function in `behavior_jail.cpp` is now a stub.
- **Pomodoro bed → EffectWindow** (`EffectTypePomodoroBed`): Bed image renders independently at bottom-right corner. Timer text and ZZZ remain in goose window (goose-relative).
- **Toys → EffectWindow** (`EffectTypeToy`): Each toy (stick or ball) gets its own window with proper rotation for sticks. Render function in `behavior_toys.cpp` is now a stub.
- **Flowers → EffectWindow** (`EffectTypeFlower`): Each dropped flower renders independently with stem, petals, and growth animation. Render function in `behavior_interactive_drops.cpp` is now a stub.

### Post-Sweep Fixes
- **Ball cursor hit-test**: Ball window sized to `ball->radius * 2`, cursor kick uses circle hit-test (`distToCursor < ball->radius`) instead of footSize-based margin. Fixes interaction only at ball center.
- **Autumn leaves interaction**: Leaf pile kick proximity uses `g_config.render.footSize` instead of hardcoded `4.0f`, matching goose's actual size.
- **NSWindow crash fix**: `releasedWhenClosed = NO` set on all dictionary-managed windows (`item_window.mm`, `effect_window.mm`) to prevent double-release.
- **Window migration audit**: Full audit of 20 behaviors — 4 candidates for independent windows identified (jails, pomodoro bed, toys, flowers). 8 already independent, 8 correctly in goose window.
- **Pomodoro timer centering**: Timer text now uses precise centering like nametag (`CTLineGetOffsetForStringIndex`).

### UI Polish
- **N1**: Appearance label vertically centered with mode selector.
- **N2**: Color rows left-aligned (Body, Neck, Head, Beak, Eyes, Outline share same left X).
- **N3**: Test Connection button moved to same row as refresh icon, status below.
- **N4**: Increased spacing before Advanced section header (10px → 26px).
- **N5**: Behavior tab headers positioned for better group separation (label moved to bottom of header row).

### Footprint Windows
- Migrated muddy footprints from goose renderer to independent `EffectWindow` windows (`EffectTypeFootprint`)
- Each footprint gets its own transparent click-through window that fades over lifetime and auto-cleans on expiry
- Footprints now persist on screen independently of goose movement/respawn
- Removed `DrawFootprints()` from `renderer.mm`
- Files: `src/platform/macos/effect_window.mm`, `src/platform/macos/renderer.mm`

### Ball Window Sizing
- Ball `BehaviorElementWindow` expanded to `ball diameter + 2 * maxKickDistance` to encompass full cursor interaction area
- Ball image centered within larger window
- Fixes mouse interactions only working when ball at rest
- File: `src/common/behaviors/behavior_ball.mm`

### Ball Animation Frames
- Added `setNeedsDisplay:YES` on content view after position update to trigger frame redraw
- File: `src/common/behaviors/behavior_ball.mm`

## May 16, 2026

### ItemRenderer Strategy Pattern
- Created `ItemRenderer` base class with `MemeItemRenderer`, `TextItemRenderer`, `ToyItemRenderer`
- Replaced type-specific branching in `goose_drawing.mm` with strategy pattern
- `DrawHeld()` for goose-held items, `DrawDropped()` for ground items
- `DrawDropped()` returns bool for close button visibility (toys return `false`)
- Factory via `ItemRenderer::ForType(ItemData::Type)`
- Files: `include/item_renderer.h`, `src/common/item_renderer.mm`, `src/common/goose_drawing.mm`

### EventBus for Decoupled Behavior Signaling
- Type-safe event bus with 13 event types: `GooseHonked`, `GooseDamaged`, `ItemDropped`, `ItemEaten`, `GooseJailed`, `GooseFreed`, `PomodoroPhaseChanged`, `GooseStuck`, `CursorFastMove`, `ToySpawned`, `BallKicked`, `BreadcrumbDropped`, `GooseTeleported`
- Thread-safe with `shared_mutex`, unique subscription IDs, unsubscribe support
- 22 unit tests
- Integrated into `behavior_anger.cpp` — anger subscribes to `GooseHonkedEvent` and `CursorFastMoveEvent`
- Files: `include/event_bus.h`, `src/common/event_bus.cpp`, `tests/common/test_event_bus.cpp`

### IRenderer Full Migration (14 behaviors)
- All 14 behaviors now use `IRenderer` interface instead of direct CoreGraphics calls
- Added `DrawPolygon()` to `IRenderer`/`CGRenderer` for toy stick rendering
- Migrated: `anger`, `breadcrumbs`, `toys`, `interactive_drops`, `health`, `ball`, `jail`, `portal`, `boredom`, `peeking`, `honcker`, `nametag`, `pomodoro`, `hats`
- Remaining CGContext references are for `CGRenderer` construction or asset preparation only

### CairoRenderer for Linux
- Created `CairoRenderer` in `include/linux/cairo_renderer.h`
- Full parity with `CGRenderer`: all primitives, transforms, state management, alpha tracking, text via Pango
- Header-only, guarded with `#ifndef __APPLE__`

### Config Schema Expansion
- Expanded `tools/config_schema.yaml` from 7 to 15 sections (added Spawn, Rig, Snatch, Mud, Honk, Step, Item, Render)
- 80+ new fields covered with proper min/max/step ranges

### Test Results
- **731 tests, 100 suites** — All pass (30 AX tests skipped)
- **Soak test** — 10 min simulated, 146,994 px cumulative, 92 state changes, 0 stuck, 0 zero-vel frames

### Issue Analysis (May 16, 2026)
- Ran 60s debug session, analyzed 225-line log
- Identified 4 bugs: drag broken, names not persisted, config compile errors, AI queue empty
- Documented in `docs/ISSUES.md` with root causes, fixes, and test plans

### Bug Fixes (May 16, 2026)

#### 1. Drag Memes Fixed
- Removed double Y-inversion in event monitor coordinates (`renderer.mm:119-121`, `renderer.mm:131-133`)
- Changed `viewY = view.bounds.size.height - viewPt.y` to `viewY:viewPt.y`
- For isFlipped=YES views, `viewPt.y` is already in correct top-left origin, Y-down space
- Files: `src/platform/macos/renderer.mm`, `include/coordinate_system.h`

#### 2. Names Persistence Fixed
- Added `Config_SaveGooseNames()` helper function in `config_save.cpp`
- Replaced inline name-save code in `main.mm:quitApp:`, `main.mm:applicationWillTerminate:`, `app_actions.cpp:AppActions_ClearGeese()`
- Names now saved whenever geese are cleared or app terminates
- Files: `include/config.h`, `src/common/config_save.cpp`, `src/platform/macos/main.mm`, `src/common/app_actions.cpp`

#### 3. Config Compile Errors Fixed
- Deleted `src/platform/macos/config_gui_generated.mm` (broken generated code)
- File was dead code — behaviors tab uses hand-written registry-lookup pattern in `config_gui.mm`
- Removed from build via GLOB exclusion (file no longer exists)

#### 4. AI Text Queue Logging Added
- Added detailed error logging to all 7 failure paths in `ai_text_meme.mm`
- Network errors now log endpoint URL
- HTTP errors now log status code + response body
- Parse errors now log raw response body
- Empty content errors now log full JSON
- Invalid URL and JSON serialization errors now logged
- Tick function now logs provider type
- **Root cause discovered**: Osaurus server (localhost:1337) responds to curl but NSURLSession requests timeout after 60s — server-side issue, not code bug

### Bug Fixes Round 2 (May 16, 2026)

#### 1. AI Timeout Fixed
- Increased `request.timeoutInterval` from 60s to 600s in `ai_text_meme.mm:156`
- Matches ztools' Python requests timeout (600s) for cold-start tolerance
- File: `src/common/behaviors/ai_text_meme.mm`

#### 2. Drag Hit Test Fixed
- `item.pos` is top-left corner but `HitTest::PointInItem` expected center
- Fixed by computing center via `ItemCoords::Center()` before hit test
- File: `src/platform/macos/item_drag_controller.mm:7-9`

#### 3. Responder Chain Enabled
- `ignoresMouseEvents` was `YES` when no drag was active, blocking all mouse events
- Now stays `NO` when dropped items exist, enabling native responder chain
- File: `src/platform/macos/renderer.mm:267-271`

#### 4. Drag Tests Fixed
- Test click position `{460, 370}` was hitting close button (top-left corner)
- Changed to `{540, 430}` (bottom-right area) to avoid close button
- Added `g_config.general.globalScale = 1.0f` initialization to all drag tests
- File: `tests/platform/macos/test_headless_rendering.mm`

### Bug Fixes Round 3 (May 16, 2026)

#### 1. AI Chat Connection Check Fixed
- Foundation provider check now polls for Ready state (max 10 retries, 1s interval)
- Previously showed "disabled" immediately because async loading wasn't complete
- File: `src/common/behaviors/ai_http_client.mm:391-427`

#### 2. Test Connection Timeout Increased
- Increased from 5s to 30s for Osaurus/Ollama/custom providers
- File: `src/platform/macos/config_gui_ai_connection.mm:7`

#### 3. Foundation LLM Race Condition Fixed
- Protected all `s_state` reads/writes with `s_stateMutex`
- `LocalLLM_Generate` now waits up to 30s for model to load instead of returning empty
- Health check timeout increased from 5s to 30s
- Files: `src/common/behaviors/local_llm_model.mm`, `src/common/behaviors/local_llm_inference.mm`, `src/common/behaviors/ai_http_client.mm`

#### 4. Rounded Corners for Nametag and Pomodoro
- Added `DrawRoundedRect()` to `IRenderer` interface
- Implemented in `CGRenderer` (CoreGraphics) and `CairoRenderer` (Linux)
- Nametag uses 6px radius, Pomodoro timer uses 8px, bed uses 6px/4px
- Files: `include/renderer_interface.h`, `include/cg_renderer.h`, `include/linux/cairo_renderer.h`, `src/common/behaviors/behavior_nametag.cpp`, `src/common/behaviors/behavior_pomodoro.cpp`

#### 5. Pomodoro Movement Fixed
- `isResting` was set too early, preventing goose from walking to bed
- Now only sets `isResting = true` after reaching bed position
- Sets `currentSpeed` explicitly when walking to bed
- File: `src/common/behaviors/behavior_pomodoro.cpp:130-166`

#### 6. UI Spacing Fixed
- Added `intercellSpacing = NSMakeSize(0, 4)` for consistent gaps between rows
- Increased header row height from 28px to 36px
- Adjusted header label positioning for better vertical centering
- File: `src/platform/macos/config_gui.mm:12,179,348`

#### 7. Drag No Longer Blocks Screen
- Window now stays click-through (`ignoresMouseEvents = YES`) always
- Item dragging handled by local event monitors which fire at application level
- No longer blocks mouse events to other apps when items exist
- File: `src/platform/macos/renderer.mm:260-266`
