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

## Session Summary (May 30, 2026) — Release hardening: assets, AI chat, CI gate

- **EXC_BAD_ACCESS crash fix** (`world_utils.mm`, `actor_dropped_item.h`, `actor.mm`): Fixed a null-pointer dereference in `World_CleanupExpired` and `ItemHitTest` caused by defunct items that had their `data` deleted and set to `nullptr` when closed but remained in memory. Added comprehensive `actor->data()` null checks. Also updated `DroppedItemActor::isAlive()` to return `false` if `m_item.data == nullptr`, allowing `ActorManager::cleanup()` to instantly reap the actor, fixing both the crash and a persistent memory leak. Resolved a use-after-free race condition crash in headless CI and unit tests by updating `Actor::closeWindowOnMainThread` inside `actor.mm` to execute synchronously if already running on the main thread, ensuring the window is closed before the actor is deallocated.
- **Test crash logger & symbolication** (`CMakeLists.txt`, `test_main.cpp`): Linked `crash_logger.mm` to the `CadGooseTests` test binary and initialized it via `CrashLogger_Init()` in `test_main.cpp`, ensuring all test crashes produce detailed, symbolicated C++ backtraces.
- **CI console logging visibility** (`crash_logger.mm`): Prevented `stderr` redirection when running inside GitHub Actions, ensuring that test timings, logs, and crash logs print directly to the CI workflow terminal instead of being hidden in local files.
- **Headless test window stubbing** (`test_main.cpp`, `actor_dropped_item.mm`): Established the `CADGOOSE_HEADLESS_TEST` flag inside the test suite, allowing `DroppedItemActor::initWindow()` to completely bypass Cocoa `NSWindow` allocation. This permanently resolves AppKit WindowServer and graphics connection segfaults on headless CI virtual machines, making the remote build suite fully robust and green.
- **GooseRender NAN drawing crash fix** (`test_goose_rendering.mm`): Fixed a crash in the rendering tests where uninitialized physics configuration values caused division-by-zero or math errors during vector normalization, resulting in `NAN` coordinates that crashed CoreGraphics drawing in headless runners. Explicitly initialized `isoScaleX` and `isoScaleY` configs in rendering test setups.
- **Deprecation warning fix** (`behavior_element_window.mm`): Removed the deprecated `setOneShot:` call from window initialization, restoring a clean, zero-warning compilation on newer macOS SDKs.
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

- **764 tests, 104 suites** — 744 pass, 2 pre-existing failures (`Integration.Goose_ReturningItem`, `Integration.Goose_DropItem` — AssetManager needs Assets/ in build dir), 6 MCPIntegrationTest failures (need running MCP server), 32 skipped (31 AX + 1 drag), 1 LocalLLMTest skipped (no model)
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

## Known Bugs (May 26, 2026)

- **Config generator** — Works correctly for registry generation. GUI generation intentionally skipped (incompatible with `config_gui.mm` key-based lookup architecture).
- **g_world.droppedItems** — 127 references across codebase. `DroppedItemActor` scaffold ready for future migration.
- **Stale pointer risk in item_window.mm** — Mitigated by `IsItemValid()` check before every use + `std::list` pointer stability guarantees.
- **Trail detection false positive** — Trail scan counts dropped-item-contaminated frames as "trails". Need to filter frames where previous-cycle dropped items are on screen.
- **Test process memory ~7.2GB** — From ring buffer (300 × 25MB frames). Acceptable for short runs; not a CadGoose issue.
- ~~**Integration.Goose_ReturningItem / Goose_DropItem fail from build dir**~~ ✅ Fixed — test harness now calls `g_assets.Init()` and `ctest` sets `WORKING_DIRECTORY` to the source tree so `Assets/` resolves.
- **MCPIntegrationTest failures** — Tests require running MCP server. Run with `./CadGoose` running in background.

## Next Steps

### Remaining
- Run trail test: verify 6/6 cycles visible in per-goose window architecture.
- Run soak test after full-screen overlay removal: verify memory drops from ~985MB to ~150-200MB.
- Run the AX accessibility tests (checking per-goose windows exist).

### Release
- CI (`.github/workflows/build_and_release.yml`) is **green end-to-end**: macOS DMG (on `macos-26`, FoundationModels SDK verified) + Linux `.tar.zst`, version-stamped, tests gating. Auto-attaches on `release: published`; use `workflow_dispatch` (`release_tag`) to attach to an existing tag.
- The released DMG is **ad-hoc signed, not notarized** — users need `xattr -dr com.apple.quarantine` (documented in README). Notarization (Developer ID + `notarytool` + staple) is the remaining release polish.
- Optional nice-to-haves: `CadGoose --version` CLI flag (the bundle already carries the version via `CFBundleShortVersionString`).

### Baby Stalin Character System (deferred)
- See `docs/PLAN.md` for full design: `CharacterSkin` interface, `SkinRegistry`, `BabyStalinSkin` with programmatic drawing.
- Not blocking any current work.
- ~~Gulag audio~~ ✅ (wired to replace honk sound for BabyStalinActor)
- ~~Honk F key~~ ✅ (Honcker dispatches through `onHonk()` virtual)
- ~~AI Chat~~ ✅ (Stalin-mode system prompt replacement + fallback string replacement)
