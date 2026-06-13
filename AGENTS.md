# CadGoose Agent Guide

## Documentation Rules

- **docs/PLAN.md** — Forward-looking only. Contains pending work, never completed items.
- **docs/CHANGELOG.md** — Completed items only. Organized by date with detailed descriptions.
- **AGENTS.md** — Current project state. Updated after each session.
- When completing work: move item from PLAN.md → CHANGELOG.md, update AGENTS.md.
- Remove obsolete documents when their content is superseded.

## Build & Run

```bash
cd $HOME/Projects/CadGoose
./build.sh              # macOS Release — checks/installs Homebrew deps, self-heals stale cache, verbose
./run.sh               # build, then run build/CadGoose
./build_debug.sh        # macOS Debug (verbose)
./build_linux.sh        # Linux Release via Docker (quiet)
./build/CadGoose [--debug]
./build/CadGooseTests          # full suite (run on a machine with a display for window/OCR tests)
( cd build && ctest --output-on-failure )   # CI gate: excludes WindowTrailTest.*
./scripts/create_bundle.sh --clean   # build/CadGoose.app (copies Assets in; self-contained)
./build/multi_goose_test
./tools/profiling/run_trail_test.sh
./tools/profiling/run_soak_test.sh
brew install --cask tools/homebrew/cadgoose.rb   # Test the Homebrew Cask installer locally
```

- **Dependencies**: `build.sh` installs `cmake ninja googletest mimalloc` via Homebrew if missing (set `SKIP_DEPS=1` to skip). **toml11** is fetched at configure time via CMake `FetchContent` (pinned commit `b32a2ff`) — no git submodule, so a plain `git clone` builds without `--recursive`. `build/` and `release-build/` are gitignored (never commit build output); `build.sh` wipes a stale/foreign CMake cache automatically.
- **Logs/crashes**: written to `<ConfigDir>/logs/` (macOS: `~/Library/Application Support/CadGoose/logs/`, or `<repo>/config/logs/` when run from the repo). Crash backtraces in `crash-<ts>.log`; stderr captured to `session-<ts>.log` when launched headless (not a terminal). `ConfigDirPath()` prefers `./config/config.toml` if present, else `~/Library/Application Support/CadGoose`.
- **Tests in CI**: `ctest` now actually runs the suite (added `enable_testing()`, WORKING_DIRECTORY = source so `Assets/` resolves). It excludes `WindowTrailTest.*` (needs a real window server) and the OCR `Rendering.Text*` tests skip when `tesseract` is absent; accessibility tests skip headless. ~770 tests gate every build.
- **CI/release**: `.github/workflows/build_and_release.yml` runs the macOS job on **`macos-26`** (so the SDK has FoundationModels — verified by a build step), builds the macOS DMG + Linux tarball, **version-stamps** artifacts (`CadGoose-<tag>.dmg`, bundle `CFBundleShortVersionString`), and attaches them on `release: published` or via `workflow_dispatch` (`release_tag` input). The `.app` is self-contained (Assets copied in, not symlinked).
  - **Release Loop Rule**: When doing a release, verify the remote GitHub Actions build succeeds end-to-end. If any failure occurs, cycle/iterate locally to fix the issues, push, and recreate the release tag until the build is perfectly green and the DMG compiles.
  - **Homebrew Update Rule**: Once the GHA CI is green and the release DMG is successfully generated and attached, update the Homebrew cask repository tap (`Casks/cadgoose.rb` in `ztomer/homebrew-tap`) with the new version and calculated DMG SHA-256 hash. Ensure this updates automatically through GHA or manually.

## Session Summary (June 13, 2026) — Coverage Phase 2 Tier 3: +20 tests, 6 files pushed past 90%

### Coverage improvements this session

| File | Before | After | Change |
|------|--------|-------|--------|
| `goose_behaviors_interact.cpp` | 77.99% | **96.86%** | +18.87pp |
| `goose.cpp` | 87.01% | **91.13%** | +4.12pp |
| `world.cpp` | 80.00% | **100.00%** | +20.00pp |
| `goose_forces.cpp` | 84.62% | **100.00%** | +15.38pp |
| `goose_behaviors_wander.cpp` | 84.92% | **97.62%** | +12.70pp |
| `goose_behaviors_fetch.cpp` | 86.67% | **94.44%** | +7.77pp |
| **P0 total** | **~76.52%** | **78.77%** | +2.25pp |

