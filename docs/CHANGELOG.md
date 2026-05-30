# Changelog

## May 30, 2026 — Release hardening: assets, AI chat, CI gate, and safety fixes

### EXC_BAD_ACCESS (SIGBUS) / snapshot iteration crash when disabling Ball behavior
- **Fixed safe Snapshot iteration in ActorManager loops.** When behaviors like the `ball` behavior were toggled off in the settings, the behavior's `cleanup` routine was triggered synchronously inside `BehaviorRegistry::TickAll`, which removed and deleted the active `BallActor`. Because the `ActorManager::tickAll` loop runs over a local `snapshot` copy of the actors list, this left a dangling pointer in the snapshot loop, causing subsequent iterations to dereference the deleted actor and crash with `EXC_BAD_ACCESS` / `SIGBUS`.
- **Added master existence check (`std::find`) in `tickAll` and `renderAll`.** Before dereferencing or ticking any actor, the loops now verify that the actor is still present in the master `actors` registry list, allowing synchronous deletion of actors to occur safely at any point.
- **Added safety regression unit tests.** Created a comprehensive test suite `tests/common/test_actor_manager.cpp` checking self-deletion, next-deletion, and previous-deletion during both tick and render loops, confirming 100% robust and crash-free execution.
- **Fixed Swift compiler warnings in `FoundationLLM.swift`.** Removed redundant `try` keywords and `do-catch` blocks around the non-throwing `SystemLanguageModel.default` property, restoring a clean, warning-free build.

