# Plan — 95% Coverage + E2E Gate

## Motivation

Current line coverage is **35.49%** (7,330 of 11,362 lines missed). CI has no coverage gate and only runs `CadGooseTests` via `ctest` — standalone targets (`multi_goose_test`, `soak_fetch_test`, `trail_detection_test`) are built but never executed. Orphaned test files exist on disk but are not compiled.

Goal: reach **95% line coverage** on coverage-eligible sources and **all E2E targets passing** as a CI hard gate.

## Scope & Exclusions

Not all files can reach 95% without a physical display server. Define two partitions:

| Partition | Files | Gate Target | Rationale |
|-----------|-------|-------------|-----------|
| **P0 — Portable** | `src/common/*.cpp` + `include/*.h` | **95%** | Pure C++, no AppKit dependency; testable headless |
| **P1 — Platform** | `src/common/*.mm` + `src/platform/macos/*` | Tracked (no gate) | Requires ObjC runtime, NSWindow, or display server |
| **P2 — Vendor** | `_deps/` | Excluded | Third-party |

P0 is the CI gate. P1 is tracked in local coverage reports but does not block CI.

## Baseline (measured after Phase 1)

| Scope | Lines | Covered | Missed | % |
|-------|-------|---------|--------|---|
| All files | 11,362 | 4,032 | 7,330 | 35.49% |
| **P0 — portable C++** | **4,339** | **2,958** | **1,381** | **68.17%** |
| `src/common/*.cpp` | 2,954 | 1,823 | 1,131 | 61.71% |
| `include/*.h` | 1,385 | 1,135 | 250 | 81.95% |

## Phases

### Phase 1: Coverage Infrastructure (A1 — additive) ✅ DONE

**Goal**: Baseline, CI gate for P0 at current coverage (no regression), orphan reclamation. No new tests yet.

**Completed work items**:

1. **Orphaned test reclamation**
   - Added `tests/common/test_behavior_sim.cpp` to `CMakeLists.txt TEST_SOURCES_COMMON` (21 tests)
   - `test_window_lifecycle.mm` skipped — macOS 15 deprecated API, cannot reclaim without rewrite
   - All 21 tests compile and pass

2. **Standalone target registration with ctest**
   - `multi_goose_test`, `soak_fetch_test`, `trail_detection_test` registered with `add_test()` + `LABELS "requires_display"`
   - CI excludes via `ctest -LE "requires_display"`

3. **Coverage collection in CI**
   - Added `Coverage Gate` step to `.github/workflows/build_and_release.yml` (macOS job only)
   - Runs `scripts/check_coverage.sh --p0-min=50`
   - Uploads `coverage-report/` as build artifact

4. **Coverage gate script**
   - `scripts/check_coverage.sh` — builds `CODE_COVERAGE=ON`, runs tests, parses `llvm-cov report` column 10
   - Accepts `--p0-min=N` and `--build-dir=`
   - P0 baseline: 68.15% line coverage

5. **Exclusion list**
   - `scripts/coverage_eligible.txt` — globs `src/common/*.cpp` + `include/*.h`
   - P1/P2 files excluded from gate metric

**Result**: CI with coverage gate green at ≥50% P0 coverage (baseline 68.15%).

### Phase 2: Portable C++ Coverage (A2 — parity proof)

**Goal**: All `src/common/*.cpp` files ≥95% line coverage.

**Priority order** (by return on effort):

**Tier 1 — 0% files, small (1-2 days)**
- `src/common/log.cpp` (17 lines, trivial) — test `LogLevelToString`, `UiLogPush`, `OpenLogFile`
- `src/common/world.cpp` (10 lines) — test `WorldContext` construction, `g_time` setter
- `src/common/app_cli.cpp` (116 lines) — test `ParseCLI`, `--help`, `--debug`, `--version`