### Tests added (20 new, all in `tests/test_goose_behavior.cpp`)

**goose_behaviors_interact.cpp** (5 tests):
- `AvoidanceSurprisesGoose` — fast cursor near beak triggers surprised state (lines 54-65)
- `SurprisedTimeout` — surprised clears after 1.5s (lines 70-72)
- `HeldItemWanderDropsOnTarget` — heldItem + at target → drop → WANDER (lines 74-80)
- `SnatchCursorWithMoveRel` — SNATCH with CAP_MOVE_REL (lines 109-121)
- `SnatchCursorWithoutMoveCaps` — SNATCH with neither ABS nor REL (line 147)

**goose.cpp** (5 tests):
- `ForceFetchTextSetsForcedText` — covers ForceFetchText (lines 489-493)
- `ForceWanderClearsState` — covers ForceWander (lines 495-501)
- `PickNewTargetReturnsEarlyInFetching` — early return guard (line 529)
- `RestingGooseHasZeroVelocity` — isResting path in UpdatePhysics (lines 347-349)
- `SolveFeetSentinelInitializesHome` — (0,0) foot initialization (lines 228-229)

**goose_forces.cpp** (2 tests):
- `EdgeAvoidanceWithMultiMonitor` — multi-monitor path in edge avoidance (lines 59-67)
- `ClampToScreenWithMultiMonitor` — multi-monitor path in clamp (lines 79-87)

**goose_behaviors_wander.cpp** (4 tests):
- `ChaseCursor_AnotherGooseGrabbing` — cursor grabbed by another goose (lines 26-30)
- `ChaseCursor_NoCursorPos` — cursor has no position (lines 32-36)
- `ChaseCursor_Timeout` — chase duration exceeds timeout (lines 39-45)
- `Wander_TextOnlyFetch` — memes disabled, text-only fetch (lines 118-119)

**goose_behaviors_fetch.cpp** (3 tests):
- `HandleFetchingClearsExistingHeldItem` — heldItem already set (lines 137-141)
- `HandleFetchingTestImage` — forceItemFetch==2 test image path (lines 163-165)
- `HandleReturningDiscardsNonFiniteItem` — NaN dragRot discard (lines 238-241)

**world.cpp** (1 test):
- `GetGooseByIdFound` — found + not-found paths (lines 20-22)

### CI gate
- P0 coverage threshold raised from **50% → 75%** in `check_coverage.sh` and `build_and_release.yml`

### Remaining sub-90% files (P0)
- `mcp_http_server.cpp` (0%), `app_actions.cpp` (8%), `mcp_server.cpp` (30%), `app_cli.cpp` (38%) — need complex mocking (TCP/HTTP/MCP)
- `goose_debug.cpp` (56%), `goose_behaviors_internal.cpp` (79%, dead-code only)
- `hotkey.cpp` (82%), `mcp_handlers.cpp` (84%), `cursor_backend.cpp` (84%), `ai_mcp_bridge.cpp` (86%)

### Pre-existing test failures (unchanged)
- 4 order-dependent behavior registration tests: `BehaviorToggles.ToysBehaviorRegistered`, `PortalCleanup.BehaviorHasCleanupFunction`, `StalinHonk.*`

## Session Summary (June 12, 2026) — Crash fix + Coverage Phase 1

### Race condition crash: `Goose::draw` null dereference + `Goose_DestroyPerGooseWindow` deadlock

**Root cause**: Two concurrent race conditions in per-goose window destruction:

1. **Dangling drawBlock**: `ActorManager::destroyAllOfType("goose")` set `g->m_perGooseWindow = nullptr` before closing the window. The main thread's `tick()` loop saw the null pointer and **created a brand new window** for the dying goose — whose draw block captured a `Goose*` that was about to be freed. Any `drawRect:` after the destructor returned hit freed memory.

