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
mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(sysctl -n hw.logicalcpu)
./build/CadGoose [--debug]
./build/CadGooseTests
# Multi-goose regression test (run from any terminal — no SCStream needed):
./build/multi_goose_test
# Trail detection test (run from Ghostty — auto-hides windows):
./tools/profiling/run_trail_test.sh
# Fetch visibility soak test (run from Ghostty — repeats fetch cycles for up to 10m):
./tools/profiling/run_soak_test.sh
# Or directly from build directory (with CadGoose running):
./build/trail_detection_test
./build/soak_fetch_test
```

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
- **Integration.Goose_ReturningItem and Goose_DropItem fail from build dir** — AssetManager (`assets.mm`) loads images from `./Assets/` relative to CWD. Tests run from `build/` which has no `Assets/` directory. Fix: copy `Assets/` to build dir, or use CMake `WORKING_DIRECTORY` property.
- **MCPIntegrationTest failures** — Tests require running MCP server. Run with `./CadGoose` running in background.

## Next Steps

### Remaining
- Run trail test: verify 6/6 cycles visible in per-goose window architecture.
- Run soak test after full-screen overlay removal: verify memory drops from ~985MB to ~150-200MB.
- Run the AX accessibility tests (checking per-goose windows exist).

### Release
- CI release infrastructure exists (`.github/workflows/build_and_release.yml`) but untested. Creates `.dmg` (macOS) and `.tar.zst` (Linux).
- Verify CI pipeline works end-to-end: push a tag, check release artifacts.
- Consider adding: `make dist` CMake target, `CadGoose --version` flag, version header.

### Baby Stalin Character System (deferred)
- See `docs/PLAN.md` for full design: `CharacterSkin` interface, `SkinRegistry`, `BabyStalinSkin` with programmatic drawing.
- Not blocking any current work.
- ~~Gulag audio~~ ✅ (wired to replace honk sound for BabyStalinActor)
- ~~Honk F key~~ ✅ (Honcker dispatches through `onHonk()` virtual)
- ~~AI Chat~~ ✅ (Stalin-mode system prompt replacement + fallback string replacement)