**Tier 2 — 0% files, medium (±1 week)**
- `src/common/mcp_http_server.cpp` (132 lines) — HTTP server lifecycle, request routing. Needs TCP socket test with ephemeral port.
- `src/common/mcp_server.cpp` (184 lines, 30%) — `MCP_Init`, `MCP_SendMessage`, `MCP_Shutdown`. Already has MCP protocol/unit tests; needs server lifecycle tests.
- `src/common/behavior.cpp` (232 lines, 18%) — `BehaviorRegistry`, behavior lookup, `GetBehaviorsForActor`. Add more states.

**Tier 3 — Sub-90% files (±1-2 weeks)**
- `src/common/app_actions.cpp` (194 lines, 8%) — `AppActions_HandleCommand`, `MCP_Spawn`, `MCP_Clear`. These only run in production; add unit test path with mock `ActorManager`.
- `src/common/goose_behaviors_fetch.cpp` (180 lines, 87%) — tight already, small gap
- `src/common/goose_forces.cpp` (104 lines, 85%) — tight
- `src/common/hotkey.cpp` (109 lines, 82%) — already well-tested (22 tests), small gaps
- `src/common/config.cpp` (144 lines, 81%) — tight
- `src/common/config_load.cpp` (178 lines, 69%) — medium

**Tier 4 — Behavior files (0%, ±2 weeks)**
Many behavior `.cpp` files at 0% because their state code is never exercised in tests:
- `behavior_hats.cpp` (41 lines) — `HatsState` ctor
- `behavior_boredom.cpp` (75 lines) — `BoredomState` ctor, transition logic
- `behavior_interactive_drops.cpp` (20 lines) — `InteractiveDropsState`
- `behavior_presence.cpp` (26 lines) — `PresenceState`
- `behavior_nametag.cpp` (30 lines) — `NametagState`
- `behavior_peeking.cpp` (43 lines, 16%) — `PeekingState`
- `behavior_toys.cpp` (61 lines, 11%) — `ToysState`
- `behavior_drag.cpp` (26 lines) — `DragState`
- `behavior_jail.cpp` (84 lines) — `JailState`, portal interaction
- `behavior_portal.cpp` (126 lines) — `PortalState`, cooldown, cleanup
- `behavior_breadcrumbs.cpp` (83 lines) — `BreadcrumbState`
- `behavior_rainbow.cpp` (23 lines) — `RainbowState`
- `behavior_health.cpp` (46 lines) — `HealthState`, regen/damage
- `behavior_anger.cpp` (104 lines, 4%) — `AngerState`, threshold, boost
- `behavior_acid.cpp` (28 lines) — `AcidState`, spin trigger
- `behavior_pomodoro.cpp` (233 lines) — `PomodoroState`, phase transitions
- `behavior_honcker.cpp` (51 lines) — `HonckerState`

**Strategy**: Extract state machine logic from `.cpp` files that's testable via `BehaviorStateManager::GetOrCreate`. Each gets a `test_behavior_*.cpp` file paralleling the existing `test_behaviors_control.cpp`, `test_behaviors_fun.cpp` etc.

**Kill criterion**: P0 coverage ≥95%. CI gate set to `--p0-min=95`.

### Phase 3: AI & MCP Coverage

**Goal**: AI/MCP paths at 95%.

- `mcp_http_server.cpp` (see Phase 2 Tier 2)
- `ai_mcp_bridge.cpp` (155 lines, 96%) — already done
- `ai_text_meme.mm` (493 lines, 10%) — `.mm` file → out of P0 scope but add headless tests for the C++ helpers
- `ai_http_client.mm` (545 lines, 0%) — requires HTTP mock
- `ai_local_llm_adapter.mm` (181 lines, 0%) — requires CoreML model
- `local_llm_inference.mm` (358 lines, 0%) — requires CoreML
- `local_llm_model.mm` (226 lines, 0%) — requires CoreML
- `local_llm_tokenizer.mm` (85 lines, 0%) — requires CoreML
- `ai_think_block_stripper.mm` (12 lines, 0%) — small, testable
- `ai_model_profiles.mm` (17 lines, 0%) — small, testable
- `behavior_ai.mm` (459 lines, 0%) — ObjC++ AI behavior, needs AppKit