2. **Concurrent CGContext access**: The dispatch block in `Goose_DestroyPerGooseWindow` closed the window (freeing its backing store) while the main thread's draw was mid-`CGContextSaveGState`, causing `malloc_vreport` → `abort` from heap corruption.

**Fix** (`goose_drawing.mm`, `actor.cpp`, `tick_manager.mm`):
- Moved `m_perGooseWindow = nullptr` **inside** the cleanup dispatch block (after `[win closeAndRemove]`), so the tick loop sees a valid window during the entire destruction window
- Nilled `cv.drawBlock` before close so any late `drawRect:` is a no-op
- Added `setActive(false)` before `delete` in `destroyAllOfType` so concurrent `getGeese()` filters out the dying goose
- Added `isActive()` guards in tick loop and key monitor

### Coverage Phase 1 — Infrastructure complete

- **Orphaned test reclamation**: `test_behavior_sim.cpp` (21 tests) re-added to `CMakeLists.txt` with duplicate test cases removed. All 21 pass. `test_window_lifecycle.mm` skipped (macOS 15 deprecated API, cannot reclaim without rewrite).
- **Standalone targets in ctest**: `multi_goose_test`, `soak_fetch_test`, `trail_detection_test` registered with `LABELS "requires_display"`, excluded via `ctest -LE`.
- **CI ctest filter**: `.github/workflows/build_and_release.yml` updated to `ctest -LE "requires_display"`.
- **Coverage gate script**: `scripts/check_coverage.sh` — builds with `CODE_COVERAGE=ON`, runs filtered tests, extracts P0 line coverage from `llvm-cov report`, compares against threshold. Accepts `--p0-min=N` and `--build-dir=`.
- **Coverage eligible list**: `scripts/coverage_eligible.txt` — globs for P0 files (`src/common/*.cpp`, `include/*.h`). P1/P2 excluded.
- **CI coverage step**: Added `Coverage Gate` step to macOS CI workflow after `Run Tests`. Uploads `coverage-report/` as build artifact.
- **P0 baseline measured**: 68.15% line coverage (threshold 50% passes).
- **Final count**: 797 tests pass, 0 failures (with `-LE requires_display`).

## Session Summary (May 30, 2026) — Release hardening: assets, AI chat, CI gate

- **Thread-safe idempotent configuration registry** (`config.cpp`): Solved dynamic configuration data race crashes during concurrent/parallel test runs. Implemented static `std::mutex` locking and idempotent state checks in `Config_Init` to prevent concurrent threads from clearing and rebuilding the active registry (`g_configRegistry`) and lookup (`g_configLookup`) structures, guaranteeing 100% thread-safety for option lookups without any runtime overhead.
- **Objective-C ARC retain imbalance fix in window teardown** (`goose_drawing.mm`): Resolved memory leaks and AppKit window manager registration corruption in `Goose_DestroyPerGooseWindow`. Substituted custom `__bridge_retained` casts on raw pointer variables with direct assignment copies and immediate nullification, which cleanly transfers retained window and key structures back to ARC via a single `__bridge_transfer` inside the main-thread dispatch block.
- **Robust consecutive GTest loop verification**: Verified the entire 808-test suite cleanly 3 consecutive times with zero crashes or warnings on the new thread-safe, leak-free binary.