### EXC_BAD_ACCESS (SIGSEGV) crash in World_CleanupExpired and Headless Test Segfaults
- **Fixed a null pointer dereference in `World_CleanupExpired` and `ItemHitTest`.** When a dropped item's close button was clicked, its `m_item.data` was deleted and set to `nullptr`, but the actor was not marked dead. In the next frame refresh, `World_CleanupExpired` iterated over the active dropped items and attempted to read `actor->data()->type`, resulting in a NULL pointer dereference.
- **Fixed a systematic memory and Cocoa window resource leak in `World_CleanupExpired`.** When the number of active dropped memes/texts exceeded the maximum configured caps, the excess actors were directly erased from the manager using `mgr.remove(actor)`. While this removed them from active rendering, it bypassed deletion and deallocation, leaking both the memory and the Cocoa window forever. Resolved by calling `actor->setActive(false)`, which allows `ActorManager::cleanup()` to cleanly deallocate the actors, close their windows on the main thread, and reap the resources.
- Added explicit null pointer checks on `actor->data()` in both `World_CleanupExpired` and `ItemHitTest`.
- Updated `DroppedItemActor::isAlive()` in `include/actor_dropped_item.h` to return `false` if `m_item.data == nullptr` or `!m_active`. This ensures that defunct actors whose close buttons have been clicked are automatically deleted and removed from the active actors list by `ActorManager::cleanup()`.
- **Fixed a use-after-free race condition crash in headless CI and unit tests.** When an actor was destroyed, its destructor scheduled an asynchronous `closeWindowOnMainThread` block to close the window. Because the actor was deallocated immediately while the block was pending, any asynchronous events (like AppKit window teardown or draw calls) accessing the dangling `_item` pointer caused a segfault. Resolved by modifying `Actor::closeWindowOnMainThread` inside `src/common/actor.mm` to execute the cleanup block synchronously if already running on the main thread, ensuring the Cocoa window is fully closed and detached before the actor memory is freed.
- **Enabled symbolicated backtrace logging for unit tests.** Linked `src/common/crash_logger.mm` to the `TEST_SOURCES_COMMON` target in `CMakeLists.txt` and initialized it with `CrashLogger_Init()` in `tests/test_main.cpp`.
- **Disabled `stderr` redirection in CI environments.** Modified `RedirectStderrIfHeadless` in `src/common/crash_logger.mm` to avoid hijacking console output when running inside GitHub Actions, ensuring that any crash dump is directly visible in the remote runner's console.
- **Bypassed Cocoa WindowServer allocations during headless tests.** Added the `CADGOOSE_HEADLESS_TEST` environment variable to `tests/test_main.cpp` and checked it in `DroppedItemActor::initWindow()` (`src/common/actor_dropped_item.mm`) to completely avoid creating `NSWindow` objects when running unit tests. This eliminates all AppKit window server interaction crashes in headless virtual machines.
- **Fixed a secondary crash and remote CI SEGFAULT due to uninitialized config fields in rendering tests.** Under the GTest runner, the global configuration structures were not automatically initialized. This caused mathematical helper calls (like vector normalization) inside goose rendering routines to consume uninitialized memory stack/heap garbage, producing `NAN` coordinate values. When these `NAN` values reached `CGContextDrawPath`, they caused a CoreGraphics internal segmentation fault (in `ripc_GetColor`) on remote VM software rasterizers during drawing tests (e.g. `GooseRender.DrawGoose_Basic`). Resolved by adding a global `SetUp()` hook to the Google Test environment in `tests/test_main.cpp` that calls `Config_Init()`, and adding `Config_Init()` to the rendering test helper `createTestGoose()` in `tests/platform/macos/test_goose_rendering.mm`. This systematically registers and zero-initializes the global `g_config` structure with its complete set of default values before any test case executes, preventing uninitialized memory stack/heap garbage from causing `NaN` coordinate propagation and fatal CoreGraphics segmentation faults on headless remote CI runners.
- **Eliminated deprecation compiler warnings on macOS.** Removed the deprecated `setOneShot:` call from `initWithDrawBlock:deviceX:deviceY:width:height:` inside `src/platform/macos/behavior_element_window.mm`. Also replaced the deprecated `NSApplicationActivateIgnoringOtherApps` option with `NSApplicationActivateAllWindows` in `tests/platform/macos/test_gui_accessibility.mm` to maintain a completely clean, 0-warning compilation pipeline.
- **Fixed the behavior cleanup lifecycle for behaviors without custom state (e.g. Ball).** `BehaviorRegistry::TickAll` previously retrieved behavior state objects via `Get<BehaviorState>()` rather than `GetOrCreate<BehaviorState>()`. As a result, behaviors that did not register custom state classes (like the Ball behavior) never had a state object allocated, leaving their `wasEnabled` state permanently `false`. This prevented the enabled→disabled transition from ever being detected, meaning their `cleanup()` handlers were never executed and turning the behavior off left items (like the ball and its transparent window) permanently stuck on the screen. Resolved by modifying `TickAll` inside `src/common/behavior.cpp` to use `GetOrCreate<BehaviorState>()` to properly initialize and track transition states.
- **Fixed all sound effects and audio playback in the distributed bundle.** The audio path resolver `GetAssetsPath()` in `src/platform/macos/audio.mm` previously used relative directory backtracking on `[NSBundle mainBundle].executablePath`, resolving to `Contents/Assets` when run from a `.app` bundle. However, the macOS app bundle builder places assets inside `Contents/Resources/Assets`. This caused all asset loading to fail silently when the app was run from a bundle (DMG or Homebrew Cask), resulting in complete silence. Resolved by including `assets.h` and modifying `GetAssetsPath()` to leverage `ASSET_ROOT` (which dynamically and correctly resolves the resources path in both source and bundle environments).
- **Added Homebrew Cask installer and documentation.** Created a fully declarative Homebrew Cask in `tools/homebrew/cadgoose.rb` along with an installation guide in `docs/HOMEBREW.md`. The cask includes a `postflight` command hook that automatically runs `xattr -rd com.apple.quarantine` on the installed `.app` bundle, permanently eliminating macOS Gatekeeper and "damaged application" warnings upon first run.

### Bundle ships assets
- `Resources/Assets` was a symlink to the build machine's project directory, leading to a dangling reference in the distributed DMG. Replaced with `cp -RL` in `scripts/create_bundle.sh` to copy actual files, followed by `chmod -R u+w` so quarantine removal works.

