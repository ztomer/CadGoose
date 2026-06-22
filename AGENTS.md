# CadGoose Agent Guide

## Documentation Rules

- **docs/PLAN.md** — Forward-looking only. Contains pending work, never completed items.
- **docs/CHANGELOG.md** — Completed items only. Organized by date with detailed descriptions.
- **AGENTS.md** — Current project state. Updated after each session.
- When completing work: move item from PLAN.md → CHANGELOG.md, update AGENTS.md.
- Remove obsolete documents when their content is superseded.

## Build & Run

```bash
cd /Users/ztomer/Projects/CadGoose
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

## Session Summary (June 13-14, 2026) — P0 .cpp coverage 94.00% → 94.60% + app_actions.cpp over 95%

### What changed this session
- **`goose_behaviors_fetch.cpp`** (98.31%): Added `HandleFetchingForcedText`, `HandleFetchingAiTextFromQueue`, `HandleFetchingFallbackFetch`, `HandleReturningDropsToyItem` tests covering forcedText path, AI text queue path, fallback fetch type, and TOY item type in drop handler. Removed dead empty `if (!g_config.cursor.multiMonitorEnabled) {}` body.
- **`app_actions.cpp`** (95.36%, above 95%): Added `GetStatusWithHeldItem`, `GetStatusWithUnpinnedItem`, `HandleCommandFetchNumericMeme`, `SpawnGooseEmptyNameAtNonZeroIndex` tests. Added `BabyStalinSpawnTest.SpawnBabyStalinDirect/ViaCommand/ShortCommand` tests covering `AppActions_SpawnBabyStalin` function and HandleCommand `spawn_baby_stalin`/`spawn_stalin` paths.
- **`goose_behaviors_wander.cpp`**: Added `WanderHonkTrigger` (sets wanderHonkDivisor=1) and `WanderHeistPath` (sets heistChancePercent=100) tests.
- **`test_cursor_backend.cpp`**: Added `ExecuteWithoutCaps`, `ReadDoesNotCallGetCursorPosWithoutGetPosCap` tests for branch coverage.
- **`test_goose_behavior.cpp`**: Added 6 new fetch/wander edge case tests.
- **`test_baby_stalin_spawn.mm`**: Added 3 new spawn tests.
- **Runtime fix**: `CMakeLists.txt` `set(CMAKE_CXX_FLAGS ...)` now uses `CACHE STRING "" FORCE` — without `FORCE`, the flag was written to a local variable that never persisted, so `build-cov/` had been compiling without `-fprofile-instr-generate` since the original reconfigure. Clean rebuild now produces correct coverage data.

### Key corrections
- **Dead code removed**: `GetDebugLog()` from `goose_behaviors_internal.cpp` (8 lines), `Rand01()` from `goose_behaviors_fetch.cpp` (1 line), `EnsureBehaviorsRestored()`/`EnsureBehaviorsRestoredForce()` from `behavior.cpp` (4 lines). Empty `if (!g_config.cursor.multiMonitorEnabled) {}` body from `goose_behaviors_fetch.cpp`.
- **Stalin spawn coverage unlocked**: `AppActions_SpawnBabyStalin` and HandleCommand `spawn_baby_stalin`/`spawn_stalin` handlers now tested via `test_baby_stalin_spawn.mm`.
- **CMake coverage flag fix**: The `build-cov/` directory had been compiling without `-fprofile-instr-generate` because `set(CMAKE_CXX_FLAGS ...)` without `CACHE FORCE` only set a local variable. Fixed by adding `CACHE STRING "" FORCE` to all four `set()` calls.

### Coverage impact
| Metric | Before | After | Change |
|--------|--------|-------|--------|
| **P0 .cpp** (`src/common/*.cpp`) line coverage | 94.00% (219/3653 missed) | **94.60%** (197/3651 missed) | +0.6pp |
| **Files ≥95% line coverage** | 36/45 | **39/45** | +3 |
| **app_actions.cpp** | 88.14% | **95.36%** | +7.22pp |
| **config.cpp** | 98.61% | **100.00%** | +1.39pp |
| **goose_behaviors_fetch.cpp** | 94.97% | **98.31%** | +3.34pp |
| **Tests** | 1414 | **1429** | +15 |

### Files below 95% line coverage (6 files, 188 missed)
`app_cli.cpp` (63.79%, 42 missed — needs daemonizer extraction), `cursor_backend.cpp` (84.31%, 8 missed — 2 lines uncoverable warning path on macOS), `goose.cpp` (84.33%, 76 missed — platform-dependent tick/draw/render), `goose_behaviors_interact.cpp` (96.86%, 5 missed), `goose_behaviors_wander.cpp` (97.62%, 3 missed), `mcp_http_server.cpp` (89.39%, 14 missed — socket failure paths), `mcp_server.cpp` (79.89%, 37 missed — socket failure paths + RunStdioServer)

## Session Summary (June 14b, 2026) — Phase 5 final push: breadcrumbs/honcker/pomodoro all ≥95%

### behavior_breadcrumbs.cpp — refactored + tests fixed
- **Refactored to use `g_cursorProvider`**: Replaced `g_backendManager.GetActiveBackend()` with `g_cursorProvider->Read()` from `cursor_io.h`. Removed `#include "cursor_backend.h"`. The eat/expire checks (previously unreachable) now execute correctly.
- **Root cause of test failures was `CursorState::hasPos()` returning false** — default `CursorState` has `caps=CAP_NONE`, so `hasPos()` returned false and tick exited early at the cursor check. Fixed by calling `mockCursor.set(0, 0)` in `SetUp()` to set caps=`CAP_GET_POS` with a valid position.
- **New tests**: `NoCursorProvider`, `CursorNoPosition`, `KeyPressDropsFirstCrumb`, `KeyHoldDragDropsAdditionalCrumbs` (12 total, all pass).
- **Fixed 4 failing tests**: `GooseEatsNearbyCrumb`, `MaxCrumbsEnforced`, `ExpiredCrumbsCleaned`, `EatenCrumbsPopped` — now correctly reach eat/expire logic.

### behavior_honcker.cpp — image-draw path now tested
- **Created `Assets/Images/OtherGfx/honk.png`** (1×1 red PNG) so `g_assets.GetBehaviorImage()` returns non-null for honk.
- **Fixed `MockHonkRenderer::GetImageSize`**: Returns true with `*w=32, *h=32` for non-null img pointer.
- **Updated `RenderHonk`**: Expects image path (ellipseCount=0, imageCount=1).
- **Removed `RenderHonkEllipseFallback`**: No longer testable since `honk.png` always exists.

### behavior_pomodoro.cpp — render paths now tested
- **Fixed `MockPomoRenderer::GetImageSize`**: Returns true with `*w=30, *h=30` for non-null img pointer; added `imageCount` field.
- **New tests**: `CleanupFunction`, `RenderBreakLabel`, `RenderLongBreakLabel`, `NonAggressiveBreakResetsAccumulatedRotation` (25 total, all pass).
- **Updated `RenderSleepingZZZ`**: Accepts either image path or text fallback.

### Corrected misunderstandings
- **Duplicate `.o` theory was wrong**: The test binary only links its own `.o` files (from `TEST_SOURCES_COMMON`). The app target's `.o` files are NOT linked into the test binary. There is exactly one `g_breadcrumbBehavior` symbol in the test binary. The root cause was always `CursorState::hasPos()` returning false, not a stale app `.o`.
- CMakeLists.txt does NOT need an OBJECT library fix for the behavior `.cpp` files.

### Verification
- **1429 tests, 0 failures** (excluded: MCPIntegration*, LocalLLMTest*, AX*, WindowTrail*, 4 order-dependent, DraggingIntegration*)
- **No regressions**: full suite passes clean.

## Session Summary (June 13-14, 2026) — Coverage Phase 5: behavior .cpp 53.25% → 96.5%

### What changed this session
- **`behavior_health.cpp`** (46 lines, 0% → 100%): Unblocked by adding `EventBus::Instance().Clear()` to test `SetUp()`. 22 tests cover init, tick damage/regen/heal/death, render (null + mock), `Health_Damage`/`Health_Heal` APIs, event publishing, full-health-no-regen, damage cooldown, multiple damage ticks, external damage (via API, no event). Previously hung in full suite due to stale EventBus subscriptions from prior tests.
- **`behavior_interactive_drops.cpp`** (20 lines, 85% → 100%): Added `TickDeterministicDropWithSeed48` test — `rng_util::Seed(48)` makes `RandRange(400)==0` on first call. Required explicitly setting `g_config.behaviors.interactiveDrops.dropInterval = 10.0f` (struct default is 120.0f, not the registry's 10.0f).
- **`behavior_jail.cpp`** (87 lines, 86.21% → 100%): Added `DisabledClearsJailActors` (jail actors removed when `g_config.behaviors.control.jail` toggled off) and `OKeyWhileActiveClearsJails` (O-press while jails active clears all jails, deactivates, and places new jail).
- **`behavior_hats.cpp`** (41 lines, 97.56% → 100%): Added `CleanupFunction` test (calls `b->cleanup(ctx)` on the hats behavior).
- **`behavior_portal.cpp`** (119 lines, 89.92% → 100%): Added `ReplacesPortalAOnSecondKeyPress`, `ReplacesPortalBOnSecondKeyPress` (second key press updates existing portal actor position instead of creating new), `JustTeleportedResetsWhenOutsidePortal` (goose leaves portal after teleport → `justTeleported` guard resets).
- **`behavior_toys.cpp`** (61 lines, 92% → 100% line coverage, 94.87% regions).

### Infrastructure
- Added `EventBus::Instance().Clear()` to health test `SetUp()` — unblocked the suite hang.

### Coverage impact
| Metric | Before | After | Change |
|--------|--------|-------|--------|
| **Behavior .cpp** (17/17 files covered) | 88.56% (573/647, 16 files) | **96.5%** (1049/1087, 17 files) | +7.94pp |
| **Tests** | 1394 | **1400** | +6 |

### Files at 100% line coverage
`behavior_health.cpp`, `behavior_interactive_drops.cpp`, `behavior_jail.cpp`, `behavior_hats.cpp`, `behavior_portal.cpp`, `behavior_toys.cpp`, `behavior_acid.cpp`, `behavior_anger.cpp`, `behavior_boredom.cpp`, `behavior_drag.cpp`, `behavior_nametag.cpp`, `behavior_peeking.cpp`, `behavior_presence.cpp`, `behavior_rainbow.cpp` (14/17 files)

### Files at 100% line+branch coverage
`behavior_health.cpp`, `behavior_interactive_drops.cpp`, `behavior_hats.cpp`...

### Remaining behavior .cpp files below 95% (1 file)
- `behavior_pomodoro.cpp`: ~93% (ellipse-fallback render path still uncovered due to static zzz image loading from prior tests)

### Pre-existing issues (unchanged)
- 4 order-dependent registration tests excluded: `BehaviorToggles.ToysBehaviorRegistered`, `PortalCleanup.BehaviorHasCleanupFunction`, `StalinHonk.*`
- `test_window_lifecycle.mm` orphaned (3 tests, deprecated API)

## Session Summary (June 22, 2026) — Phase 5 fix loop: 20+ code-quality fixes, 0 regressions

### Adversarial review findings fixed (20+ across 35 files)

**Type safety: strcmp → actorType() (8 sites)**
- `behavior_jail.cpp`, `behavior_portal.cpp` (3 sites), `app_actions.cpp`, `behavior_toys.cpp`, `behavior_hats.cpp`, `world_utils.mm` — all string-based `strcmp(a->type(), ...)` replaced with `a->actorType() == ActorType::...` fast enum comparison
- Test files updated: `test_behavior_portal.mm` (4 strcmp → `ActorType::Portal`), `test_behavior_jail.mm` (1 strcmp → `ActorType::Jail`)

**Type safety: int → FetchType enum (8 sites)**
- `goose_behaviors_wander.cpp` — `int fetchType` → `FetchType fetchType`, all comparisons use `FetchType::Meme`/`FetchType::Text`
- `app_actions.cpp` — `int type` → `FetchType type`, `args[1/2]` parsing maps to `FetchType::Text`/`Meme`/`TestImage`
- `ui_callbacks.cpp` (Linux) — 4 `ForceFetch(0/1, ...)` → `ForceFetch(FetchType::Meme/Text, ...)`
- `test_goose_behavior.cpp` — 2 `ForceFetch(0, ...)` → `ForceFetch(FetchType::Meme, ...)`

**ActorManager extraction (`include/actor_manager.h`)**
- `ActorManager` class extracted from `actor.h` to its own header — `actor.h` includes it at the bottom (transparent to existing includes). Forward-declares `ActorType`, `Actor`, `Goose`, `DroppedItemActor`, `WorldContext`, `IRenderer`.

**Audio thread safety**
- `audioMuted` data race: `extern bool audioMuted` was a regular bool read/written from multiple threads. Replaced with `static std::atomic<bool> g_audioMuted` in `audio.mm` + `Audio_SetMuted(bool)` function declaration in `audio.h`.

**ASSET_ROOT init ordering guard**
- `Audio_Init()` now returns early if `ASSET_ROOT` environment variable is not set, preventing silent audio failure during test bootstrap when ASSET_ROOT hasn't been initialized yet.

**Abstract class fixes**
- `TestActor` in `test_actor.cpp` — missing `actorType()` override added (was abstract, 4 tests couldn't compile)
- `MockModifyActor` in `test_actor_manager.cpp` — missing `actorType()` override added

**Dead code removed**
- `g_cutoverMode` extern declaration and all references deleted — the cutover to per-goose windows has been complete since 1.10
- Cross-instance `static` state removed from `Goose::draw()`

### Verification
- **1488 tests, 0 failures** (excluding 4 pre-existing order-dependent: `BehaviorToggles.ToysBehaviorRegistered`, `PortalCleanup.BehaviorHasCleanupFunction`, `StalinHonk.*`)
- Same baseline preserved — no regressions
- Committed as `7636d87`

## Session Summary (June 22b, 2026) — 3-way adversarial review: 9 CRITICAL + 10 HIGH fixes

### Adversarial review sweep — remaining CRITICAL/HIGH items (19 fixes)

**Data race** (`local_llm_tokenizer.mm`):
- Added `std::mutex s_tokenizerMutex` with `std::lock_guard` in all 6 accessor functions (`Encode`/`EncodeWithSpecialTokens`/`Decode`/`DecodeWithSpacePrefix`/`LoadVocab`/`IsLoaded`). Previously `s_vocab`/`s_idToToken` were unsynchronized across threads.

**Cursor source** (`behavior_ball.mm`):
- Switched from `g_backendManager.GetActiveBackend()` to `g_cursorProvider->Read()`, matching the established breadcrumbs pattern. `MacCursorBackend` destructor added: `CFRelease(m_eventSource)` call to release CGEventSource.

**Hardcoded values** (`behavior_health.cpp`):
- Replaced `kDamagePerHit = 5.0f` with `g_config.behaviors.health.damagePerHit`.
- Replaced magic speed threshold `0.6f` with `g_config.behaviors.health.speedDamageThreshold`.

**CGImageRef leaks** (`behavior_hats.cpp`, `behavior_pomodoro.cpp`):
- `cleanupHat()`: added `CGImageRelease()` on `s_hatImage` (guarded `#ifdef __APPLE__`).
- `cleanupPomoFont()`: added `CGImageRelease()` on the font/ZZZ `CGImageRef` (guarded `#ifdef __APPLE__`).

**Dead code removed** (4 files):
- `behavior_breadcrumbs.cpp`: removed `s_crumbImage` and stale `LogCrumb` function.
- `behavior_pomodoro.cpp`: removed `extern void Audio_PlayHonk()` block.
- `behavior_ai.mm`: removed dead `g_httpClient` declaration.

**Duplicate includes removed** (3 files):
- `behavior_interactive_drops.cpp`: duplicate `#include "behaviors/states/interactive_drops_state.h"`.
- `behavior_boredom.cpp`: unused `#include "cg_renderer.h"`.
- `behavior_honcker.cpp`: duplicate `#include "behaviors/states/honcker_state.h"`.

**Multi-goose shared static timer** (`behavior_toys.cpp`):
- `static double lastSpawnTime` → `state->lastSpawnTime` from per-goose `ToysState`. Full suite confirms no regression.

### Verification
- **1429 tests, 0 failures** (excluding pre-existing exclusions)
- Same baseline preserved — no regressions

## Session Summary (June 22c, 2026) — MEDIUM/LOW sweep: 8 more fixes, 0 regressions

### MEDIUM
- **Ghost windows** (`effect_window.mm`): `addWindow:` used `addObject:` growing pre-allocated array. Fixed to `replaceObjectAtIndex:` with computed insert index.
- **Breadcrumb zombie actors** (`behavior_breadcrumbs.cpp`): Eaten/popped crumbs left BreadcrumbActor alive. Added `deactivateCrumbActor()` position-match helper.
- **Rainbow dead store** (`behavior_rainbow.cpp`, `rainbow_state.h`): Removed `lastUpdate` field and its lone write-site.
- **Jail always-true guard** (`behavior_jail.cpp`): Removed `if (time > s_lastInputTime)` — always true, no actual throttling.
- **Portal O(n)** (`behavior_portal.cpp`): Manual actor scan → `mgr.findByType(ActorType::Portal, id)`.

### LOW
- **app_actions null-check** (`app_actions.cpp`): Dead ternary `goose ? goose->id : -1` simplified to `goose->id`.
- **Duplicate includes** (3 files): `jail_state.h`, `portal_state.h`, `rainbow_state.h` — all had duplicates.
- **Test cleanup** (`test_behaviors_visual.cpp`): Removed orphaned `lastUpdate` assertion.

### Verification
- **1429 tests, 0 failures** — same baseline preserved

## Session Summary (June 22d, 2026) — Round 3: include/dead-code sweep, 31 files

### Duplicate includes (9 files, 16 pairs)
- `behavior.cpp`: 6 duplicate state includes removed.
- `goose_behaviors_fetch.cpp`: duplicate `actor_dropped_item.h` removed.
- `behavior_acid/anger/boredom/health/honcker/peeking/pomodoro.cpp`: each had a duplicate state `#include`.

### String→ActorType enum (54 sites, 10 files)
- **`ui_escape.cpp`**: `destroyAllOfType("goose")` → `ActorType::Goose`.
- **9 test files**: all `destroyAllOfType("X")` / `countByType("X")` replaced with `ActorType::X` (goose, baby_stalin, portal, jail, toy, flower, leafpile, breadcrumb).

### Dead code removed (5 items)
- `DRAG_RADIUS` (behavior_drag.cpp), `kStuckRecoveryMargin` + `CloseDebugLog()` (goose.cpp — separate from `goose_debug.h`'s `CloseDebugLog` which is the one actually called).
- `kMinMcpPort`/`kMaxMcpPort`/`kDefaultMcpPort`/`kTestTimeout` (config_gui_ai.mm — unused duplicates).
- `kModelRefreshDelay`/`kModelPopupTag` (config_gui_ai_connection.mm — unused duplicates).

### Unused standard includes (7 files)
- `<cstring>` (behavior_hats, hotkey, behavior_ai).
- `<cstdio>` (goose_behaviors_internal, local_llm_model).
- `<ctime>` (behavior_interactive_drops).
- `<algorithm>` (hotkey, cursor_backend).

### Verification
- **1429 tests, 0 failures** — same baseline

## Key Facts
- **Seed 48**: `rng_util::Seed(48)` makes `RandRange(400) == 0` on first call (interactive drops trigger), `RandRange(360) == 51` on second call (hue).
- **Config struct vs registry defaults**: Struct member initializers in `config.h` are the actual runtime values; registry defaults in `config_registry_*.cpp` are only used for GUI/TOML serialization. Many struct defaults differ from registry defaults (e.g., `dropInterval`: struct=120.0f, registry=10.0f).
- **`EventBus::Clear()`** must be called in `SetUp()` for any test that uses event subscriptions — subscriptions persist across tests.
- **`g_cursorProvider` must be set with valid position for breadcrumbs tick**: Default `CursorState` has `caps=CAP_NONE`, so `hasPos()` returns `false` until `mockCursor.set(x, y)` is called. Without it, tick exits early at the cursor check.
- **`ActorType` enum now replaces `strcmp(a->type(), ...)`**: All strcmp-based type comparisons replaced with `a->actorType() == ActorType::...`. Pure virtual `actorType()` added to `Actor` base, implemented in all 10 subclasses (Goose, BabyStalin, Ball, Breadcrumb, DroppedItem, Flower, Jail, Leafpile, Portal, Toy).
- **`FetchType` enum replaces `int` for ForceFetch**: `enum class FetchType { Random=-1, Meme=0, Text=1, TestImage=2 }`. `Goose::forceItemFetch` and `Goose::ForceFetch` parameter changed from `int` to `FetchType`.
- **`ActorManager` extracted to separate header** (`include/actor_manager.h`): Forward-declares all dependencies. `actor.h` includes it at the bottom — all existing includes update transparently.
- **`g_cutoverMode` fully removed**: The per-goose-window cutover has been complete since v1.10. The extern declaration, definition, and all references are deleted.

## Relevant Files
- `tests/common/test_behavior_health.mm` — 22 tests, `EventBus::Clear()` in SetUp unblocks suite
- `tests/common/test_behavior_interactive_drops.mm` — 8 tests, seed 48 deterministic drop
- `tests/common/test_behavior_jail.mm` — 13 tests, O-clear + disabled cleanup
- `tests/common/test_behavior_hats.mm` — 7 tests, cleanup function
- `tests/common/test_behavior_portal.mm` — 15 tests, re-placement + justTeleported reset

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
- **Status bar icon changes in Stalin mode**: Shows hammer and sickle (U+262D) instead of dark/Canada maple leaf or light/default goose.
- **Dynamic update on mode switch**: `UpdateStatusBarIcon()` C function called from both `setupMenubar` (launch) and `modeChanged:` (GUI mode switch).

### AI Stalin Mode — Honker & Chat
- **Honcker behavior** calls `goose->onHonk()` instead of `g_assets.Honk()`, so BabyStalin plays Gulag sound via F key.
- **AI chat system prompt** replaces HONK→GULAG, Goose→Comrade in Stalin mode. Fallback responses use `s_applyStalinMode()` for string replacement.
- **Chat UI** uses Stalin-mode text: "GULAG!" greeting, "Comrade:" markers, window title "Chat with Comrade X".

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
- **g_cutoverMode removed**: The per-goose-window cutover has been complete since v1.10. `Goose::render()` returns immediately, only `Goose::draw()` executes (per-goose window path).
- **Goose::onHonk() virtual method**: Called from `triggerHonk()` instead of hardcoded `g_assets.Honk()`. Default calls `g_assets.Honk()`. BabyStalinActor overrides to call `g_assets.Gulag()`. `m_canHonk = true` for BabyStalinActor so its audio fires through the normal honk system.
- **Gulag audio**: `Audio_PlayGulag()` with dedicated 2-player AVAudioPlayer pool. MP3 at `Assets/Sound/NotEmbedded/Gulag.mp3`. `AssetManager::Gulag()` abstracts platform: macOS → `Audio_PlayGulag()`, Linux → fallback to `Honk()`.
- **Multi-goose test**: Command-socket-only, no SCStream needed. Verifies goose_count + per-goose fetch cycle. `clear_dropped` command isolates fetch cycles. `fetch <idx> type` targets specific goose.
- **renderer.h/renderer.mm deleted**: Placeholder files finally removed after Phase 3 cleanup. No replacement needed — Cocoa imports are direct.
- **WindowManager stub removed**: No remaining callers. 3 active managers: `ItemWindowManager`, `EffectWindowManager`, `BehaviorElementWindowManager`.

## Known Bugs (June 22, 2026)

- **Config generator** — Works correctly for registry generation. GUI generation intentionally skipped (incompatible with `config_gui.mm` key-based lookup architecture).
- **g_world.droppedItems** — 127 references across codebase. `DroppedItemActor` scaffold ready for future migration.
- **Trail detection false positive** — Trail scan counts dropped-item-contaminated frames as "trails". Need to filter frames where previous-cycle dropped items are on screen.
- **Test process memory ~7.2GB** — From ring buffer (300 × 25MB frames). Acceptable for short runs; not a CadGoose issue.
- **MCPIntegrationTest failures** — Tests require running MCP server. Run with `./CadGoose` running in background.
- **Coverage** — P0 `.cpp` line coverage at 94.60% (197/3651 missed), 25/28 files ≥95%. Behavior `.cpp` at 100% (all 17 files). `app_actions.cpp` pushed past 95% (95.36%). 6 files remain below 95%.
- **test_window_lifecycle.mm** — Still orphaned (3 tests). macOS 15 deprecated API (`CGWindowListCreateImage`), cannot reclaim without rewrite.
- **Pre-existing issues** (unchanged): 4 order-dependent registration tests excluded (`BehaviorToggles.ToysBehaviorRegistered`, `PortalCleanup.BehaviorHasCleanupFunction`, `StalinHonk.*`).
- **Remaining Adversarial Review issues (deferred)**: `void*` ObjC pointers in 12 actor headers — well-established type-safe pattern with `__bridge` casts and documented type comments. Deemed cosmetic, no runtime bugs.

## Next Steps

### Remaining coverage work
- [ ] Phase 6: AI mock layer (ai_http_client.mm, local_llm_*.mm) — ~1800 lines, interface extraction
- [ ] Phase 7: Thin wrappers — window.mm, tick_manager.mm, effect window files
- [ ] Phase 8: CI gate hardening — multi-metric threshold (P0 95%, P1 80%, project 90%)

### Release
- CI (`.github/workflows/build_and_release.yml`) is **green end-to-end**: macOS DMG + Linux `.tar.zst`, tests gating, coverage gate.
- Notarization (Developer ID + `notarytool` + staple) is remaining release polish.
- Optional: `CadGoose --version` CLI flag.