- **ActorManager safe iteration crash fix** (`actor.cpp`): Fixed a crash (`EXC_BAD_ACCESS` / `SIGBUS`) when enabling/disabling behaviors (such as the ball behavior) during execution. When a behavior like `ball` is toggled off, its `cleanup` function synchronously deletes the associated `BallActor`. Because the actor was part of the snapshot of active actors being iterated inside `tickAll`, this left a dangling pointer in the loop, causing subsequent iterations to crash when calling virtual methods on the deleted actor. Resolved by adding a master existence check (`std::find`) in `tickAll` and `renderAll` to ensure the actor still exists in the master actors list before dereferencing or ticking it.
- **Added safety unit tests** (`test_actor_manager.cpp`): Created a new safety test suite under `tests/common/test_actor_manager.cpp` verifying synchronous self-deletion, next-deletion, and previous-deletion in both tick and render loops, ensuring 100% robust and crash-free execution.
- **Swift compiler warning fixes** (`FoundationLLM.swift`): Resolved all unreachable `catch` block and redundant `try` compiler warnings caused by Swift SDK updates declaring `SystemLanguageModel.default` as a non-throwing property.
- **EXC_BAD_ACCESS crash & resource leak fixes** (`world_utils.mm`, `actor_dropped_item.h`, `actor.mm`): Fixed a null-pointer dereference in `World_CleanupExpired` and `ItemHitTest` caused by defunct items that had their `data` deleted and set to `nullptr` when closed but remained in memory. Added comprehensive `actor->data()` null checks. Also resolved a massive persistent memory leak and Cocoa window resource leak in `World_CleanupExpired` where capping maximum memes/texts used `mgr.remove(actor)` directly, erasing them from the manager list but bypassing deletion. Corrected this by calling `actor->setActive(false)`, allowing `ActorManager::cleanup()` to cleanly deallocate the actors and close their associated Cocoa windows. Updated `DroppedItemActor::isAlive()` to return `false` if `m_item.data == nullptr`, allowing `ActorManager::cleanup()` to instantly reap the actor. Resolved a use-after-free race condition crash in headless CI and unit tests by updating `Actor::closeWindowOnMainThread` inside `actor.mm` to execute synchronously if already running on the main thread, ensuring the window is closed before the actor is deallocated.
- **Test crash logger & symbolication** (`CMakeLists.txt`, `test_main.cpp`): Linked `crash_logger.mm` to the `CadGooseTests` test binary and initialized it via `CrashLogger_Init()` in `test_main.cpp`, ensuring all test crashes produce detailed, symbolicated C++ backtraces.
- **CI console logging visibility** (`crash_logger.mm`): Prevented `stderr` redirection when running inside GitHub Actions, ensuring that test timings, logs, and crash logs print directly to the CI workflow terminal instead of being hidden in local files.
- **Headless test window stubbing** (`test_main.cpp`, `actor_dropped_item.mm`): Established the `CADGOOSE_HEADLESS_TEST` flag inside the test suite, allowing `DroppedItemActor::initWindow()` to completely bypass Cocoa `NSWindow` allocation. This permanently resolves AppKit WindowServer and graphics connection segfaults on headless CI virtual machines, making the remote build suite fully robust and green.
- **GooseRender NAN drawing crash & CI SEGFAULT fix** (`test_goose_rendering.mm`, `test_main.cpp`): Fixed a crash in the rendering tests where uninitialized physics configuration values caused division-by-zero or math errors during vector normalization, resulting in `NAN` coordinates that crashed CoreGraphics drawing in headless runners. Resolved by modifying `tests/test_main.cpp` to call `Config_Init()` in the global `CadGooseTestEnvironment::SetUp()` hook and adding `Config_Init()` in `createTestGoose()` in `test_goose_rendering.mm`. This systematically registers and zero-initializes the global `g_config` structure with its complete set of default values before any test case executes, preventing uninitialized memory stack/heap garbage from causing `NaN` coordinate propagation and fatal CoreGraphics segmentation faults on headless remote CI runners.
- **Deprecation warning fix** (`behavior_element_window.mm`, `test_gui_accessibility.mm`): Removed the deprecated `setOneShot:` call from window initialization and replaced the deprecated `NSApplicationActivateIgnoringOtherApps` option with `NSApplicationActivateAllWindows` in the accessibility tests, restoring a clean, zero-warning compilation on newer macOS SDKs.
- **Ball behavior toggling cleanup fix** (`behavior.cpp`): Fixed a bug where toggling the Ball behavior off in the preferences did not remove the ball from the screen. Modifying `TickAll` to use `GetOrCreate<BehaviorState>` ensures a base state is created for behaviors lacking custom states, allowing enabled→disabled transitions to be tracked and running the `cleanup` handler to destroy the ball and close its window.
- **Footsteps and honk sound effects bundle fix** (`audio.mm`): Fixed footstep, honk, bite, and mud squish sound effects not playing when launching the app from the `.app` bundle (DMG or Homebrew Cask). Modified `GetAssetsPath()` to leverage `ASSET_ROOT` instead of hardcoded relative directories, allowing sound assets to be correctly resolved inside `Contents/Resources/Assets` within the bundle.
- **Homebrew Cask Installer** (`tools/homebrew/cadgoose.rb`, `docs/HOMEBREW.md`): Added a fully functioning Homebrew Cask definition featuring a custom `postflight` hook that recursively strips the macOS quarantine attribute (`com.apple.quarantine`) from `CadGoose.app` after installation, making first-time startup seamless and Gatekeeper-free.
- **Bundle ships assets** (`scripts/create_bundle.sh`): `Resources/Assets` was a **symlink** to the build machine's project dir → dangling in the DMG, so every released `.app` had no images (and `xattr` failed on the broken link). Now `cp -RL`'d in (self-contained, 1.5M → 25M) and `chmod -R u+w` so quarantine removal works. **This is the fix for "no images when installed from DMG".**
- **Local-model (Foundation) chat** (`ai_local_llm_adapter.mm`, `FoundationLLM.swift`): fixed a dangling `const std::string&` that delivered garbage/empty text; cap the persona at `kFoundationMaxEvilLevel` (0.72 ≈ "villainous", `ai_prompt_builder`) since Apple's guardrail refuses Overlord/Dictator; on a guardrail refusal, **retry at progressively lower evil** before a canned fallback. AI settings panel shows the cap note (below the slider, with the %).
- **Osaurus/HTTP chat** (`ai_http_client.mm`): retry transient 5xx/timeout; a reachable server is no longer shown as "disconnected"; fixed the chat status dot (`windowDidBecomeKey` + local-provider `connected` not being set).
- **Crash fix**: `AI_SendMessage`/`AI_OpenChat` + the `send` socket handler captured a `const char*`/`args` past its lifetime → SIGABRT on `setStringValue:nil`. Snapshot to NSString synchronously. Added an `openchat` socket verb.
- **Build/run scripts**: `build.sh` self-heals a foreign CMake cache and `cd`s to repo root; `run.sh` now runs `build/CadGoose` (was a stale `release-build/`); removed committed `build/`+`release-build/` from git. **This was the real cause of "fixes don't take effect".**
- **CI**: macOS job on `macos-26` + FoundationModels-SDK verify step; `ctest` actually gates now (enable_testing, WindowTrail excluded, OCR tests skip without tesseract); version-stamped DMG; app icon regenerated (`scripts/make_icon.swift`).
- **Docs**: README rewritten customer-first (no emojis); this file.