### Local-model (Foundation) chat
- Fixed a dangling `const std::string&` that delivered garbage/empty text.
- Capped the persona at `kFoundationMaxEvilLevel` (0.72 ≈ "villainous", `ai_prompt_builder`) because Apple's guardrail refuses Overlord/Dictator.
- Implemented a progressive retry fallback on guardrail refusal, trying lower evil levels before falling back to a canned response.

### Osaurus/HTTP chat
- Added retries for transient 5xx/timeout errors.
- Fixed the chat status dot by updating `windowDidBecomeKey` and properly setting local-provider connection state.

### Socket crash fixes
- Capturing `const char*`/`args` past its lifetime in `AI_SendMessage`/`AI_OpenChat` resulted in SIGABRT on `setStringValue:nil`. Fixed by taking a synchronous snapshot to `NSString`. Added an `openchat` socket verb.

### Build/run scripts & CI
- `build.sh` now self-heals foreign CMake caches and correctly changes to the repo root.
- `run.sh` updated to run the correct executable under `build/CadGoose`.
- Cleaned up gitignore and removed committed build directories.
- Updated CI macOS runner to `macos-26` with FoundationModels-SDK verification. Added test gating to CTest (excluding display-dependent window trail tests).

## May 28, 2026 — Fresh-machine bug fixes + release packaging

### AI chat — local foundation model connection
- **`completeWithLocalLLM` no longer rejects a loading model.** Chat previously bailed with "local brain isn't ready" whenever `LocalLLM_GetState()` wasn't `Ready`, so the first message after launch failed while the model was still warming up — even though the Test Connection panel (which tolerates `Loading`) reported success. Now only `Unavailable`/`Error` bail; `Ready`/`Loading` proceed to generation.
- Generation runs on a background queue, since `LocalLLM_Generate`'s internal wait-for-Ready loop (`usleep`, up to ~30s on the CoreML path) would otherwise block the main thread and freeze the UI.

### Stalin texture intermittently not rendering
- **`AssetManager::GetBehaviorImage` and `PreloadBehaviorAssets` now resolve image paths through `ASSET_ROOT`** instead of raw relative paths. Relative paths resolved against the current working directory, which is `/` when launched from the `.app` bundle, so lazily-loaded behavior images failed to load. `stalin_head.png` was worst-affected as the only behavior image not preloaded; it's now also added to the preload list.

### Crash + log capture
- **New `crash_logger` module** (`src/common/crash_logger.mm`, `include/crash_logger.h`), initialized in `main()` before the app run loop. Installs signal handlers (SIGSEGV/SIGABRT/SIGBUS/SIGILL/SIGFPE/SIGTRAP) and an `NSUncaughtExceptionHandler` that write a symbolicated backtrace to `<ConfigDir>/logs/crash-<timestamp>.log`.
- When stderr is not a terminal (launched from the bundle / Finder), stderr is redirected to `<ConfigDir>/logs/session-<timestamp>.log` so normal diagnostics are preserved for bug reports. Verified end-to-end with a live SIGSEGV.

### Build & packaging
- **`build.sh`**: checks for Homebrew (prints install instructions if missing) and installs any missing dependencies (`cmake ninja googletest mimalloc`); `SKIP_DEPS=1` bypasses. Restored visible build output (removed the `grep`/`2>/dev/null` filtering that made it silent).
- **toml11 is now fetched via CMake `FetchContent`** (pinned to commit `b32a2ff`) instead of a vendored git submodule. The `vendor/toml11` submodule and `.gitmodules` were removed; a fresh `git clone` (no `--recursive`) now builds.
- **Release workflow**: added a `workflow_dispatch` trigger with a `release_tag` input so the build can be run from `main` and attach artifacts to an existing release tag. Upload steps fire on `release: published` or manual dispatch.

### README
- Added macOS install instructions and the Gatekeeper / `xattr -dr com.apple.quarantine` first-run workaround for the unsigned beta.

## May 27, 2026 — Gulag Audio + Menubar Icon + Audio Fix + AI Stalin Mode + Linux Fix

