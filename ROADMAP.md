# CadGoose — Roadmap

Forward-looking only. Shipped work lives in git history and `docs/CHANGELOG.md`;
do not accumulate DONE entries here. This is the ONE plan file: no other file in
the tree (AGENTS.md included) carries a backlog.

Rephased 2026-10-12 from every agent session that worked on this repo (see
*Sources consolidated*). The plan it replaces: `git show a3353d2:docs/PLAN.md`;
the session log that used to live in AGENTS.md: `git show a3353d2:AGENTS.md`.

## Context (measured 2026-10-12, not copied; gate state 2026-10-14)

- Released as **v1.81** (`a3353d2`, 2026-10-08); the Homebrew cask in the
  tap points at 1.81. CI green on `b1d6b50` (release + PR check, 2026-10-12);
  CI does not run the house structural gate, the local pre-push does.
- History was flattened on 2026-09-04 (`af63341`, 320 commits). Commit shas
  quoted in docs/CHANGELOG.md from before that date no longer resolve.
- Every commit since the flatten is hygiene: gates, CI hardening, docs, one MCP
  protocol-negotiation change. **No feature work has landed since 2026-09-04**;
  feature development happens in a separate macOS-only fork (D1, ruled: maintenance line).
- 1697 `TEST`/`TEST_F`/`TEST_P` cases under `tests/`. Coverage ratchet in CI:
  P0 94 / P1 53 / total 85 (`build_and_release.yml:106`), whole-binary floor 64
  (`scripts/coverage_whole_binary_floor.txt`) — but see G0.1 for what the tiers
  actually measure.
- The repo is PUBLIC (MIT). Owner ruling 2026-09-04 (`ae819a7`): it must not
  name or depend on the author's private repositories. Keep it that way here.

---

## D1 — RULED 2026-10-12: maintenance line (option A)

Owner ruling, verbatim: "CadGoose is maintenance". This repo keeps CI, gates,
dependencies and the release/cask path green and current, and keeps the Linux
build building. **No new features here**: feature development happens in a
separate, private macOS-only successor fork. Phases 0-1 are the work; Phase 2
is parked for good unless this ruling is reversed. Expected cost: about one
session per quarter.

Rejected: B (retire — would lose the public and Linux builds) and C (keep
developing — two goose code bases drifting apart).

**D1.1 Shared settings directory (do in Phase 1).** Both apps read and write
`~/Library/Application Support/CadGoose` (`ConfigDirPath()` here; the fork
documents the same path). With both installed they share `config.toml` and
`themes/`, and the two schemas have diverged. Under A, move this app to its own
directory with a one-time migration. Proof: a unit test pinning
`ConfigDirPath()` per bundle id, plus a migration test (old dir present and new
dir absent, so it copies; both present, so nothing is touched).

---

## Phase 0 — Wire in goh 0.27.0: closed 2026-10-14

`tools/gate.sh --full` green at `37ed5f7`, pushed through the pre-push gate.

- **W.1** `no early-exit pipe under pipefail` (7 sites, no seed): `f86ac8d`
- **W.2** `no committed secrets`, the tap token via `http.extraheader`: `0bd05f3`.
  Unproven until the next release's cask bump lands.
- **W.3** `every child process names its stdin` (1 call): `9c14b16`; the
  `GOH_CI_STEPS` shim through `gates/python.sh`: `177cba2`
- **W.4** Scanner floors (13 keys, calibrated red): `37ed5f7`
- **W.5** Stock hooks reinstalled: `6605f91`

## Phase 0b — Gate holes and false claims (S, 1 session)

**Goal:** every gate measures what it says it measures, and every written claim
matches the code. **Exit:** each item's proof is in the tree and has been seen
to go red once.