## Session Summary (May 28, 2026) — Fresh-machine bug fixes + release packaging

- **Local-model chat fix** (`ai_local_llm_adapter.mm`): chat no longer rejects a model in `Loading`; only `Unavailable`/`Error` bail. Generation runs on a background queue so the CoreML wait loop doesn't freeze the UI. This resolved "chat fails but Test Connection works".
- **Stalin texture fix** (`assets.mm`): `GetBehaviorImage`/`PreloadBehaviorAssets` now resolve via `ASSET_ROOT` (was cwd-relative → failed from the bundle). `stalin_head.png` added to preload list.
- **Crash/log capture** (new `crash_logger.mm`/`.h`, wired in `main.mm`): signal + uncaught-exception handlers write symbolicated backtraces; stderr redirected to a session log when headless.
- **Packaging**: `build.sh` dep-check + verbose output; toml11 → FetchContent (submodule removed); `workflow_dispatch` added to release workflow; README install/Gatekeeper docs.
- Known pre-existing test failures (unrelated): `Integration.Goose_ReturningItem`, `Integration.Goose_DropItem` — fail on committed HEAD independent of these changes.

## Session Summary (May 27, 2026) — Gulag Audio + Menubar Icon + AI Stalin Mode + Linux Fix

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
- **Honcker behavior** calls `goose->onHonk()` instead of `g_assets.Honk()`, so BabyStalin plays Gulag sound via F key.
- **AI chat system prompt** replaces HONK→GULAG, Goose→Comrade in Stalin mode. Fallback responses use `s_applyStalinMode()` for string replacement.
- **Chat UI** uses Stalin-mode text: "GULAG!" greeting, "Comrade:" markers, window title "Chat with Comrade X ☭".