### Gulag Audio for BabyStalinActor
- **Gulag MP3 replaced**: Downloaded from Wikimedia Commons (`Ru-Gulag.ogg`), trimmed to first word only (~1.15s, "ГУЛаг") via ffmpeg silence detection, converted to 44.1kHz stereo 64kbps MP3 (10KB).
- **`Audio_PlayGulag()`**: New function with own 2-player AVAudioPlayer pool. Same `PlayFromPool` pattern as honk.
- **`AssetManager::Gulag()`**: Platform-abstracted method (macOS calls `Audio_PlayGulag()`, Linux falls back to regular honk).
- **`Goose::onHonk()` virtual method**: Default calls `g_assets.Honk()`. Replaces hardcoded call in `triggerHonk()`.
- **BabyStalinActor overrides `onHonk()`**: Plays Gulag clip instead of honk. `m_canHonk` set to `true` (was `false`) so BabyStalin triggers audio through normal honk system.
- **Test updated**: `StalinModeSpawnHasPhotoHead` expects `m_canHonk = true` + verifies `type() == "baby_stalin"`.

### Stalin Mode Menubar Icon
- **Status bar icon changes in Stalin mode**: Shows ☭ (hammer and sickle, U+262D) instead of 🍁 (dark/Canada) or 🪿 (light/default).
- **Dynamic update on mode switch**: `UpdateStatusBarIcon()` C function called from both `setupMenubar` (launch) and `modeChanged:` (GUI mode switch).

### AI Stalin Mode — Honker & Chat
- **Honcker behavior** (F key) now calls `goose->onHonk()` instead of `g_assets.Honk()`, so BabyStalin plays Gulag sound instead of normal honk.
- **AI chat system prompt** replaces "HONK" → "GULAG" and "Goose" → "Comrade" when in Stalin mode via `ai_prompt_builder.mm`. Fallback responses also get string-replaced via `s_applyStalinMode()`.
- **Chat UI** uses Stalin-mode text: "GULAG!" instead of "HONK!", "Comrade:" instead of "Goose:", window title shows "Chat with Comrade X ☭".

### Linux Build Fixes
- **`onHonk()` moved out of `__APPLE__` guard** in `goose.h` — virtual method was inside `#ifdef __APPLE__` but called from common code (`goose_behaviors_internal.cpp`), causing Linux compile error.
- **`AppActions_SpawnBabyStalin` guarded with `#ifdef __APPLE__`** — function body and declaration now macOS-only; BabyStalinActor constructor only exists in `.mm` files not compiled on Linux.
- **`stalinMode` variable removed from non-`__APPLE__` path** in `app_actions.cpp` — was unused on Linux.

### Build & Tests
- 4 BabyStalin tests pass, 744 non-AX tests pass (no regressions)
- Build zero warnings on both targets

## May 24, 2026 — Per-Goose-Window Migration Plan + Memory Investigation + Trail Test v5

### Trail Detection v5: 6/6 Cycles Visible
- **CALayer sublayer removed** — `updateHeldItemLayers` and `heldItemLayers` deleted from `renderer.mm`. Held item was being rendered redundantly via both CoreGraphics (`DrawHeldItem` in `drawRect:`) AND CALayer sublayer (which AppKit creates for `wantsLayer=YES` views that return a non-nil `layer` from `makeBackingLayer`). Removal of sublayer fixes:
  - **~3.7GB/cycle IOSurface leak** — each `display` call on the layer-backed view created a new IOSurface that was never released
  - **Intermittent held-item invisibility** — sublayer memory pressure caused the window server to skip frames; held item only rendered via sublayer (CG path was present but was being composited underneath the sublayer), so skipped frames = invisible held item
- **Post-fix results**: 6/6 cycles visible in trail test, peakRed values consistent (~4,872 for single held item, ~9,600-14,380 with accumulated dropped items)
- **"Double-rendering" explained** — peakRed ~9,900 (cycles 2+) is from previous cycles' dropped items still on screen, not actual rendering trail. Each 100×50 test image contributes ~5,000 extra red pixels. Item lifecycle (15s) causes items to expire mid-test.
- **facingBack investigated** — NOT root cause of invisibility. `goose_facingBack` and `goose_dir` added to status command. Cycles with many `BACK` segments still show item visible — goose changes direction enough during 6-10s carry window.
- **Trail findings documented** — `docs/TRAIL_TEST_FINDINGS.md` captures full investigation for all 5 bugs (B1 resolved, B2 explained, B3 fixed, B4 stable, B5 FP pending).

