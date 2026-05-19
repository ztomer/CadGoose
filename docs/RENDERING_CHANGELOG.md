# Rendering Fixes — Changelog

Completed rendering-regression fixes tracked from `RENDERING_BUGS.md`.
Newest first.

---

## 2026-05-19 — Phase 4+5+6+7

All four remaining rendering regressions fixed. The original
`RENDERING_BUGS.md` table is now empty; the doc keeps the empty-table
template for future regressions to file against.

### Phase 4 — per-goose visibility (#5)
Bug: All geese share a window — only the primary (`geese.front()`) is
visible most of the time.

`WindowManager` was creating one 600×600 `GooseWindow` per screen and
recentering all of them on `geese.front()->pos` every tick. Other
geese — rendered into the same `CGContext` at their true device
coords — were clipped against the 600×600 box and effectively
invisible.

Fix in `src/platform/macos/window.mm`:

- `GooseWindow::initWithScreen:` now creates a window the full size
  of the target screen. The window stays stationary; goose rendering
  happens at device coords inside it.
- `updateWindowPositionsForGeese:` was repositioning the window each
  frame to follow the front goose; replaced with a plain
  `setNeedsDisplay:` to retrigger drawing. No reshaping, no
  recentering.
- `click-through (ignoresMouseEvents = YES)` was already set, so a
  screen-spanning window doesn't capture mouse input.

This is the minimum-churn version; a future refactor can split into
per-goose windows keyed by `Goose::id` if z-order or per-goose
collision becomes needed.

### Phase 5 — first-tick skating (#2)
Bug: Before the first fetch the goose's leg animation visibly lagged
its body, looking like it was skating.

`Goose::Goose` set `rig.{l,r}Foot.currentPos = {0, 0}` and left
`moveDuration` at the struct's default `0.2`. On the first
`SolveFeet` call the (0,0) sentinel triggered a reset to the foot's
home position, but `moveDuration` only became a real walking duration
once the first step *fired*. Combined with the "only one foot in
flight at a time" gate, this stalled the cadence for several seconds
post-spawn.

Fix in `src/common/goose.cpp`: constructor pre-seeds
`rig.lFoot/rFoot.currentPos` to `GetFootHome(...)` and seeds
`moveDuration` to `g_config.step.durationWalk`. The rig is in a
valid walking state from frame 1.

### Phase 6 — mud footprints invisible (#6)
Bug: Footprints were generated but never visible.

Two callsites compared `[[NSDate date] timeIntervalSince1970]`
(wall-clock seconds-since-epoch) against `fp.timeSpawned`, which is
the renderer's monotonic tick counter starting from 0. The
difference was billions of seconds — every footprint's `age >
lifetime` predicate was true on the very first check, so:

- `Footprint_GetPositions()` returned an empty vector.
- `Footprint_ExistsAt()` returned false for every position.
- `EffectWindowManager::syncWindows` never created the windows.
- And when it *did* (after re-spawning), the drawRect alpha
  calculation `1.0 - age/life` clamped to 0, so they were invisible.

Fix:

- `src/common/config.cpp::g_time` (declared in `config.h`, defined
  but never updated) is now updated each frame from
  `renderer.mm::GooseView::tick` to the renderer's `currentTime`.
- `effect_reg_footprint.mm` and the phase-3 redraw block in
  `effect_window.mm::syncWindows` now read `g_time` instead of
  `NSDate`. Producer and consumer share a time base.

### Phase 7 — leafpile bottom-only (#4)
Bug: Autumn leaf piles only showed the bottom half of the pile.

`actor_leafpile.mm` was rendering leaves at `y = -leaf.curPosPlanar.y`
*after* a `Translate(winSize/2, winSize/2)` into a flipped-Y content
view. With `isFlipped=YES` and a center-translate already in place,
the additional negation flipped the leaves around the wrong axis and
kicked the upper half (positive planar Y in flipped space) above the
window's top edge, clipping it.

Also: `curPosZ` (leaf vertical altitude) was computed but never used
in the draw position — kicked leaves never visually rose.

Fix:

- Drop the spurious Y negation: `y = leaf.curPosPlanar.y -
  leaf.curPosZ * 0.6f`. Leaves now sit symmetrically around the
  pile center; kicked leaves rise as they should.
- Enlarge the window height to accommodate `m_height * 0.6` of
  vertical lift, so leaves don't clip against the top edge when
  kicked.

### Verified at the time

Clean build; 716 tests pass.

---

## 2026-05-19 — Phase 1+2+3 — `08d1b26`

Text Y-flip, effect-window clipping, pomodoro bed dims. 7 of the 11
reported bugs.

### Phase 1 — text Y-flip
Bugs: **#1 nametag upside down**, **#11d pomodoro timer upside down**.

`CGRenderer::DrawText` now saves the text matrix, applies
`CGAffineTransformMakeScale(1.0, -1.0)`, draws, restores. The goose
view's CTM has positive-Y going down; without the inverted text
matrix CoreText drew glyphs upside-down. Callers no longer need to
know about the flip.

File: `include/cg_renderer.h`.

### Phase 2 — effect-window rect clipping
Bugs: **#3 portal cut off**, **#7 breadcrumbs cut off**, **#10 jail
not visible**, **#8 hats not visible**.

Root cause: `Translate(-halfW, -halfH); DrawAt(0, 0, w, h)` pattern
drew sprites starting above and left of the `BehaviorElementWindow`'s
flipped-Y origin, so they were clipped against the window's top-left
edges.

Fixes:

- `src/common/actor_portal.mm` — replaced negative-half translate
  with `DrawImage` to a centered destination rect inside
  `[0..winSize, 0..winSize]`. Aspect-preserved against the larger of
  `(w, h)` so non-square portal art stays centered.
- `src/common/actor_breadcrumb.mm` — same fix; aspect-preserved
  centered draw.
- `src/common/actor_jail.mm` — cage outline and bars previously
  drawn in `[-half, half]` coords; rewritten in window-local
  `[0..winSize]`.
- `src/common/behaviors/behavior_hats.cpp` — was reading
  `goose->rig.neckHead` directly (goose-local rig space, not device
  space) and double-flipping with `Translate(-halfW, halfH); Scale(1, -1)`.
  Switched to `WorldCoord::RigNeckHead(*goose)` for device coords
  and a single centered `DrawImage`; the facing-left mirror now
  uses an explicit `Translate(headX, 0); Scale(-1, 1)` around the
  hat's vertical axis.

### Phase 3 — pomodoro bed dimensions
Bugs: **#11a bed lands at top-left**, **#11b never sleeps**,
**#11c bed not visible**.

`init()` was reading `g_world.screenWidth/Height` while they were
still 0 — the renderer assigns them on first tick. Bed anchor ended
up at `(-150, -100)` clipped to the origin, and the proximity check
that triggers sleep never reached it.

Centralized into `ComputeBedPosition()`; `tick()` recomputes from
current screen dims and stores it if changed, so the bed catches up
the moment dims become non-zero. Also handles future screen-resize
events.

File: `src/common/behaviors/behavior_pomodoro.cpp`.

### Verified at the time

Clean build; 716 tests pass; no flaky regressions.
