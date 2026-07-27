# Plan — Future Work

Forward-looking only. Completed work lives in git history and `CHANGELOG.md` —
do not accumulate DONE entries here.

Baseline as of v1.75 (2026-07-27): P0 95.24%, P1 33.85%, total 81.27%,
1615 tests. Ratchet in CI is 94/30/79.

---

## Coverage: reach 95% on testable code

The agreed definition of done is 95% on code that CAN be tested, with thin AppKit
shells excluded under `coverage_eligible.txt` rule (b) — never before their logic
has been extracted in a prior commit.

**No-refactor targets** (plain tests, ~230 lines total, highest value per effort):
- `cursor_backend.mm` — 37 uncovered
- `effect_reg_pomodorobed.mm` — 43
- `audio.mm` — 36
- `assets.mm` — 28
- `effect_reg_footprint.mm` — 24
- `behavior_element_window.mm` — 60

**Extraction targets** (follow `item_window_logic.cpp` / `effect_window_logic.cpp`):
- `tick_manager.mm` — 158 uncovered. Extractable: the per-goose window frame
  maths, the `tickCount % kWorldCleanupTickInterval` cadence, the leaf-spawn
  state machine (first-run burst vs 1-in-500 roll), display-link FPS clamping.
- `item_window.mm` — 312 remaining after the first extraction pass. Still holds
  drag-delta maths and `drawRect` geometry worth lifting.
- `effect_window.mm` — 230 remaining. `-syncWindows` phase logic is extractable;
  the `EffectContentView` drawing is not.

Once those are thin shells, exclude them under rule (b) and hold 95% on the rest.

**Measured reality:** extraction yields roughly 0.25pp of total per pass. Do not
promise a total-coverage number that assumes the AppKit shells get covered — they
cannot be, without a windowed harness that would make CI flaky.

**Deliberately uncovered:** `app_cli.cpp`'s `DaemonizeProcess()` (42 lines)
`posix_spawn`s a real CadGoose instance. That belongs to integration, not unit
tests. This is a decision, not an oversight.

---

## Repair two dead test files

Both exist on disk but are absent from `CMakeLists.txt`, so they do not run:

- `tests/platform/macos/test_window_lifecycle.mm` (304 lines) — uses
  `CGWindowListCreateImage`, obsoleted in macOS 15; needs a ScreenCaptureKit
  port. Also calls `-drawRect:` on `ItemWindow`, which no longer declares it.
  Worth reviving: it drives real windows and would reach code nothing else does.
- `tests/common/test_config_gui_rendering.mm` (144 lines) — written against
  XCTest, not GoogleTest, so it cannot link into `CadGooseTests`. Either port the
  assertions or give it its own target.

Any test file added to `tests/` must be registered in `CMakeLists.txt`. There is
currently nothing that catches an unregistered one — a gate for this would have
caught ~1200 lines of tests silently not running.

---

## Verify the shipped bundle

The v1.75 DMG has never been launched — CI builds the `.app` but nothing runs it.
Mount the DMG, launch the bundled app, confirm the goose appears and behaves
(wander, drag, drop an item, close it), on both light and dark backdrops. v1.75
touched the behavior tick path and both window classes, so a green test suite is
not sufficient evidence here.

---

## Open correctness question: rotated held-item window sizing

`CalculateGooseWindowSize` uses `max(rotatedAABB.x, rotatedAABB.y) * 0.5` as the
held item's extent from its centre. The true bound is half the diagonal, which is
rotation-invariant. For a long thin item the AABB's max dimension SHRINKS toward
45 degrees (900x60 -> ~679x679), so the computed extent drops from 450 to ~339
while the requirement stays ~451 — rotating a long item makes its window smaller.

`GooseWindowSizeTest.RotatedLongItemExtentIsBelowTheHalfDiagonalBound` pins the
current behaviour so any change is deliberate. NOT yet established whether it
visibly clips: there is 40px of padding and the item is drawn out at the beak, so
the slack may absorb it. Needs a user-POV check before it is called a bug.

---

## Notarization

- Developer ID signing + hardened runtime + `com.apple.security.cs.allow-jit`
  entitlement
- Notarytool submission + staple
- Remove the `xattr` step from the install docs

---

## Minor

- `config/config.toml` is rewritten by the CMake configure step, so local edits
  (e.g. `mcp_port`) silently revert on every build. Either stop templating it over
  the tracked file or move the runtime copy out of the source tree.

---

## Note for whoever plans the next phase

The previous version of this file listed Phase 8 ("CI Gate Hardening") as DONE on
the strength of the script existing and the CI step being wired. The gate had
never measured a single file — it died on a quoted glob before reading anything,
and its 95/80/90 thresholds were never met by real code. A gate that has not
printed its number is a claim, not a gate. Do not mark coverage work DONE without
pasting the number a real run produced.
