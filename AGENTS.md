# CadGoose Agent Guide

Instructions for agents working in this repo. **The backlog lives in
`ROADMAP.md` and nowhere else**, and this file carries none. The session log
that used to fill this file was pruned on 2026-10-12; the per-session detail is
in `docs/CHANGELOG.md`, and the old text is at `git show a3353d2:AGENTS.md`.

**This repo is a maintenance line** (ROADMAP.md D1, owner ruling 2026-10-12): no
new features here; they go to a separate fork.

## Documentation rules

- **ROADMAP.md**: forward-looking only, with pending work and never completed
  items. Every item states how we will know it is done.
- **docs/CHANGELOG.md**: completed work only, by date.
- **AGENTS.md**: standing instructions and durable facts. It is not a session
  diary; do not append session summaries here.
- When work completes, move it from ROADMAP.md to CHANGELOG.md in the same
  commit as the change.
- Remove a document once its content is superseded, and leave one `git show`
  pointer to it.
- This repo is public. Do not name or depend on the author's private
  repositories or machine-local paths (owner ruling, `ae819a7`).

## Build & run

```bash
./build.sh              # macOS Release: checks/installs Homebrew deps, self-heals a stale cache
./build_release.sh      # build-release/: Release + CG_DISABLE_DEBUG_LOG=ON (the CI/shipping build)
./build_debug.sh        # build-debug/: Debug, logging on (ASan via EXTRA_CMAKE_ARGS=-DENABLE_ASAN=ON)
./build_linux.sh        # Linux Release via Docker
./run.sh                # build, then run build/CadGoose
./build/CadGoose [--debug] [--foreground]
./build/CadGooseTests   # full suite (window/OCR tests need a display)
( cd build && ctest --output-on-failure )   # the CI test gate
./scripts/create_bundle.sh --clean          # self-contained build/CadGoose.app
tools/gate.sh [--staged|--full]             # house gates; --full = the CI copy (build + tests)
```

- **Platforms**: macOS arm64, Linux x86_64 and Linux aarch64 only;
  `cmake/PlatformPolicy.cmake` fails the configure for anything else.
- `build/` is a **Ninja** dir. Rebuild with `cmake --build build --target CadGoose`.
  `make CadGoose` there is a silent no-op that leaves a stale binary.
- Plain `./build/CadGoose` daemonizes (`posix_spawn`), so `$!` is a dead PID.
  Use `--foreground` when you need to attach (profiling, debugging).
- **Dependencies**: `build.sh` installs `cmake ninja googletest` via
  Homebrew (`SKIP_DEPS=1` skips this). toml11 and mimalloc come in through CMake
  `FetchContent` as release tarballs pinned by `URL_HASH` (CMakeLists.txt;
  `tools/check_fetchcontent.py` refuses a git clone); there is no submodule.
  mimalloc is linked as one object file, so the bundle carries no dylib.
- **Minimum macOS**: 26, written once in `cmake/MacOSFloor.cmake` (G0.14).
  `tools/check_macos_floor.py` holds README, the cask, Info.plist and the
  `minos` of every Mach-O in the bundle to it; `scripts/create_bundle.sh` runs it.
- `CG_DISABLE_DEBUG_LOG=ON` compiles debug logging out of the **app target
  only**. The test target keeps it, so `test_log.cpp` still runs.
- **Logs/crashes**: `<ConfigDir>/logs/`. `ConfigDirPath()` prefers
  `./config/config.toml` when it exists, else
  `~/Library/Application Support/CadGoose`. Crash backtraces go to
  `crash-<ts>.log`; headless stderr goes to `session-<ts>.log`.
- **Tests in CI** run with the `--gtest_filter` defined once in
  `tests/gtest_ci_filter.txt`; `scripts/run_tests_ci.sh` and `cmake/Tests.cmake`
  consume that file, and `tools/check_gtest_filter_parity.py` fails when a call
  site stops doing so (no second value copy). Window-server tests carry the
  `requires_display` label and are excluded headless; OCR tests skip without
  `tesseract`.
- Concurrent `CadGooseTests` runs are safe and gated: each process gets its
  own config sandbox (`/tmp/cadgoose-test-config-XXXXXX`, removed when green),
  the MCP HTTP tests bind port 0 (`MCP_HTTPPort()` reads it back) and the
  internal-server tests a per-process socket (`MCP_SetInternalSocketPath`).
  `scripts/run_tests_concurrent.sh` runs two copies at once in CI and
  pre-push; `tools/check_test_endpoints.py` refuses a fixed port or socket
  path under `tests/`.

