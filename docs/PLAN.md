# Plan — Future Work

Forward-looking only. Completed work lives in git history and `CHANGELOG.md` —
do not accumulate DONE entries here.

Baseline as of 2026-08-22: P0 95.27%, P1 54.44%, total 86.01%,
1650 tests. Ratchet in CI is 94/53/85.

---

## Coverage: reach 95% on testable code

The agreed definition of done is 95% on code that CAN be tested, with thin AppKit
shells excluded under `coverage_eligible.txt` rule (b) — never before their logic
has been extracted in a prior commit.

P0 sits at 95.27% — effectively at the target; the remaining misses are
socket-failure paths in `mcp_*` and `app_cli.cpp`'s `DaemonizeProcess()`
(deliberate, see below). P1's gap is AppKit drawing shells (`item_window.mm`
drawRect, `effect_window.mm` EffectContentView, `tick_manager.mm`'s display-link
plumbing) whose decision layers are ALREADY extracted into
`item_window_logic` / `effect_window_logic` / `tick_manager_logic`. What remains
there is not extractable without a flaky windowed harness.

**Deliberately uncovered:** `app_cli.cpp`'s `DaemonizeProcess()` (42 lines)
`posix_spawn`s a real CadGoose instance. That belongs to integration, not unit
tests. This is a decision, not an oversight.

**Extraction yield note:** each extraction pass moved total coverage ~0.25pp.
Do not promise a total-coverage number that assumes the AppKit shells get
covered — they cannot be, without a windowed harness that would make CI flaky.

---

## Notarization

- Developer ID signing + hardened runtime + `com.apple.security.cs.allow-jit`
  entitlement
- Notarytool submission + staple
- Remove the `xattr` step from the install docs

Requires a paid Apple Developer account/certificate; nothing to do until the
credential exists.