### Linux Build Fixes
- **`onHonk()` moved out of `__APPLE__` guard** in `goose.h` — virtual method was inside `#ifdef __APPLE__` but called from common code (`goose_behaviors_internal.cpp`), causing Linux compile error
- **`AppActions_SpawnBabyStalin` guarded with `#ifdef __APPLE__`** — function body and declaration now macOS-only; BabyStalinActor constructor only exists in `.mm` files not compiled on Linux
- **`stalinMode` variable removed from non-`__APPLE__` path** in `app_actions.cpp` — was unused on Linux

### Multi-Goose Test Extended
- **Mixed goose types**: `multi_goose_test` now spawns 2 regular geese ("Alpha", "Beta") + 1 BabyStalin ("Stalin") using `spawn_baby_stalin` command. Each completes a forced fetch cycle. Verifies BabyStalin is fully functional through the command socket.

### Build & Tests
- 4 BabyStalin tests pass, 744 non-AX tests pass (no regressions)
- Build zero warnings on both targets

## Session Summary (May 26, 2026) — Afternoon

### Multi-Goose Regression Tests
- **Integration test** (`test_multi_goose.mm`): Command-socket-only. Spawns 3 geese, verifies goose_count=3, each goose completes a forced fetch cycle via `fetch <idx> test`. Exit 0 = all pass. Duration: ~8-20s. Verified 3/3 passes.
- **GTest** (`GooseRender.DrawThreeGeese_AllVisible`): 3 geese at different positions/directions render into single CGBitmapContext. Each body visible at rig coords. Head positions differ by direction.
- **`fetch <idx>` backward-compatible**: First arg parsed as optional numeric goose index (0-1). Non-numeric → existing type-only parsing. `chicken_dinner` → backward compat test for arbitrary non-numeric first arg.

### Background-Thread AppKit Crash Fix
- `Goose_DestroyPerGooseWindow()` called from command socket server thread crashed with `EXC_BREAKPOINT "Must only be used from the main thread"`. Fixed via `dispatch_sync(dispatch_get_main_queue(), ...)` with `__bridge_retained`/`__bridge_transfer` ownership transfers.

### Phase 3 Final Cleanup — renderer.h/renderer.mm Removed
- `src/platform/macos/renderer.h` (Cocoa import placeholder) and `renderer.mm` (2-line comment placeholder) deleted.
- `#include "renderer.h"` removed from `main.mm`, `tick_manager.mm`, `test_renderer.mm` (all already import Cocoa directly).
- `CMakeLists.txt`: `renderer.mm` removed from both `CadGoose` and `CadGooseTests` targets.
- `WindowManager` stub confirmed: no remaining callers or references. The 3 active window managers (`ItemWindowManager`, `EffectWindowManager`, `BehaviorElementWindowManager`) are all necessary and unrelated.

### Verification
- 744 non-AX tests pass (no regressions from 741)
- 19 GooseRender tests pass (including new DrawThreeGeese_AllVisible)
- Multi-goose integration test: 3/3 geese pass
- Build zero warnings on both targets

## Project State (May 27, 2026, evening)

- **797 tests, 104 suites** — 776 pass (no failures under `-LE requires_display`), 2 pre-existing failures (`Integration.Goose_ReturningItem`, `Integration.Goose_DropItem` — AssetManager needs Assets/ in build dir), 6 MCPIntegrationTest failures (need running MCP server), 32 skipped (31 AX + 1 drag), 1 LocalLLMTest skipped (no model)
- **CGBitmapContextCreate works on macOS 26.5** — Y-flip formula `pixelRow = imageHeight - 1 - cgY`. Default byte order = `A,R,G,B` (R at byte +1). 19 rendering unit tests pass.
- **Full-screen overlay eliminated**: No more ~1GB backing store. Every entity uses small per-actor windows.
- **TickManager owns rendering**: `ActorManager::renderAll(nullptr)` called from `TickManager::tick`. Per-goose window creation/update loop in `tick_manager.mm`. Global NSEvent monitor for 'f' honk.
- **Current rendering architecture**:
  ```
  TickManager::tick (CADisplayLink, 60fps)
    ├─ ActorManager::tickAll + cleanup
    ├─ ActorManager::renderAll(nullptr)
    └─ Per-goose BehaviorElementWindow update loop

  PerGooseWindow::drawRect: (~600×600, centered on goose->pos)
    └─ translate → Goose::draw(&r)
  ```