### Build Fix
- **Duplicate `AssetManager` symbols** — `test_main.cpp` defined mock `g_assets` methods that conflicted with real `assets.mm` implementations when both compiled into `CadGooseTests` target. Removed mock definitions — tests now use the real AssetManager.

### Debug Logging Cleanup
- Removed per-frame `fprintf` calls from `renderer.mm`:
  - `[TICK]` — fired every 60fps frame
  - `[DRAW_RECT]` — fired every drawRect
  - `[COLOR_SPACE]` — fired every drawRect
- Removed red/white test rects from `drawRect:` (were debug artifacts for SCStream color-space investigation)
- Removed empty `setNeedsDisplay:` override
- Debug logging should use `--debug` flag only going forward

### Memory Analysis
- **App memory stable at ~985MB** — no growth over 15-minute soak test. ~1GB is expected for a full-screen transparent overlay on a 5K display (3360×1890×32bpp × multiple IOSurface buffers × CoreAnimation compositing overhead × 2 monitors).
- **Test process memory 7.2GB is the test's ring buffer** — 300 frames × 25MB/frame = 7.5GB. NOT a CadGoose leak. User confirmed acceptable.
- **Root cause of 1GB footprint**: The full-screen transparent overlay window, not any leak. Each per-actor window (BallActor ~200×200, ToyActor ~200×200) uses negligible memory. The goose is the only remaining entity in the full-screen overlay.

### Per-Goose-Window Migration Plan
- `docs/PLAN.md` rewritten with comprehensive migration plan:
  - Phase 0: Tick Extraction — standalone `TickManager` class, display link extracted from `GooseView`
  - Phase 1: GooseActorWindow Additive — per-goose windows alongside full-screen overlay (dual-write)
  - Phase 2: Cutover — full-screen overlay no longer draws goose
  - Phase 3: Cleanup — remove full-screen `GooseWindow`, `GooseView`, dead code
  - Phase 4: Key Event Migration — NSEvent global monitor

### Architecture Documentation
- Full rendering pipeline documented in `docs/PLAN.md` (current vs. target architecture)
- Coordinate transform for per-goose windows specified
- Risk assessment table with mitigation strategies
- Verification checklist for each phase

### AGENTS.md
- Comprehensive project state updated to reflect current architecture, known bugs, and next steps

### Build & Tests
- 773 tests, 102 suites (unchanged) — 741 pass, 1 pre-existing failure, 32 skipped

## May 21, 2026 — Dead Code Cleanup + Window Trail Fix + Warning Fixes

### Dead Code Cleanup (53 items removed)

**Architectural improvement — generic API adoption:**
- `Actor::closeWindowOnMainThread` now used by all 8 actors (`actor_ball`, `actor_breadcrumb`, `actor_dropped_item`, `actor_flower`, `actor_jail`, `actor_leafpile`, `actor_portal`, `actor_toy`) — eliminates 8 duplications of raw `dispatch_async(dispatch_get_main_queue(), ^{...})` in destructors

**Removed dead declarations:**
- `BallActor::initWindow()/updateWindow()` — declared, never defined or called
- `ToyActor::initWindow()/updateWindow()` — declared, never defined or called
- `Goose::UpdateChaseCursor` — empty function, never called (logic handled by standalone `handleChaseCursor()`)

**Removed dead state:**
- `Goose::stepTime` — written 3 places, never read (stepping uses `rig.lFoot.moveDuration`)

**Removed dead file:**
- `src/platform/macos/behavior_mac.mm` (331 LOC) — contained 9 `extern "C"` functions never called, plus 3 unused Obj-C classes (`AccessibilityManager`, `FailsafeHotkeyMonitor`, `CursorHijackProtection`)