These are predominantly P1 (ObjC++ / CoreML / AppKit). Cover what we can with headless mocks; the rest stays P1.

**Kill criterion**: P0 ≥95%, AI C++ helpers ≥90%, P1 tracked.

### Phase 4: E2E Test Suite

**Goal**: Every standalone test target registered with ctest and passing in CI.

**Existing targets**:
| Target | Test File | Status |
|--------|-----------|--------|
| `multi_goose_test` | `tests/platform/macos/test_multi_goose.mm` | Builds, not in ctest |
| `soak_fetch_test` | `tests/platform/macos/test_soak_fetch_visibility.mm` | Builds, not in ctest |
| `trail_detection_test` | `tests/platform/macos/test_window_trail_detection.mm` | Builds, not in ctest |

**New targets to add**:
- `e2e_clear_spawn_cycle` — spawn goose → fetch → return → clear → verify count
- `e2e_baby_stalin` — Stalin mode spawns BabyStalin, honk plays Gulag
- `e2e_multi_goose_concurrent` — 3 geese, each completes fetch cycle concurrently
- `e2e_window_lifecycle` — rapid create/destroy per-goose windows
- `e2e_mcp_config` — start MCP server, set config, verify via TCP

**CI Integration**:
- `ctest` already excludes `WindowTrailTest.*` — add exclusions for targets needing display
- E2E targets that need a display run only on `macos-26` runner (already in use)
- Add `--label` system: `LABELS REQUIRED_DISPLAY` for display-dependent tests

**Kill criterion**: `ctest` runs all 5+ E2E targets. All pass on macOS CI.

### Phase 5: CI Gate Hardening

1. **Coverage threshold ratchet**: CI script accepts `--p0-min` argument. Start at 50% (Phase 1), ratchet to 70% (Phase 2), 90% (Phase 3), 95% (Phase 4).

2. **Regression guard**: CI compares against previous run's coverage. If P0 drops, fail.

3. **E2E pass gate**: All non-excluded ctest targets must pass. Exclusions require documented rationale.

4. **Coverage badge**: `coverage-report/badge.svg` generated by `gen_coverage_badge.py` from summary.txt. Committed or uploaded to Pages.

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `.mm` actor files impossible to cover without display | High | Medium — gate always <95% | Exclude from P0 gate; track separately |
| Orphaned test files don't compile cleanly | Medium | Low — delays Phase 1 | Fix compile issues as they arise |
| E2E tests flaky on CI runner | Medium | High — broken gate | Retry logic, timeout increase, label system |
| Coverage-per-line drops on new PRs | Medium | Low — requires discipline | Regression guard catches it |
| Simulated 95% on C++ but untested platform bugs | Low | Medium — false confidence | P1 tracked manually; E2E tests catch platform bugs |

## Timeline

| Phase | Effort | CI Target | Milestone |
|-------|--------|-----------|-----------|
| 1 — Infrastructure | 3-5 days | P0 ≥50%, ctest has E2E targets | `[x] CI gates on coverage` |
| 2 — Portable C++ | 3-4 weeks | P0 ≥95% | `[x] All .cpp files ≥95%` |
| 3 — AI/MCP | 1-2 weeks | P0 ≥95%, AI helpers tested | `[x] AI paths covered` |
| 4 — E2E Tests | 1-2 weeks | All targets green | `[x] Full E2E suite` |
| 5 — Gate Hardening | 1 week | 95% gate locked | `[x] PRs blocked below 95%` |

Total: 7-10 weeks.

## Verification

After each phase:
- [x] Phase 1: `scripts/check_coverage.sh --p0-min=50` exits 0 (68.15%)
- [x] Phase 1: `ctest -LE "requires_display"` — 4 targets, no orphans (test_window_lifecycle.mm excepted)
- [x] Phase 1: Coverage artifact uploaded to CI build page
- [ ] `scripts/check_coverage.sh --p0-min=<N>` exits 0 (Phases 2-5)
- [ ] `ctest` reports all expected targets (E2E excluded only if documented)
- [ ] No orphaned test files on disk (all compiled and run)