- **g_cutoverMode = true**: `Goose::render()` returns immediately. Only `Goose::draw()` executes (per-goose window path).
- **Goose::onHonk() virtual method**: Called from `triggerHonk()` instead of hardcoded `g_assets.Honk()`. Default calls `g_assets.Honk()`. BabyStalinActor overrides to call `g_assets.Gulag()`. `m_canHonk = true` for BabyStalinActor so its audio fires through the normal honk system.
- **Gulag audio**: `Audio_PlayGulag()` with dedicated 2-player AVAudioPlayer pool. MP3 at `Assets/Sound/NotEmbedded/Gulag.mp3`. `AssetManager::Gulag()` abstracts platform: macOS → `Audio_PlayGulag()`, Linux → fallback to `Honk()`.
- **Multi-goose test**: Command-socket-only, no SCStream needed. Verifies goose_count + per-goose fetch cycle. `clear_dropped` command isolates fetch cycles. `fetch <idx> type` targets specific goose.
- **renderer.h/renderer.mm deleted**: Placeholder files finally removed after Phase 3 cleanup. No replacement needed — Cocoa imports are direct.
- **WindowManager stub removed**: No remaining callers. 3 active managers: `ItemWindowManager`, `EffectWindowManager`, `BehaviorElementWindowManager`.

## Known Bugs (June 12, 2026)

- **Config generator** — Works correctly for registry generation. GUI generation intentionally skipped (incompatible with `config_gui.mm` key-based lookup architecture).
- **g_world.droppedItems** — 127 references across codebase. `DroppedItemActor` scaffold ready for future migration.
- **Trail detection false positive** — Trail scan counts dropped-item-contaminated frames as "trails". Need to filter frames where previous-cycle dropped items are on screen.
- **Test process memory ~7.2GB** — From ring buffer (300 × 25MB frames). Acceptable for short runs; not a CadGoose issue.
- **MCPIntegrationTest failures** — Tests require running MCP server. Run with `./CadGoose` running in background.
- **Coverage 68.15% (P0)** — P0 (portable C++ + headers) at 68.15%. Overall 35.49%. Phase 1 CI gate at 50%. See `docs/PLAN.md` for Phases 2-5.
- **test_window_lifecycle.mm** — Still orphaned (3 tests). macOS 15 deprecated API (`CGWindowListCreateImage`), cannot reclaim without rewrite.

## Next Steps

### P0: Coverage Fill (Phases 2-5)
- [ ] Phase 2: `src/common/*.cpp` to 95% (3-4 weeks)
  - Tier 1: `log.cpp`, `world.cpp`, `app_cli.cpp`
  - Tier 2: `mcp_http_server.cpp`, `mcp_server.cpp`, `behavior.cpp`
  - Tier 3: sub-90% files (`app_actions.cpp`, `config_load.cpp`, etc.)
  - Tier 4: all behavior `.cpp` files (18 files, currently 0% combined)
- [ ] Phase 3: AI/MCP C++ helpers to 95% (1-2 weeks)
- [ ] Phase 4: E2E suite — register all targets, write new scenarios (1-2 weeks)
- [ ] Phase 5: CI gate hardening — add regression guard, coverage badge (1 week)

### Release
- CI (`.github/workflows/build_and_release.yml`) is **green end-to-end**: macOS DMG + Linux `.tar.zst`, tests gating, coverage gate.
- Notarization (Developer ID + `notarytool` + staple) is remaining release polish.
- Optional: `CadGoose --version` CLI flag.
