# Rendering Bugs — Post-IRenderer-Migration Regression Report

Date: 2026-05-19

After the §3 IRenderer enforcement pass migrated 10 behaviors off
`(CGContextRef)nativeContext()` casts to pure `IRenderer` calls,
several visual regressions appeared. Many share a root cause:
the legacy code applied Y-flips and bottom-up coordinate math
inside the CG-specific path; the `IRenderer` migration kept the
top-level transforms but lost some of the per-call adjustments
(text matrix flip, image flip-before-draw, position from-screen-
bottom math). A few are unrelated: window plumbing per goose,
effect-window positioning, dropped-item-actor window plumbing
for items the goose has no business owning, etc.

## Reported bugs

| # | Symptom | Suspected root cause | Severity |
|--:|---|---|:--:|
| 1 | Nametag is upside down | `IRenderer::DrawText` doesn't apply the per-draw Y-flip that the legacy `CGContextSetTextMatrix(cg, MakeScale(1, -1))` did; the renderer's text matrix is still the CG default (no Y-flip), so glyphs draw upside-down in our top-left coord system | High |
| 2 | Before first fetch the goose "skates" — leg animation runs ~slow until first item is fetched, then normalizes | Foot-step state machine isn't initialized to a "moving" cycle on spawn; `lastStepSoundTime` or similar accumulates from `-1e9` and biases timing. First fetch resets internal state and unblocks normal cycle | Med |
| 3 | Portal windows are cut off — only lower-right visible | `PortalActor` element-window rect computed from a position that's not the top-left of the window, but the *center*. The window is sized for full portal but anchored wrong, clipping ~75% of it off the top-left | High |
| 4 | Autumn leaf piles only show the bottom of the pile | `LeafPileActor`'s effect-window size doesn't cover the Z-extent of leaves above the ground plane; leaves fountain up but get clipped at the top of an undersized window. (Particle Z is converted to a vertical offset; window height needs to accommodate maxZ.) | Med |
| 5 | All geese render in the same window — only the primary is visible most of the time | `WindowManager` (`window.mm`) creates one `GooseWindow` per screen and assigns *only the front of the geese list* to it via `updateWindowPositionsForGeese`. Multi-goose support was never finished; we need one `GooseWindow` per goose, not per screen | High |
| 6 | Mud footprints not visible | Footprint render path was likely tied to the now-deleted `ui_drawing.cpp` on Linux, *and* on macOS the per-frame effect-window for footprints uses positions that need a screen-height-anchored Y flip the post-migration code dropped | High |
| 7 | Breadcrumbs cut off | Same family as #3/#4 — `BreadcrumbActor`'s effect-window rect uses raw `position.y` for the top of the window, but the visual sprite is centered on `position`; top half of the breadcrumb is above the window's top edge and clipped | High |
| 8 | Hats not visible | We dropped the `CGBitmapContextCreate` pre-scaled cache in the §3 migration *and* moved to `DrawImage` with raw image dims; the existing `Translate/Scale` to flip-and-center may now be flipping into negative coords that the window clips, so the hat draws outside the goose window | High |
| 9 | drag, ball, acid, honcker, health, rainbow look fine | No-op — these are the behaviors we did NOT touch with per-draw coordinate flips during the §3 migration. They confirm the regression is localized to the flip-sensitive ones | — |
| 10 | Jail not visible | `JailActor` window position likely uses cursor-space coords that didn't get the device-coord transform; or the window size is zero on macOS because of a missing assets path | High |
| 11 | Pomodoro: timer drifts to top-left instead of bottom-right; bed not visible; never sleeps; text is upside down | Multiple regressions stacked: (a) bed position uses `g_world.screenWidth/Height` which in some tick paths is still zero → bed lands at origin; (b) the "sleep" trigger checks distance to bed, never reached when bed is at (0,0); (c) timer text upside-down same root cause as #1 (text matrix); (d) bed sprite hidden by zero-sized effect window OR by being off the goose's own window | High |

## Common-cause clusters

These reports collapse into ~4 underlying defects:

- **Cluster A — text Y-flip missing in IRenderer::DrawText**
  Affects: #1 nametag, #11(d) pomodoro timer.
  Fix in `cg_renderer.h`: apply `CGContextSetTextMatrix(m_ctx, CGAffineTransformMakeScale(1.0, -1.0))` inside `DrawText` (save/restore the existing matrix), so callers don't need to know about it.

- **Cluster B — effect-window rects use position as top-left when it's center (or vice versa)**
  Affects: #3 portal, #4 leaves, #7 breadcrumbs, possibly #8 hats and #10 jail.
  Fix per-actor: when computing `BehaviorElementWindow` device-rect, account for the sprite's half-extent on both axes so the window encloses the full sprite area centered on `position`.

- **Cluster C — single goose window, not per goose**
  Affects: #5.
  Fix in `WindowManager`: create one `GooseWindow` per Goose actor (track by `Goose::id`), not one per screen with a shared "front of list" pointer. Update window positions per goose.

- **Cluster D — pomodoro bed position uses uninitialized/stale screen dims**
  Affects: #11(a/b/c).
  Fix `behavior_pomodoro` bed-position assignment to capture screen dims at the moment of phase-change to Break/LongBreak when `g_world.screenWidth/Height` are known-good (renderer.mm sets them on first tick), and recompute on screen-resize events.

