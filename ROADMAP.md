# CadGoose — Roadmap

Forward-looking only. Finished work lives in git history and `docs/CHANGELOG.md`;
this file carries open work and the rulings still in force, nothing else. It is
the ONE plan file: no other file in the tree (AGENTS.md included) carries a
backlog. The plan this one replaced: `git show a3353d2:docs/PLAN.md`; the old
session log: `git show a3353d2:AGENTS.md`.

Rephased 2026-11-08 (owner: "rework, rephase, harden the roadmaps"). No open
item of this repo is in the umbrella queue (`~/Projects/.loop/queue.json`); the
queue's CadGoose items are all closed, and RS (Rust-tification) excludes this
repo by ruling.

## State (measured 2026-11-08)

- Latest release **v1.83** (`321a5d0`, lightweight tag, like v1.80-v1.82); the
  tap's cask is at 1.83 and `brew list --cask --versions cadgoose` reads
  `cadgoose 1.83`. `main` is level with `origin/main` at `9a201bf`
  (2026-11-07); both CI workflows green on it (run `42419320312`, Build &
  Release Artifacts; `42419320311`, PR Build Check). CI does not run the house
  structural gate; the local pre-push does.
- 1777 `TEST`/`TEST_F`/`TEST_P` cases under `tests/`. Coverage ratchet: P0 95 /
  P1 26 / total 63 in `scripts/check_coverage.sh`; whole-binary floor 65 in
  `scripts/coverage_whole_binary_floor.txt`.
- History was flattened on 2026-09-04 (`af63341`); shas quoted in
  `docs/CHANGELOG.md` from before that date no longer resolve.

## Rulings in force

- **D1, 2026-10-12, owner:** "CadGoose is maintenance". This repo keeps CI,
  gates, dependencies and the release/cask path green and current, and keeps
  the Linux build building. No new features here; feature development happens
  in a separate macOS-only successor fork. Feature-line work (Phase 3) reopens
  only if the owner reverses D1. Expected cost: about one session per quarter.
  (B, retire, and C, keep developing, were rejected.)
- **Public repo, 2026-09-04, owner (`ae819a7`):** the repo is public (MIT) and
  must not name or depend on the author's private repositories.
- **Rust-tification, 2026-10-28, owner (queue item RS):** CadGoose v1 is in
  maintenance mode only and is excluded from the Rust port.
- **Linux, standing under D1:** keep Linux building; do not grow it.

---

## Phase 1 — Prove each release reached the machine (agent, recurring)

Why first: it is the only open item an agent can move, it costs minutes, and
until 2026-10-19 the owner's machine ran 1.82 while the fix was already in the
tree; every later phase assumes a release means an installed release.

- **REL.1 Re-check the installed app after each release.** The cask bump (through
  the tap's own gate, `scripts/push_release.sh`) and the install are the two
  steps between a landed fix and an installed one.
  *Done when:* `brew list --cask --versions cadgoose` names the new tag's
  version, and `/Applications/CadGoose.app/Contents/MacOS/CadGoose --version`
  writes one line to stdout and nothing to stderr. Last met for v1.83.

## Phase 2 — Notarization (owner prerequisite)

Why second: it changes the release path itself, so it comes before any further
release work, but nothing in this repo can move it until the owner supplies the
credential.

- **M1.4 Notarization.** Developer ID signing, the hardened runtime and the
  `com.apple.security.cs.allow-jit` entitlement; `notarytool` submit and
  staple; remove the quarantine `xattr` step from the README (line 38) and from
  the tap's cask `postflight` (`homebrew-tap/Casks/cadgoose.rb:16`).
  **Blocked on the owner:** a paid Apple Developer certificate in the repo's
  secrets.
  *Done when:* `spctl -a -vv` accepts the app from the downloaded DMG with the
  `xattr` step gone from README and cask.

## Phase 3 — Feature-line work (PARKED by D1)

Why last: D1 parked it; it reopens only on an owner reversal. Order if it does:
measure before building (C2.1), then the harness that unlocks more coverage
(C2.2), then the optional items.

- **C2.1 Coverage of the AI layer.** `src/common/behaviors/ai_http_client.mm`,
  `ai_local_llm_adapter.mm` and `local_llm_{inference,model,tokenizer}.mm`
  (about 60 KB). Measure them first, then put the HTTP transport and the LLM
  backend behind seams with fakes (`ai_http_client_mock.cpp` and
  `ai_local_llm_mock.cpp` exist for the Linux build).
  *Done when:* the five files' measured coverage is recorded in the commit and
  a P1 (or total) floor in `scripts/check_coverage.sh` is raised by that
  measured gain with the coverage gate green.
- **C2.2 P1 AppKit shells.** The decision layers of `item_window.mm`
  (drawRect), `effect_window.mm` (EffectContentView) and `tick_manager.mm`
  (display link) are already extracted into `*_logic`; what remains needs a
  windowed harness. Do not promise coverage of these without one, and do not
  exclude them under rule (b) of `scripts/coverage_eligible.txt` unless the
  extraction has landed first. `app_cli.cpp`'s `DaemonizeProcess()` is left to
  integration tests.
  *Done when:* a windowed harness (tests labelled `requires_display`) drives
  those three views and their files count toward the tier floors, with the
  floor raised by the measured gain.
- **C2.3 Linux parity.** The chat UI is not implemented
  (`src/common/behaviors/ai_chat_window.mm:445`), the honcker and portal
  behaviors have Linux stubs, `Config_IsSystemDarkTheme()` always returns
  false (`src/platform/linux/linux_stubs.cpp:168`), and the theme editor has no
  GTK half. Under D1 this is not grown.
  *Done when:* (owner to decide)
- **C2.4 `CGRenderer::DrawText` creates a `CGColor` per call**
  (`include/cg_renderer.h:132`). Do it only if a profile shows text or
  nametags as hot; the 2026-06-28 profile decision ("stop here: remaining
  self-time is normal CoreGraphics churn") stands.
  *Done when:* (owner to decide) the hotness threshold; the proof would be a
  before/after profile of the `--foreground` build showing the call drop out.