- **G0.1 Coverage tiers skip 43 production files.** `scripts/coverage_eligible.txt`
  globs `src/common/*.cpp` (P0) and `src/platform/macos/*.mm` (P1). `compgen -G`
  does not recurse, so `src/common/behaviors/` (17 behavior `.cpp` + 12 AI/LLM
  `.mm`) and the 14 `src/common/*.mm` files (`goose_drawing.mm`,
  `item_renderer.mm`, the actor `.mm` files...) are in NO tier. Only the
  whole-binary floor of 64 sees them. Fix: tiers cover every production source,
  or name each file in an `EXCLUDE` with a reason. Add a check that fails when a
  file under `src/` matches no tier. *Proof:* add an untiered `.cpp`, and the
  check goes red. *Risk:* P0 and P1 drop once the scope grows. Record the
  honest new floors and the size of the drop (rule 8); do not keep the old
  numbers by narrowing the scope.
- **G0.2 CI excludes tests that were fixed.** The `--gtest_filter` (duplicated
  in `scripts/run_tests_ci.sh` and `cmake/Tests.cmake:295`) still excludes
  `BehaviorToggles.ToysBehaviorRegistered`, `PortalCleanup.BehaviorHasCleanupFunction`
  and `StalinHonk.*`, which were fixed on 2026-06-22 (registry
  `SaveOriginal`/`RestoreOriginal`). It also excludes all of `MCPIntegrationTest*`,
  which has run in-process since the hotkey-cache fix removed the main-thread
  hop. Fix: one filter source, one reason per remaining exclusion, and
  un-exclude those that pass. *Proof:* the un-excluded tests pass under
  `--gtest_shuffle --gtest_repeat=3`, and a check fails if the two filter copies
  differ (or one copy is deleted).
- **G0.3 MCP tests bind a fixed port (31073).** Two concurrent `CadGooseTests`
  processes collide (`tests/common/test_mcp_http_server.cpp:19,36`), and the
  failure reads like an MCP bug. Fix: bind port 0 and read it back. *Proof:* two
  test binaries running the MCP suites at once both pass. Depends on G0.2.
- **G0.4 Deprecated API, no deprecation gate.** `activateIgnoringOtherApps:`
  (deprecated in macOS 14) at `src/platform/macos/config_gui.mm:403` and
  `src/common/behaviors/ai_chat_window.mm:442`. CMake has no
  `-Werror=deprecated-declarations`, so the next one also lands silently (rule
  17). *Proof:* the flag on the app and test targets, the build green,
  reintroducing one call goes red.
- **G0.5 Platform policy is not a hard gate.** CMake rejects only non-macOS and
  non-Linux systems (`CMakeLists.txt:291`). An Intel Mac configures and fails
  later at the arm64-only Swift object (`CMakeLists.txt:171`), and a non-x86_64
  Linux would build untested. Fix: reject macOS x86_64 and any 32-bit target,
  keep Linux x86_64, behind an overridable `OS`/`ARCH` seam. *Proof:* a test
  that asserts both the reject and the accept directions on one machine.
- **G0.6 CI actions are three majors behind, and one runner label moves.**
  `actions/checkout@v4` (latest v7), `actions/cache@v4` (v6),
  `actions/upload-artifact@v4` (v7). `ubuntu-latest` is a moving label. *Proof:*
  current majors pinned, a named Ubuntu runner, CI green on a push and a
  `workflow_dispatch` release dry run.
- **G0.8 Stated vs implemented.** The `check_coverage.sh` usage line
  advertises `--p1-min=30 --total-min=79`, while the defaults are 53/85 (the
  hook-comment half moved to W.5). *Proof:* the text matches the code,
  verified by grep.
- **G0.9 `.gitignore` ignores `*.md` behind a whitelist.** Any new doc is
  silently untracked: this file needed a whitelist entry to be committed, and a
  stale entry for a deleted `docs/CLEANUP_PLAN.md` was still listed. Fix:
  replace the blanket rule with explicit ignores for the generated markdown
  (`tools/profiling/soak_results_*.md`, stray probe files). *Proof:* a new
  `docs/X.md` shows up in `git status`.
