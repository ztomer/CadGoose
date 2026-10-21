# CadGoose — Roadmap

Forward-looking only. Shipped work lives in git history and `docs/CHANGELOG.md`;
do not accumulate DONE entries here. This is the ONE plan file: no other file in
the tree (AGENTS.md included) carries a backlog.

Rephased 2026-10-12 from every agent session that worked on this repo (see
*Sources consolidated*). The plan it replaces: `git show a3353d2:docs/PLAN.md`;
the session log that used to live in AGENTS.md: `git show a3353d2:AGENTS.md`.

## Context (measured 2026-10-21, not copied; gate state 2026-10-21)

- Released as **v1.83** (`321a5d0`, 2026-10-19); the Homebrew cask in the
  tap points at 1.83 (`fd57318`). The tag is lightweight, like v1.80-v1.82.
  It carried M1.7 (the MCP `get_config` and `resources/read` reads on the
  owning thread) and the quiet release binary (`--version` prints its one
  line and nothing else). CI green on the release run `39289049100` (macOS
  DMG, Linux tar.zst); CI does not run the house structural gate, the local
  pre-push does. Actions are ON, so the run ids here are real: the release run
  above is the `v1.83` release (green), and the tree's newest runs are
  `39501628954` (Build & Release Artifacts) and `39501628893` (PR Build
  Check), both green on 2026-10-20.
- History was flattened on 2026-09-04 (`af63341`, 320 commits). Commit shas
  quoted in docs/CHANGELOG.md from before that date no longer resolve.
- Every commit since the flatten is hygiene or maintenance, not feature work:
  gates, CI hardening, docs, one MCP protocol-negotiation change, and the
  Phase 1 correctness fixes (M1.1-M1.7, 2026-10-18/19). **No feature work has
  landed since 2026-09-04**; feature development happens in a separate
  macOS-only fork (D1, ruled: maintenance line).
- 1765 `TEST`/`TEST_F`/`TEST_P` cases under `tests/` (1258 + 503 + 4; measured
  2026-10-21 — the same count read 1697 at the 2026-10-12 rephase). Coverage
  ratchet in CI: P0 95 / P1 26 / total 63, set once in
  `scripts/check_coverage.sh` (re-read 2026-10-21, unchanged), and the
  whole-binary floor (65) in `scripts/coverage_whole_binary_floor.txt`; the
  tiers cover every production source (G0.1, 2026-10-16).
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
  Superseded 2026-10-17: the CI step that held the token is gone; the cask
  is bumped by `scripts/push_release.sh` through goh's `tap_bump.sh` and the
  tap's own gate (homebrew-tap T2.1). Proven on a scratch tap: v1.81 with
  the published DMG is a no-op. The `HOMEBREW_TAP_TOKEN` secret was deleted
  (2026-10-17, owner's go-ahead).
- **W.3** `every child process names its stdin` (1 call): `9c14b16`; the
  `GOH_CI_STEPS` shim through `gates/python.sh`: `177cba2`
- **W.4** Scanner floors (13 keys, calibrated red): `37ed5f7`
- **W.5** Stock hooks reinstalled: `6605f91`

## Phase 0b — Gate holes and false claims: closed 2026-10-21

**Goal:** every gate measures what it says it measures, and every written claim
matches the code. **Exit:** each item's proof is in the tree and has been seen
to go red once. Every item in it has landed; the record is
`docs/CHANGELOG.md`, not here.

- **G0.17** the DMG is made by `diskutil image create from`, and the floor
  check mounts it and reads it green: `7e6b66f`.
- **G0.18** the in-tree cask is gone and the tap's own gate holds the tap's
  copy: `7e6b66f` (homebrew-tap `731d9a1`, `b03dc29`).
- **G0.11** the stale remote branch `origin/unify/harnesses` is no longer on
  the remote — `git ls-remote --heads origin` lists only `refs/heads/main`
  (measured 2026-10-21). It was a remote change, so it carries no commit in
  this tree; the branch's 2026-08-25 file splits are all on `main`, and its
  harness wiring was superseded by `75eb43f`.

## Phase 1 — Maintenance-line correctness (S-M, 1 session)

**Goal:** no setting, state or race that silently does something other than
what it says. **Exit:** each proof below is green in CI.

M1.1-M1.3 closed 2026-10-18; M1.5-M1.7 closed 2026-10-19. M1.7 is `5ca7055`
(test `6361127`): the `get_config` and `resources/read` TSan cases exit 134
against the pre-fix handler and pass clean after it; docs/CHANGELOG.md.
Open:

- **M1.4 Notarization** (carried from the old plan). Developer ID signing, the
  hardened runtime and the `com.apple.security.cs.allow-jit` entitlement;
  `notarytool` submit and staple; remove the `xattr` step from the README, the
  cask `postflight` and docs/HOMEBREW.md. **Blocked** on a paid Apple Developer
  certificate. *Unblocks:* the credential existing in the repo's secrets.
  *Proof:* `spctl -a -vv` accepts the downloaded DMG's app.
- **Re-check the shipped app after a release.** The cask is what the owner's
  machine runs, and until 2026-10-19 it ran 1.82 while the tree was already
  quiet: the fix was in the tree and the installed app was not. The cask bump
  and the install are the two steps between a landed fix and a shipped one.
  *Proof:* `brew list --cask --versions cadgoose` names the tag's version, and
  `/Applications/CadGoose.app/Contents/MacOS/CadGoose --version` writes one
  line to stdout and nothing to stderr.

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

## Open, and who can move it

What is open in this repo, and the hand each one needs. An agent can move the
first and the last; the second and the third are the owner's.

- **M1.4 Notarization** — **owner.** A paid Apple Developer certificate in
  the repo's secrets is the only thing that unblocks it; no commit here moves
  it.
- **Re-check the shipped app after a release** — **agent**, on the owner's
  machine, after each release: bump the cask through the tap's own gate,
  install it, read `--version`. Done for v1.83 (`321a5d0`); it stands for the
  next release.
- **D1.1 Shared settings directory** — **agent**, in Phase 1. The proof is a
  unit test pinning `ConfigDirPath()` per bundle id, plus a migration test
  (old dir present and new dir absent, so it copies; both present, so nothing
  is touched).
- **Phase 2 (C2.1-C2.4)** — **parked** by D1 (owner ruling, 2026-10-12). It
  reopens only if the owner reverses that ruling.

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
  list; "Remaining known issues" → G0.2, closed 2026-10-17). The session log was pruned to git
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