## Investigation plan (do these in order before fixing)

1. **Verify Cluster A.** `git grep -n "DrawText" include/cg_renderer.h include/linux/cairo_renderer.h` to confirm neither backend applies the flip. Quick fix candidate; ~5 lines.
2. **Inventory effect-window rect math.** For each of `actor_portal.mm`, `actor_leafpile.mm`, `actor_breadcrumb.mm`, `actor_jail.mm`, plus `behavior_hats` and the macOS effect-registration files for footprints / pomodoro bed: list the `[BehaviorElementWindow alloc] initWith...deviceX:Y:width:height:` (or equivalent) call and what `position` represents (center? top-left?). Decide a single convention.
3. **Audit WindowManager.** Re-read `window.mm:updateWindowPositionsForGeese` and `createWindowsForAllScreens`. Plan per-goose window lifecycle: spawn-on-add (subscribe to ActorManager?), destroy on remove, follow goose pos in tick.
4. **Reproduce skating bug.** Look for `lastStepSoundTime`, `stepTime`, `rig.lFoot.moveStartTime` initial values in `Goose` constructor. Confirm whether `-1e9` causes an unintended branch in `SolveFeet`.
5. **Pomodoro state machine.** Read the phase-transition block in `behavior_pomodoro.cpp` for where `state->bedPosition` is set; check screen-dim source.

## Fix plan (do after investigation)

Phase ordering minimizes test churn:

**Phase 1 — Cluster A (text flip).** Fix `cg_renderer.h::DrawText` to save/scale-flip/draw/restore the text matrix. Verify #1 nametag right-side-up and #11(d) pomodoro timer right-side-up.

**Phase 2 — Cluster B (effect-window math).** Per actor: change window-rect computation to `{position.x - halfW, position.y - halfH, fullW, fullH}` where `halfW/H` reflect the actual sprite extent (image dimensions on macOS, fixed kSize constants on Linux). Verify #3, #4, #7, plus visually #8/#10 if they shared the same bug.

**Phase 3 — Cluster D (pomodoro bed).** Capture screen dims into `state->bedPosition` only when both are > 0; subscribe behavior_pomodoro to a screen-resize event (or just re-evaluate each tick in Work phase since it's cheap). Add a watchdog so a zero-dim bedPosition is recomputed on next tick rather than silently failing. Verify #11(a/b/c).

**Phase 4 — Cluster C (per-goose windows).** Bigger refactor: rework `WindowManager` to maintain `std::unordered_map<int, GooseWindow*>` keyed by goose id. Spawn on first sight, destroy on goose removal, drive position from the goose's `pos` each tick. Verify #5.

**Phase 5 — Skating animation.** Investigate `rig.lFoot.moveStartTime = -1.0` initial; either initialize to a small positive value or guard the duration-divide so the first frame doesn't take the `dt / -1e9` branch. Verify #2.

**Phase 6 — Mud footprints (#6).** Re-trace the post-`ui_drawing.cpp`-deletion render path. On macOS this is `effect_reg_footprint.mm` driving `EffectWindowManager`. On Linux it's done via the canvas-overlay `draw_overlay` in `ui.cpp`. Check the macOS path first since that's what builds.

Each phase ends with a `cmake --build build && ./build/CadGooseTests` green pass and a screenshot/manual sanity check of the affected behavior on the goose. Commit per phase, push to `origin/main`.

## What this doc is for

Single source of truth for the rendering regression cleanup.
Update the status column inline as each phase lands; don't open
new docs.

### Status

| Phase | Bug coverage | Status |
|--:|---|---|
| 1 — text Y-flip | #1, #11(d) | ✅ DONE — `CGRenderer::DrawText` saves the text matrix, applies `MakeScale(1, -1)`, draws, restores. Callers never see the flip. |
| 2 — effect-window rects | #3, #4, #7, #8, #10 | ✅ Mostly done — `actor_portal.mm`, `actor_breadcrumb.mm`, `actor_jail.mm` switched from `Translate(-half, -half) + DrawAt(0,0,w,h)` (which clipped against the window's top-left edge) to aspect-preserving centered draws in window-local coords. `behavior_hats.cpp` rewritten to use `WorldCoord::RigNeckHead` (device coords) instead of the goose-local `rig.neckHead` and to use a single `DrawImage` call with explicit centered rect. `actor_leafpile.mm` left as-is — the math looks correct on paper and the "only bottom shows" report needs a live screenshot to debug further. |
| 3 — pomodoro bed dims | #11(a–c) | ✅ DONE — `init()` no longer captures stale `g_world.screenWidth/Height` (which are 0 before the renderer's first tick); `tick()` recomputes the bed position from current screen dims and stores it if changed, so the bed catches up the first time dims become non-zero. |
| 4 — per-goose windows | #5 | TODO — bigger refactor; needs `WindowManager` keyed by goose id, not a single screen-shared window. |
| 5 — first-tick skating | #2 | TODO — investigate `rig.lFoot.moveStartTime` init. |
| 6 — mud footprints | #6 | TODO — investigate effect-window-manager spawn path. |
| §4 leafpile partial-bottom | #4 | DEFERRED — math reviewed, looks correct; needs a screenshot to spot the actual clip. |