**Removed unused config fields (9):**
- `SpawnConfig::maxFetchingGeese` — logic uses `ItemConfig::maxFetchGeese` instead
- `RigConfig::headBaseX`, `headBaseY`, `head1OffsetY`, `head1OffsetZ`, `head2OffsetY`, `strideMax`, `stepLiftHeight` — registered in UI but never used in rig calculations
- `ItemConfig::attackMouseBiasMax` — never clamped against

### Window Trail Investigation (In Progress)
- **Root cause identified:** Trail appears when the **goose drops an item window** onto the screen (not during user drag). `orderFront` makes window visible immediately, but `drawRect` is scheduled asynchronously. If window server composites before `drawRect` runs, uninitialized backing store contains stale screen pixels (the trail).
- **Attempted fixes (failed):** `[self.contentView display]` before `orderFront` — does NOT trigger synchronous drawRect. `[self setOneShot:YES]` — deprecated, doesn't work. `NSBackingStoreNonretained` — deprecated.
- **Current fix (testing):** Off-screen window creation — create window at (-10000,-10000), call `orderFront` to trigger initial `drawRect` (clears backing store), then move to final position with `setFrame:display:`. Ensures backing store is clean before window appears at drop location.
- **Detection system:** Created `test_window_trail.mm` with 5 tests to verify fixes.
- **Why intermittent:** Depends on window server scheduling, CoreAnimation transaction timing, vsync alignment, and runloop state.
- **See:** `docs/WINDOW_TRAIL_DEBUG.md` for full debug log and attempted fixes.

### Compilation Warning Fixes (5 warnings)
- `config_gui_helpers.h` — moved category method declarations (`promptPreviewForEvilLevel:`, `modelsEndpointForTest`, `refreshModels:`, `testConnection:`, `evilSliderChanged:`) to category interfaces in header, eliminating "category implementing method also implemented by primary class" warnings
- `assets.mm` — replaced deprecated `NSCompositeSourceOver` with `NSCompositingOperationSourceOver`
- `window.mm` — fixed missing `self` argument in `DEBUG_LOG` format string (`%p` had no corresponding argument)

### Autumn Leaves Improvements
- Spawn rate reduced 3x (1/10800 per tick → ~3 min average)
- Lifetime increased to 180s (was 120s)
- Fade time increased to 20s (was 10s) for smoother fade out

### Build & Tests
- **776 tests, 103 suites** — 744 pass, 32 skipped (same as before — no regressions)
- Zero compilation warnings

## May 19, 2026 — Leaf Pile Scaling + Footprint Fix + Behavior Toggle + Pomodoro + Leaf Window + UI Spacing + Goose Sync

### Bugs Fixed (7)
- **Leaf pile rendering not scaled by globalScale** — `actor_leafpile.mm` computed window size and leaf positions using raw world units without applying `g_config.general.globalScale`. Fixed: window size, leaf positions (`curPosPlanar`, `curPosZ`), and leaf ellipse dimensions now all multiplied by `globalScale`, matching the scaling approach used by Goose rendering and item windows.
- **Muddy footprints not showing** — Effect registration used hardcoded magic numbers (`1` for Footprint, `5` for PomodoroBed) instead of actual `EffectType` enum values (`0` and `1`). Windows created with wrong type, so `drawRect` never matched the footprint rendering path. Fixed both `effect_reg_footprint.mm` and `effect_reg_pomodorobed.mm` to use `EffectTypeFootprint` and `EffectTypePomodoroBed`. Also removed 4 unnecessary `(int)` casts from enum comparisons in `effect_window.mm`.
- **Ball toggle broken — behavior doesn't cleanup when disabled** — `BehaviorRegistry::TickAll` ran behaviors when `isEnabled || wasEnabled` but never called `cleanup()` on the enabled→disabled transition. Fixed: `TickAll` now detects both enabled→disabled (calls `cleanup()`) and disabled→enabled (calls `init()`) transitions at runtime.
- **Pomodoro issues** — Goose now quiet during rest phase (`triggerHonk` returns early when `isResting` is true). Timer resets on re-enable (handled by behavior toggle fix calling `init()`). Bed rendering fixed by the enum fix above.
- **Leaf window bounds** — Leaf pile windows were too small for kicked leaves, causing clipping. Increased window padding from 20px to 40px (`kKickScatterPad`) to accommodate leaf scatter trajectory when kicked.
- **UI spacing in preferences panel** — Header rows had too much space below them (before their group items) and not enough space above them (after previous group). Fixed by adjusting header label position (y=2→8, height=32→20) and reducing header row height (36→28px) while slightly tightening behavior rows (52→48px).
- **Goose synchronization** — Multiple geese doing the same thing at the same time. Added `randomOffset` field (0-3s) to Goose struct, applied to fetch cooldowns and honk timing. Each goose now has unique timing offsets.