## Release

- `.github/workflows/build_and_release.yml` builds the macOS DMG (on `macos-26`,
  for the FoundationModels SDK) and the Linux tarball, version-stamps them,
  and attaches them on `release: published` or `workflow_dispatch`.
  `scripts/push_release.sh` drives it.
- **Release loop rule**: a release is done only when the remote Actions run is
  green end to end and the DMG is attached. On any failure, fix it locally,
  push, and recreate the tag until it is.
- **Homebrew rule**: once the DMG is attached, the workflow updates
  `Casks/cadgoose.rb` in the tap with the version and SHA-256. Confirm the cask
  version matches the tag; it is a manual step if the tap token is absent.

## Durable facts (verified 2026-10-12)

- **ActorManager owns actors.** Tests must not `delete` an actor after `add()`.
  Deactivate it and let `cleanup()` delete it, or the heap corrupts later
  (SIGBUS mid-suite).
- **Shared state is restored after every test** by `GlobalStateRestorer`
  (`tests/global_state_reset.h`): actors, behavior states, event-bus
  subscriptions, the behavior registry, `g_config`, `g_world`, `g_time`, the
  cursor provider, module file-statics (the `*_ResetForTest()` seams in
  `include/test_seams.h`, compiled into `CadGooseTests` only), the test
  doubles, audio (the suite is muted, and the test binary has no route to the
  speakers at all: a clip starts in a silent sink that `Audio_StartedForTest`
  reads, so playback is asserted unmuted without a sound), and the hour
  (`Nighttime_SetForTest(false)`: the suite runs in daytime whatever the
  clock says). New process-wide state gets its reset there; the shuffle step
  in `.gatesrc` goes red when one is missing. Coverage that came from state
  one test leaked into the next is gone with it (G0.16): a path counts as
  covered only through a test that sets up its own precondition.
  A test behavior's callbacks may capture the test's stack, so the registry
  is restored to the app's own snapshot (`RestoreOriginal()`); there is no
  "restore everything ever registered".
- **Cursor mocks**: a default `CursorState` has `caps=CAP_NONE`, so `hasPos()`
  is false until `mockCursor.set(x, y)`, and cursor-driven ticks exit early.
- **Config defaults**: the struct initializers (`include/config_sections.h`)
  are the only defaults. The registry's numeric arguments are min/max/step.
- **Seed 48**: `rng_util::Seed(48)` makes `RandRange(400) == 0` on the first
  call and `RandRange(360) == 51` on the second.
- **Effect registrations** in `effect_reg_*.mm` are static-initialized, so any
  binary that links them can exercise them through `EffectGetRegistrations()`.
- **Tests never mutate the process environment or cwd** (no
  `setenv`/`unsetenv`/`chdir`/`current_path(path)`; enforced by
  `tools/check_test_process_state.py`). Every thread reads them, and a test
  that did once leaked a config into the repo root (G0.12). Use
  `ResolveConfigDir(ConfigDirSources)` or `ScopedConfigDirOverride`, or split
  a pure function of the value out (`Log_LevelFor`). Every test is also
  checked for writes into the cwd by `tests/cwd_leak_guard.h`, and the binary
  exits 1 naming the offender.
- **Headless tests** set `CADGOOSE_HEADLESS_TEST=1`, which makes the actor
  `.mm` files skip their `NSWindow` allocation. Audio is gated separately by
  `AudioSuppressed()` (enabled/muted), and under `CG_TEST_SEAMS` never
  reaches the device.
- **The pre-push gate runs every gate CI runs**, the coverage gate included
  (`GOH_CI_STEPS` in `.gatesrc`; goh's `checks/check_ci_local_parity.py` refuses a
  CI gate script no local step runs). A new CI gate goes into both.
- **MCP config handlers** run on the HTTP server thread and must not hop to the
  main thread. Hotkeys are read through the lock-free cache
  (`include/hotkey_cache.h`); the strings in `g_config` are read only under
  `Hotkey_Name()`.
- **Drawing colours** go through `CGCtx_SetFillColor`/`SetStrokeColor`
  (`include/cg_color_cache.h`), not `CGContextSetRGB*Color`. Per-frame
  `CGColor` churn was the top render hotspot.
- **Emoji**: only the product-identity glyphs in `.gatesrc` `GOH_ALLOW` (the
  goose, the maple leaf, the hammer and sickle). Status text uses the Kare set.