- **G0.10 Every clean configure clones toml11 with its full history and all
  of its submodules.** `FetchContent_Declare(toml11 ... GIT_TAG v4.4.0)` sets
  neither `GIT_SHALLOW` nor `GIT_SUBMODULES ""`. The pre-push gate (`rm -rf
  build && cmake ...`) therefore spends minutes on every push cloning
  nlohmann/json, doctest and a Hugo theme that nothing builds (measured on the
  2026-10-12 push). Fix: a shallow fetch with no submodules, or a release
  tarball with `URL_HASH`. *Proof:* `build/_deps/toml11-src/tests/extlib/` is
  empty after configure, and the configure step's wall time is recorded before
  and after.
- **G0.11 Stale remote branch `origin/unify/harnesses`.** It predates the
  flatten (no merge base with `main`). Its 2026-08-25 file splits are all on
  `main`, and its harness wiring was superseded by `75eb43f`. Delete it on the
  remote; this needs owner approval because it changes the remote.

## Phase 1 — Maintenance-line correctness (S-M, 1 session)

**Goal:** no setting, state or race that silently does something other than
what it says. **Exit:** each proof below is green in CI.

- **M1.1 `audio_enabled` is overwritten on every load.**
  `src/common/config_load.cpp:113` forces `audioEnabled = true` on macOS each
  time the config loads. The setting is registered (`config_registry_generated.cpp:22`)
  and settable over the socket and MCP, but it never holds. This is a one-time
  migration for configs written by an old `false` default that was left running
  forever. Fix: a versioned one-shot migration, or remove the key. *Proof:* set
  `false`, reload, and it stays `false`; an old-schema file migrates once.
- **M1.2 Hotkey save race (documented, not fixed).** `Config_SaveAll` (via
  `config_registry_behaviors.cpp`) reads the hotkey strings without the hotkey
  mutex, racing an MCP `set_hotkey`. It is cold-path and non-crashing, but it is
  a real data race. *Proof:* the save path reads through `Hotkey_Name()`, and a
  TSan test that races save against `set_hotkey` is clean.
- **M1.3 Trail-detection false positive (status unverified since 2026-06-22).**
  The trail scan counted frames containing dropped items as "trails". Measure
  first. *Proof:* `tools/profiling/run_trail_test.sh` reports 0 trails with
  dropped items on screen and at least 1 for a planted trail. Close with the
  measurement if it already holds.
- **M1.4 Notarization** (carried from the old plan). Developer ID signing, the
  hardened runtime and the `com.apple.security.cs.allow-jit` entitlement;
  `notarytool` submit and staple; remove the `xattr` step from the README, the
  cask `postflight` and docs/HOMEBREW.md. **Blocked** on a paid Apple Developer
  certificate. *Unblocks:* the credential existing in the repo's secrets.
  *Proof:* `spctl -a -vv` accepts the downloaded DMG's app.

## Phase 2 — Feature-line work (PARKED: D1 ruled A on 2026-10-12)

**Goal:** close the remaining coverage and parity gaps. Parked: reopens only if
the owner reverses D1. C2.3's "keep Linux building; do not grow it" is the
standing rule meanwhile.

- **C2.1 Coverage of the AI layer.** `ai_http_client.mm`, `ai_local_llm_adapter.mm`
  and `local_llm_{inference,model,tokenizer}.mm` (about 60 KB). Measure them
  first under G0.1's corrected scope, then put the HTTP transport and the LLM
  backend behind seams with fakes. *Proof:* the tier floor is raised by the
  measured gain.
- **C2.2 P1 AppKit shells** (carried from the old plan). The decision layers of
  `item_window.mm` (drawRect), `effect_window.mm` (EffectContentView) and
  `tick_manager.mm` (display link) are already extracted into `*_logic`. What
  remains needs a windowed harness. Decision on record: do not promise coverage
  of these without one, and do not exclude them under rule (b) of
  `coverage_eligible.txt` unless the extraction has landed first.
  `app_cli.cpp`'s `DaemonizeProcess()` is deliberately left to integration
  tests.