### Build & Tests
- **773 tests, 102 suites** — 741 pass, 1 pre-existing failure (`LocalLLMTest.GenerateWithHighTemperatureDoesNotCrash`), 32 skipped (31 AX + 1 drag test)
- 3 new unit tests added for leaf pile scaling (`tests/common/test_leafpile_scaling.cpp`)
- 5 new unit tests added for footprint registration (`tests/common/test_footprint_registration.cpp`)
- 5 new unit tests added for behavior toggle transitions (`tests/common/test_behavior_toggle.cpp`)
- 3 new unit tests added for pomodoro quiet/reset (`tests/common/test_pomodoro_quiet.cpp`)

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

## May 26, 2026 — Multi-Goose Test Suite + Phase 3 Final Cleanup

### Multi-Goose Regression Test
- **New test: `test_multi_goose.mm`** — Command-socket-only integration test (no SCStream needed). Verifies 3 geese spawned, each completes a forced fetch cycle via `fetch <idx> test` command. Exit 0 = all pass, 11+ = failure count.
- **New test: `GooseRender.DrawThreeGeese_AllVisible`** — GTest-based unit test verifying 3 geese render into a single CGBitmapContext at different positions. Each goose's body visible at its rig coordinates. Different head positions confirmed.
- **Supporting infrastructure**:
  - `app_actions.cpp` — `fetch` accepts optional goose index (`fetch <idx> type`), backward-compatible
  - `app_actions.cpp` — `clear_dropped` command removes all dropped items (isolates fetch cycles)
  - `tools/profiling/run_multi_goose_test.sh` — wrapper script with auto-launch
- **Files**:
  - `tests/platform/macos/test_multi_goose.mm` (new)
  - `tests/platform/macos/test_goose_rendering.mm` (modified — added DrawThreeGeese test)
  - `src/common/app_actions.cpp` (modified — goose index + clear_dropped)
  - `CMakeLists.txt` (modified — added multi_goose_test target + new source)
  - `tools/profiling/run_multi_goose_test.sh` (new)

### Background-Thread AppKit Crash Fix
- **Problem**: `Goose_DestroyPerGooseWindow()` called from command socket server thread (non-main) crashed with `EXC_BREAKPOINT/SIGTRAP "Must only be used from the main thread"`
- **Fix**: Dispatch window close/unregister to `dispatch_sync(dispatch_get_main_queue(), ...)` with `__bridge_retained`/`__bridge_transfer` ownership transfers
- **File**: `src/common/goose_drawing.mm:332-356`

### Phase 3 Final Cleanup — renderer.h/renderer.mm Removed
- **Files deleted**: `src/platform/macos/renderer.h` (placeholder with Cocoa imports), `src/platform/macos/renderer.mm` (2-line comment placeholder)
- **Removed `#include "renderer.h"`** from:
  - `src/platform/macos/main.mm:19` (already imports `<Cocoa/Cocoa.h>` directly)
  - `src/platform/macos/tick_manager.mm:2` (already imports `<AppKit/AppKit.h>` directly)
  - `tests/platform/macos/test_renderer.mm:3` (already imports `<Cocoa/Cocoa.h>` directly)
- **CMakeLists.txt**: Removed `src/platform/macos/renderer.mm` from both `CadGoose` and `CadGooseTests` target source lists
- **Verification**: 744 tests pass (no regressions), multi-goose integration test passes 3/3 geese, build zero warnings
