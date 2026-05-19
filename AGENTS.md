# CadGoose Agent Guide

## Documentation Rules
- **PLAN.md** — Forward-looking only. Contains pending work, never completed items.
- **CHANGELOG.md** — Completed items only. Organized by date with detailed descriptions.
- **AGENTS.md** — Current project state. Updated after each session.
- When completing work: move item from PLAN.md → CHANGELOG.md, update AGENTS.md.
- Remove obsolete documents when their content is superseded.

## Build & Run
```bash
cd /Users/ztomer/Projects/CadGoose
mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(sysctl -n hw.logicalcpu)
./build/CadGoose [--debug]
./build/CadGooseTests
```

## Project State (May 18, 2026)
- **755 tests, 99 suites** — 723 pass, 1 pre-existing failure (`LocalLLMTest.GenerateWithHighTemperatureDoesNotCrash` — FoundationModels timeout), 32 skipped (31 AX + 1 drag test)
- **Actor System (R1)** — All world entities migrated to Actor pattern. `Actor` base class (`include/actor.h`) with `ActorManager` singleton. 9 actor types: `BallActor`, `ToyActor`, `FlowerActor`, `JailActor`, `PortalActor`, `BreadcrumbActor`, `LeafPileActor`, `Goose`, `DroppedItemActor`. Each actor has its own window, lifecycle (tick/render/cleanup), managed by `ActorManager`. `Goose` extends `Actor`. `renderer.mm` uses `ActorManager::tickAll()` and `ActorManager::renderAll()`. All `g_geese` iteration code migrated to `ActorManager::getGeese()`. `g_geese` retained for lifecycle management only (spawn, clear, config save/load).
- **DroppedItemActor** — Created (`include/actor_dropped_item.h`, `src/common/actor_dropped_item.mm`). Passive actor for memes/text/toys on ground. Window managed by `ItemWindowManager::syncWindows`. `g_world.droppedItems` retained for compatibility (127 refs across codebase).
- **EffectWindow Registration (R2)** — Each effect type self-registers via `EffectRegister()` with callbacks. `EffectWindowManager::syncWindows` is generic — iterates registrations instead of monolithic switch statements. Files: `include/effect_registration.h`, `src/platform/macos/effect_registration.mm`, `src/platform/macos/effect_reg_footprint.mm`, `src/platform/macos/effect_reg_pomodorobed.mm`. Only 2 effect types remain: `Footprint`, `PomodoroBed`. All other effects migrated to Actors. `effect_window.h` enum cleaned to only active types.
- **EventBus** — Type-safe event bus (`include/event_bus.h`) for decoupled behavior signaling. 13 event types: `GooseHonked`, `GooseDamaged`, `ItemDropped`, `ItemEaten`, `GooseJailed`, `GooseFreed`, `PomodoroPhaseChanged`, `GooseStuck`, `CursorFastMove`, `ToySpawned`, `BallKicked`, `BreadcrumbDropped`, `GooseTeleported`. Thread-safe with `shared_mutex`, unique subscription IDs, unsubscribe support. 22 unit tests. Integrated into `behavior_anger.cpp` — anger subscribes to `GooseHonkedEvent` and `CursorFastMoveEvent`.
- **ItemRenderer strategy** — `ItemRenderer` base class + `MemeItemRenderer`, `TextItemRenderer`, `ToyItemRenderer`. Eliminates type-specific branching in `goose_drawing.mm`. `DrawHeld()` for goose-held items, `DrawDropped()` for ground items (returns bool for close button visibility — toys return `false`). Factory via `ItemRenderer::ForType(ItemData::Type)`.
- **IRenderer full migration** — All 14 behaviors now use `IRenderer` interface instead of direct CoreGraphics calls. `CGRenderer` wraps `CGContextRef` with `DrawEllipse`, `DrawEllipseOutline`, `DrawLine`, `DrawRect`, `DrawRectOutline`, `DrawPolygon`, `DrawImage`, `DrawText`, `SaveState/RestoreState`, `Translate/Scale/Rotate`, `SetAlpha`, `nativeContext()`. Added `DrawPolygon()` for toy stick rendering.
- **CairoRenderer** — Linux `IRenderer` implementation (`include/linux/cairo_renderer.h`) using Cairo + Pango. Full parity with `CGRenderer`: all primitives, transforms, state management, alpha tracking, text via Pango. Header-only, guarded with `#ifndef __APPLE__`.
- **Coordinate system** — Typed coordinate wrappers (`DevicePoint`, `WorldPoint`, `ScreenPoint`, `ViewPoint`) in `coordinate_system.h`. Prevents coordinate space mixing bugs. All goose pos/target/vel/item.pos are DEVICE coords. Rig parts are WORLD coords. Explicit transforms via `CoordTransform`. Single hit test logic in `HitTest` struct.
- **Behavior rendering scaling** — `renderer.mm` applies `CGContextScaleCTM` transform around `goose->pos` before calling `RenderPass`. All behaviors use raw pixel values (no manual scaling). Goose-relative positions use raw rig coords (`goose->rig.neckHead`). Global device coords (toys, ball, breadcrumbs, portals, jails, flowers, bed) converted to transform space: `drawPos = goose->pos + (devicePos - goose->pos) / scale`. Single unified rendering approach matching main goose drawing.
- **Item drag controller** — `ItemDragController` class extracts shared hit-test + drag logic from `renderer.mm`. Both AppKit responder chain and NSEvent monitor handlers delegate to it. Single source of truth for drag operations.
- **Timer utility** — `Timer` struct in `timer.h` with `Start()`, `Elapsed()`, `IsExpired()`, `Reset()`, `Remaining()`, `Progress()`. Header-only, zero overhead. 10 unit tests.
- **Ball state** — All ball physics state migrated from 8 static globals to `BallState` via `BehaviorStateManager`. Multi-goose support restored. Shared `CGImageRef` assets remain static (acceptable).
- **Footprint windows** — Muddy footprints use independent `EffectWindow` windows (`EffectTypeFootprint`) instead of goose renderer. Each footprint gets transparent click-through window that fades over lifetime and auto-cleans on expiry. Persists on screen independently of goose movement/respawn. `EffectWindowManager::syncWindows` creates/updates/removes footprint windows.
- **Headless rendering tests** — 22 tests covering hit-test at different scales, close button hit-test, coordinate transforms, item coords, screen bounds, and drag controller behavior. No AppKit dependencies, runs in CI.
- **Config code generation** — YAML schema + Python generator for registry entries and GUI rows. Expanded from 7 to 15 sections (added Spawn, Rig, Snatch, Mud, Honk, Step, Item, Render). 80+ new fields covered with proper min/max/step ranges. Generator in `tools/generate_config.py`.
- **Behavior toggle system** — All 19 behaviors use `BEHAVIOR_DEF*` macros (`behavior_registry.h`) that enforce `enabledPtr` == `configPtr` pointing to the same config bool. Prevents the toggle-desync bug where behaviors kept running after user disabled them. Four macro variants: `BEHAVIOR_DEF` (standard), `BEHAVIOR_DEF_STARTER` (starter behavior), `BEHAVIOR_DEF_GROUND` (ground-pass rendering), `BEHAVIOR_DEF_CUSTOM` (custom cleanup/priority/renderOnGround/isStarter).
- **Config key consistency** — GUI `addRow:` calls, config registry, and behavior `configPtr` all use the same registry keys (e.g., `ball_enabled`, not `behaviors.fun.ball`). Regression tests in `test_gui_config.mm` verify: every GUI key exists in registry with correct pointer, every registry toggle has a GUI row, no duplicate pointers.
- **Preferences panel** — Three tabs (Behaviors/Appearance/AI). Behaviors tab: 19 toggles in 5 categories (FUN/JOY/CONTROL/INFO/SYSTEMS), detail panel with sliders/hotkey fields per behavior. All toggles functional and verified via AX accessibility tests.
- **AI chat** — HTTP-only LLM client (`ai_http_client.mm`) with function calling (MCP tools as OpenAI tools), `think...>` block stripping for Gemma/Qwen, connection health check on chat window open, 4-stage fallback chain (LLM→tools→MCP bridge→keywords). Foundation provider routes to local CoreML LLM. Model switching reflected in chat via `refreshConnection()`. AI code split into 5 files: `ai_http_client.mm`, `ai_local_llm_adapter.mm`, `ai_model_profiles.mm`, `ai_prompt_builder.mm`, `ai_think_block_stripper.mm`.
- **MCP server** — 12 tools, 5 resource URIs, dual transport: Unix socket (`/tmp/desktop-goose-mcp.sock`) + HTTP (port 31072), JSON-RPC 2.0 (see [docs/MCP.md](docs/MCP.md))
- **AI→MCP bridge** — natural language routing of 8 command categories, first-word prefix matching (see [docs/MCP.md](docs/MCP.md))
- **Fallback chain** — LLM errors displayed to user; MCP bridge still works when LLM is down
- **AI Text Memes** — Two-pool system: **AI pool** (LLM-replenished) + **file pool** (always available). `Dequeue()` prefers AI, falls back to file. Async generator builds prompts from evil level + active behaviors + color mode + random seed. Temperature 0-2 (default 1.2). Cooldown = response_time × 1.5 (min 2s). Dedup via `RingBuffer<size_t, 500>`. Auto-save to `ConfigDir/TextMemes/`. Queue max 5 pending. Foundation provider falls back to file texts when no local model.
- **Local CoreML LLM (`local_llm*.mm`)** — Direct `MLModel` integration, discovers Apple Intelligence FM GenerativeModels assets at system paths, `ConfigDir/Models/`, or custom search paths from config. Tokenizer from model package. Autoregressive top-K (K=40), 48 token limit, 512 context. State machine: Unavailable → Loading → Ready → Error. Thread-safe result queue. Split into tokenizer/model/inference modules.
- **Text generation backends** — Priority: (1) local CoreML LLM → (2) HTTP provider (Osaurus/Ollama/custom) → (3) file texts. 11 API contract tests (no model required).
- **AI text paper canvas** — AI texts render with cream paper `#F5F0E8` + sepia stroke vs yellow for file texts. `"AI"` label. `ItemData::isAIGenerated` flag.
- **Chewing animation** — When goose eats a breadcrumb, beak splits into upper/lower halves oscillating open/close at 10 Hz for 0.4s with decaying amplitude. `Goose::isChewing` + `chewingStartTime` fields.
- **Breadcrumbs** — `Hold RightShift` to drop crumbs at cursor, expire after 10s. Migrated to `BreadcrumbActor` with own window. `Crumbs` struct moved to `world.h` with global `g_crumbs` RingBuffer. Goose eats on proximity (`footSize × 2`), plays `Bite()`.
- **Pomodoro** — work=rest (green, goose walks to bottom-right corner then sleeps with bed + ZZZ animation), break=manic (red "ATK!"). Timer text uses `CGContextSetTextMatrix` Y-flip. `bedPosition` initialized to bottom-right in `init()`.
- **Toys fetch** — Goose picks up toys into `Goose::heldItem` (new `ItemData::TOY` type), transitions to `RETURNING` state, drops at random location. `CreateToyItem(bool isStick)` creates stick/ball CGImage. Toys managed by `ToyActor` with own window.
- **Fetch loop** — `fetchCooldown=4s`, `fetchEdgeMargin=80`, CHASE_CURSOR timeout at `fetchCooldown*2`. Detailed [FETCH] logging for debugging.
- **Hotkey system** — `hotkey.h`/`.cpp` (22 tests). All behaviors use semantic hotkey strings. Toggle persistence via config registry.
- **Ring buffers** — `ring_buffer.h` with `ConstIter`. Applied to breadcrumbs (200), jails (10), footprints (500), seen hashes (500), goose names (10). AI chat history capped at 100.
- **Memory profile** — Physical ~460-490 MB (IOSurface 193.5 MB at 3456×2234×8 triple-buffered + CoreAnimation 229-293 MB). RSS stable 82-146 MB with **zero growth** over 15-min soak. No Metal textures — all CoreGraphics.
- **macOS bundle** — `.app` crashes on macOS 26.5 with ad-hoc signing. Root cause: ad-hoc signed binaries rejected by MTLCompilerService XPC. Two trigger paths: `MLComputeUnitsAll` in CoreML + `wantsLayer=YES` on CALayer-backed views. Fix requires Apple Developer signing + hardened runtime + `com.apple.security.cs.allow-jit` entitlement. Use `./build/CadGoose` directly.
- **Velocity** — `steerSeekForce=4.0`, `maxForce=1000` produce ~800-1000 px/s² acceleration; walk ~0.25s, run ~0.48s.
- **Stuck detection** — Goose tracks position every frame. If moves < 5px over 3s → picks new target. Logs [STUCK] events.
- **Portal persistence** — Portals persist across goose spawns. Migrated to `PortalActor` with own window. No longer cleared on goose init.
- **Mute option** — Apple B&W icons (NSImageNameTouchBarAudioOutputMuteTemplate/NSImageNameTouchBarAudioInputTemplate) in menubar after Honk! option.
- **Geese names persistence** — Saved to config file as TOML array, loaded on startup. RingBuffer<std::string, 10>.
- **AI prompts** — All prompts defined in `Assets/prompts.toml` for easy configuration. Evil levels, text meme system prompt, chat fallback responses, local LLM search paths.
- **Removed features** — Petting, weekend mode, nighttime mode, banish, idle preening, AI typing sounds, custom behavior toggle, interactive drops puddles, Sonic mode, Custom Affirmations.
- **Added features** — Toys enabled toggle, bed assets from Toys mod for pomodoro rest, Avoidance, Boredom Sigh, Window Peeking, Custom Affirmations, Interactive Drops (flowers only).
- **Code quality** — All files under 500 LOC. No magic numbers (extracted to named constants). DRY violations eliminated. Prompts in TOML. Local LLM search paths configurable. `BEHAVIOR_DEF*` macros prevent toggle-desync bugs. Typed coordinate system prevents space mixing bugs. All behaviors use `IRenderer` interface for platform-agnostic rendering.
- **Actor-per-object architecture** — ALL world entities now use Actor pattern: goose (`Goose`), ball (`BallActor`), toys (`ToyActor`), flowers (`FlowerActor`), jails (`JailActor`), portals (`PortalActor`), breadcrumbs (`BreadcrumbActor`), leaf piles (`LeafPileActor`), dropped items (`DroppedItemActor`). Each actor has own window, lifecycle, managed by `ActorManager`. Zero behaviors manage entity state directly.
- **EffectWindow types** — 2 remaining effect types: `Footprint`, `PomodoroBed`. Manager uses circular buffer (max 50 windows). Other effects migrated to Actors.
- **Ball cursor hit-test** — Ball window now sized to ball's visual radius (`ball->radius * 2`), cursor kick detection uses circle hit-test (`distToCursor < ball->radius`) instead of footSize-based margin.
- **Autumn leaves interaction** — Goose kick proximity for leaf piles uses `g_config.render.footSize` instead of hardcoded `4.0f`, matching the goose's actual size.
- **NSWindow crash fix** — `releasedWhenClosed = NO` set on all dictionary-managed windows (`item_window.mm`, `effect_window.mm`) to prevent double-release when windows are removed from dictionary before system deallocation.
- **item_window.mm refactored** — `IsItemValid()` helper extracted (eliminated 7 repetitions), `GetMouseDeviceCoords()` helper extracted (eliminated 4 repetitions). Named constants: `kItemDragLogPath`, `kCloseButtonPadding`. ~110 LOC reduction (17%).
- **behavior.h split** — Split into 4 focused headers: `behavior_state.h` (29 LOC), `behavior_manager.h` (86 LOC), `behavior_registry.h` (107 LOC), `behavior_api.h`. 15 behavior state structs in individual files under `include/behaviors/states/`.
- **Goose monolith deconstructed** — `Update()` split into `UpdatePhysics()`, `UpdateDetection()`, `UpdateAnimation()`, `UpdateDebug()`. Each focused on single responsibility.
- **WorldContext** — Exists in `world.h` with all global state encapsulated: `geese`, `monitors`, `droppedItems`, `footprints`, `crumbs`, `leafPiles`, screen dimensions, cursor state.

## Known Bugs (May 18, 2026)
- **Config generator broken** — `tools/generate_config.py` produces incorrect code. Generated file deleted; behaviors tab uses hand-written registry pattern.
- **behavior_manager.h:78** — Hash collision risk in `MakeKey()` (32-bit behavior hash, improved from 16-bit but still theoretical collision possible)
- **ai_http_client.mm** — Recursive tool call loop could stack overflow if tools call each other (max 5 turns)
- **ui.cpp (Linux)** — Fragile monitor-to-window matching in tick loop