- **C2.3 Linux parity.** The chat UI is not implemented
  (`ai_chat_window.mm:445`), the honcker and portal behaviors have Linux stubs,
  `Config_IsSystemDarkTheme()` always returns false (`linux_stubs.cpp:168`), and
  the theme editor has no GTK half. Under A, keep Linux building; do not grow
  it.
- **C2.4 `CGRenderer::DrawText` creates a `CGColor` per call**
  (`include/cg_renderer.h:132`). Do this only if a profile shows text or
  nametags as hot. The 2026-06-28 profile decision ("stop here: remaining
  self-time is normal CoreGraphics churn") still stands.

---

## Closed or dropped in this rephase (evidence, not narrative)

- The multi-metric CI coverage gate (old "Phase 8") is DONE: `check_coverage.sh`
  with P0/P1/total floors runs in `build_and_release.yml:106`.
- The `g_world.droppedItems` migration is DONE: no `g_world.droppedItems`
  references remain, and `ActorManager::getDroppedItems()` owns the list.
- The `test_window_lifecycle.mm` orphan was reclaimed (`4b05641`, 4/4 green).
- Config key collisions were fixed by `lookupKey` (2026-06-22).
- The `--version` flag shipped before the flatten.
- DROPPED: `void*` ObjC pointers in actor headers (cosmetic, no defect); config
  generator GUI generation (intentionally incompatible with the key-based GUI);
  "test process uses about 7 GB" (a ring-buffer property of the trail test, not
  a defect).
- `CMakeLists.txt` over the 500-line cap (goh's Downstream list, counted 702
  at `e1fa2b3c`): already fixed by `b377d79` (2026-10-08, tests moved to
  `cmake/Tests.cmake`); 295 lines on 2026-10-14, `file length <= 500` green.
- CORRECTED: AGENTS.md said the registry carries different *defaults* from the
  config struct (for example "dropInterval: registry 10"). It does not. The
  `CONFIG_FLOAT` arguments are min/max/step (`include/config.h:28`), and the
  struct initializers are the only defaults.

## Sources consolidated (2026-10-12)

- `docs/PLAN.md`: moved here (`git mv`), and its two open items carried (C2.2,
  M1.4).
- `AGENTS.md` (817 lines, mostly session summaries): the plan parts are folded
  here (Next Steps → C2.x and the closed list; Known Bugs → M1.3 and the closed
  list; "Remaining known issues" → G0.2). The session log was pruned to git
  history. AGENTS.md keeps the agent instructions only.
- Antigravity conversation "push release" (2026-06-26, release-script and CI
  validator fixes): superseded. `scripts/push_release.sh` and `verify_ci.sh`
  are in the tree, and v1.77 and v1.81 have shipped since.
- Antigravity conversation on meme rendering research (2026-07-10, a research
  subagent): superseded by the 2026-07-10 ImageIO fix in docs/CHANGELOG.md.
- Gemini CLI project store (110 chats, 275 checkpoints, May-June 2026) and its
  four plan files: theme editor (shipped on macOS; the GTK half → C2.3),
  physics and pat sounds (shipped: `Audio_PlayPat`), config GUI registry
  refactor (shipped), 500-line file diet (shipped: `app_cli.cpp`, the
  `GOH_MAX_LINES=500` gate).
- Two Antigravity IDE conversations that only list this directory in passing
  (an SMB share question and git repo size): not this project's, so nothing
  was carried.
- Claude Code memory store for this repo (five notes, kept under an old
  lower-case path slug): its facts were checked against the code. The
  hotkey-cache residual race → M1.2, and the MCP fixed port → G0.3. The store
  stays where it is.
- `origin/unify/harnesses`: superseded (G0.11).
- opencode: the `.opencode/` directory holds only plugin package files; there
  are no loop jobs or goals.
- Cross-project Rust roadmap: CadGoose `src/` is on its "stay as-is"
  allow-list, so there is no item to carry.
- gates_of_heck v0.27.0 Downstream (2026-10-14): stdin -> W.3, early-exit
  pipes -> W.1, secrets + tap_bump -> W.2, stale hooks -> W.5, floors -> W.4,
  line cap -> already closed (above).
