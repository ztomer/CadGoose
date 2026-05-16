# CadGoose Bug & Feature Plan

## Completed (May 15, 2026)

### 1. Toys NO close button ✅
- Skip close button rendering/hit-test for `ItemData::TOY`
- Files: `goose_drawing.mm`, `renderer.mm:mouseDown:`

### 2. Ball toy rendering ✅
- Radius `STICK_WIDTH=4px` → `BALL_RADIUS=15px`
- File: `behavior_toys.cpp`

### 3. Sonic blue screen ✅
- `goose->pos` is device coords, `ToDevice()` double-scaled → removed call
- File: `behavior_sonic.cpp`

### 4. Dragging regression ✅
- Root cause: transparent borderless window with `canBecomeKeyWindow=NO` → responder chain broken
- Fix: `NSEvent addLocalMonitorForEvents` intercepts mouse events at app level, dispatches directly to `mouseDownAtPoint:viewY:` and `mouseDraggedAtPoint:viewY:`
- Also restored accidentally removed `isFlipped` (goose was upside down)
- Files: `renderer.mm` (monitors + direct handlers), `window.mm` (reverted to NO)

### 5. Config for memes/texts lifecycle ✅
- Added: `meme_lifetime`, `text_lifetime`, `max_dropped_memes`, `max_dropped_texts`
- `World_CleanupExpired()` enforces per-type limits with oldest-first eviction
- Files: `config.h`, `config_registry_general.cpp`, `world_utils.mm`

### 6. Architecture reflection ✅
- File: `docs/ARCHITECTURE_REFLECTION.md`
- 7 key findings, prioritized refactoring order

### 7. Linux CI build fix ✅
- `gtk4-layer-shell-0` required but not installed on CI runner, source code doesn't use it
- Made optional: `pkg_check_modules(LS gtk4-layer-shell-0)` (no REQUIRED), empty fallbacks
- Fixed `sysctl -n hw.logicalcpu` → `nproc` in Linux CI workflow (macOS-only command)
- Tests disabled on Linux — behaviors have `CoreGraphics` dependencies (macOS-only)
- Test sources split into `TEST_SOURCES_COMMON` + `TEST_SOURCES_PLATFORM` for clean cross-platform builds
- Files: `CMakeLists.txt`, `.github/workflows/pr_check.yml`

### 8. Remove Sonic Mode ✅
- Deleted `behavior_sonic.cpp`, removed from CMakeLists.txt
- Removed `SonicState`/`SonicTrail` structs from `behavior.h`
- Removed `sonicMode` field from `config.h` and registry
- Removed GUI row, emoji, and all test references (6 files)
- Removed from README.md, config.toml, AGENTS.md
- Fixed `BEHAVIOR_DEF_CUSTOM` macro field order (reorder warning)
- Suppressed toml11 vendor warning via `-Wno-deprecated-literal-operator`
- Fixed test state leak: `ToysEnabledLoadFromToml` didn't restore `toysEnabled`

## In Progress

### 9. Coordinate system audit ✅
**Problem**: Three coordinate spaces (world/device/screen) with ambiguous boundaries. `goose->pos` is device coords but `ToDevice()` implied world coords. Caused bugs in sonic, pomodoro, toys, snatch, and footprints.

**Fixes applied**:
- Added coordinate space documentation to `world.h` and `goose_math.h` (DEVICE vs GOOSE-LOCAL WORLD)
- Renamed `ToDevice(worldPos, goose)` → `WorldToDevice(worldPos, goose)` (explicit: goose-local world → device)
- Renamed `ToDevice(worldPos)` → `OriginToDevice(worldPos)` (explicit: origin-relative world → device)
- Removed `GoosePos()` — was a no-op (`goose.pos + (goose.pos - goose.pos) * scale = goose.pos`)
- **Pomodoro bed render**: removed `ToDevice(bedPosition)` — bedPosition is already device coords
- **Snatch anchor**: removed `ToDevice(snatchAnchor, *this)` — snatchAnchor is device coords (from `goose->pos`)
- **Footprints**: removed `ToDevice(home, *this)` — `GetFootHome()` returns device coords
- **Toys**: converted from world coords to device coords — spawn uses `g_screenWidth` directly, no `/ globalScale`, no `ToDevice` in render
- **Linux callback**: removed non-existent `DeviceToWorld` call — cursor position is already device coords
- Removed `GoosePosIsIdentityAtUnitScale` test (was testing a no-op)

**Files touched**: `include/world.h`, `include/goose.h`, `include/goose_math.h`, `src/common/behaviors/behavior_toys.cpp`, `src/common/behaviors/behavior_pomodoro.cpp`, `src/common/goose.cpp`, `src/common/goose_behaviors_interact.cpp`, `src/common/goose_behaviors_wander.cpp`, `src/common/behaviors/behavior_peeking.cpp`, `src/platform/linux/ui_callbacks.cpp`, `tests/test_goose_behaviors.cpp`

**672 tests pass** (702 total, 30 AX skipped).

### 10. Dragging integration test ✅
- Expanded from 3 tests (2 pass, 1 skip) to 8 tests (7 pass, 1 skip)
- New tests: `DragOffsetCalculation`, `DragPositionUpdate`, `SimulatedFullDragCycle`, `DragFromCorner`, `DragWithRotation`
- Tests replicate `mouseDownAtPoint:viewY:` and `mouseDraggedAtPoint:viewY:` logic directly
- `CGEventSynthesis` skipped (requires active GUI session + Accessibility permissions)
- File: `tests/platform/macos/test_dragging_integration.mm`

### 11. Coordinate system deep dive ✅
**Deep dive findings** (traced 100+ coordinate usages across 30+ files):

**Bugs fixed:**
1. **ItemCenter/ItemHalfSize Y-axis bug** — `DeviceSize(item->w)` used width for both X and Y. Added `ItemSize(item)` that uses `item->w` and `item->h` correctly. Affected heist approach, pickup detection, drop positioning for non-square items.
2. **ClampToScreen Y-axis inconsistency** — Y-max used `screenClampBounce` instead of `snapDistance` for position snap. Fixed to match X-axis behavior.
3. **Linux debug overlay syntax errors** — `g.state = GooseState::= GooseState:: FETCHING` (assignment + malformed). Fixed to `g.state == GooseState::FETCHING` (5 occurrences).

**Verified correct:**
- All goose position/target/velocity/acceleration usage in DEVICE coords
- All cursor backend readings return DEVICE coords (macOS CGEvent, Linux X11/Wayland)
- All mouse event coordinate transforms (NSEvent → window → view → device)
- All rendering coordinate usage (macOS CGContext, Linux Cairo with Y-flip)
- All distance/comparison checks use consistent coordinate spaces
- Ball behavior correctly converts between DEVICE and WORLD coords
- Portal, breadcrumbs, drag, jail all use DEVICE coords consistently
- Multi-monitor edge avoidance and clamping respect `multiMonitorEnabled`

**Minor issues noted (not bugs):**
- Ball/breadcrumbs/portal bypass `g_cursorProvider` and use backend directly — works but inconsistent
- macOS `g_screenWidth/Height` only captures main screen — known limitation
- `rig.*` fields are in hybrid space (device pos + unscaled offsets) — rendering correct via `WorldCoord::RigNeckHead`

**677 tests pass** (707 total, 30 AX skipped).

### 12. Coordinate system consolidation ✅
**Created typed coordinate system to prevent coordinate space bugs:**

**New file: `include/coordinate_system.h`**
- `DevicePoint` — screen pixels, top-left origin, Y-down (goose pos, items, cursor)
- `WorldPoint` — goose-local unscaled coords (rig parts, config offsets)
- `ScreenPoint` — raw OS screen coordinates (platform boundary)
- `ViewPoint` — NSView-local coordinates (macOS event handling)
- `CoordTransform` — explicit transforms between spaces (no implicit conversions)
- `ItemCoords` — item center/half-size/size helpers (correctly uses w AND h)
- `HitTest` — point-in-item and close-button hit testing
- `ScreenBounds` — screen boundary clamping with consistent snap distance

**Refactored files:**
- `world.h` — `WorldCoord` methods now return `DevicePoint`, uses `ItemCoords` and `CoordTransform`
- `goose.h` — includes `coordinate_system.h`, documents coordinate spaces
- `goose_math.h` — simplified docs, references coordinate_system.h
- `renderer.mm` — hit test and drag handlers use `DevicePoint`, `HitTest`, `CoordTransform`
- `world_utils.mm` — `ItemHitTest` uses `HitTest::PointInItem`
- `goose_behaviors_wander.cpp` — uses `ItemSize` instead of `DeviceSize`

**Key design principles:**
- No implicit conversions between coordinate spaces
- All transforms must be explicit and named
- Type system prevents accidental mixing of DEVICE and WORLD
- Clear documentation at every platform boundary
- Single source of truth for hit testing logic

**677 tests pass** (707 total, 30 AX skipped).

### 13. Coordinate exception fixes ✅
**Fixed all 11 coordinate space exceptions found in deep dive:**

| # | File | Severity | Fix |
|---|------|----------|-----|
| 1 | `goose_drawing.mm:DrawHeldItem()` | HIGH | Scale `heldItem->w/h` by `globalScale` in all CGContext calls |
| 2 | `goose_drawing.mm:DrawDroppedItem()` | HIGH | Scale `item.data->w/h` by `globalScale` in all CGContext calls |
| 3 | `goose_behaviors_interact.cpp:isTargetReached()` | HIGH | Remove double-scale on `g.vel` (already device coords) |
| 4 | `ui_drawing.cpp:draw_dropped_item()` | HIGH | Scale `item.data->w/h` by `globalScale` in Cairo surface position |
| 5 | `goose.cpp:GetFootHome()` | MEDIUM | Scale `footSpacing` by `globalScale` |
| 6 | `behavior_interactive_drops.cpp:render()` | MEDIUM | Scale `stemHeight`, `petalSize`, line width by `globalScale` |
| 7 | `behavior_breadcrumbs.cpp:render()` | LOW | Scale crumb image dimensions by `globalScale` |
| 8 | `behavior_pomodoro.cpp:render()` | LOW | Scale bed image, ZZZ offsets, padding by `globalScale` |
| 9 | `behavior_portal.cpp:render()` | LOW | Scale portal image dimensions by `globalScale` |
| 10 | `goose_behaviors_interact.cpp:dodge` | LOW | Scale hardcoded `400.0f` dodge distance by `globalScale` |

**677 tests pass** (707 total, 30 AX skipped).

### 14. Unified rendering API ✅
**Eliminated manual scaling by applying `CGContextScaleCTM` at behavior dispatch level:**

**Before:** Two different rendering approaches
- Main goose drawing: `CGContextScaleCTM` transform → raw pixel values auto-scale
- Behavior rendering: manual `* globalScale` on every pixel value

**After:** Single unified approach
- `renderer.mm:drawRect:` applies scale transform around `goose->pos` before calling `RenderPass`
- All behaviors use raw pixel values (no manual scaling)
- Goose-relative positions use raw rig coords (`goose->rig.neckHead`)
- Global device coords (toys, ball, breadcrumbs, portals, jails, flowers, bed) converted to transform space: `drawPos = goose->pos + (devicePos - goose->pos) / scale`

**Files changed:**
- `renderer.mm` — scale transform applied before `RenderPass` (ground and non-ground)
- `behavior_pomodoro.cpp` — raw rig coords, raw pixel values
- `behavior_honcker.cpp` — raw rig coords, raw pixel values
- `behavior_peeking.cpp` — raw rig coords, raw pixel values
- `behavior_nametag.cpp` — raw rig coords, raw pixel values
- `behavior_boredom.cpp` — raw rig coords, raw pixel values
- `behavior_hats.cpp` — raw rig coords, raw pixel values
- `behavior_toys.cpp` — global device coords converted to transform space
- `behavior_ball.cpp` — global device coords converted to transform space
- `behavior_breadcrumbs.cpp` — global device coords converted to transform space
- `behavior_portal.cpp` — global device coords converted to transform space
- `behavior_jail.cpp` — global device coords converted to transform space
- `behavior_interactive_drops.cpp` — global device coords converted to transform space
- `behavior_anger.cpp` — raw pixel values (uses `goose->pos` which is transform center)
- `behavior_health.cpp` — raw pixel values (uses `goose->pos` which is transform center)

**Key design:**
- Transform center is `goose->pos` (device coords)
- Rig parts are in world coords (unscaled offsets from goose)
- Global objects convert device → transform space at render time
- All hardcoded pixel values are raw and auto-scale via context transform

**677 tests pass** (707 total, 30 AX skipped).

## Next Up

### 15. Architectural Retrospective & Refactoring Plan

**Retrospective findings (May 15, 2026):**

#### What Works Well
- **Typed coordinate system** — `DevicePoint`, `WorldPoint` etc. are zero-cost, compile-time safe
- **Behavior macros** — `BEHAVIOR_DEF*` family enforces `enabledPtr == configPtr`, preventing toggle desync
- **Unified rendering pipeline** — `CGContextScaleCTM` at dispatch level, all behaviors use raw pixels
- **500 LOC discipline** — forces modularization
- **Per-goose state management** — `BehaviorStateManager` with composite key
- **717 tests** — 687 pass, 30 AX skipped

#### Anti-Patterns Found
| Pattern | Where | Impact |
|---------|-------|--------|
| Static globals in behaviors | `behavior_ball.cpp` (8 statics), 38 total across files | Breaks multi-goose, state leaks on respawn |
| `void*` render context casting | Every behavior `render()` | No type safety |
| Duplicate mouse handlers | `renderer.mm` (2 paths, ~90% duplicated) | Maintenance burden, double bug fixes |
| `g_config` global struct | Everywhere | No dependency tracking, any file can read/write anything |
| Mixed responsibility in `goose.cpp` | 496 lines | State machine + physics + rig + stuck + drag + debug |
| `conflicts` field never populated | `behavior.h` | Pomodoro + Drag can fight over `goose->vel` |

#### Missing Abstractions
1. **Event bus** — Behaviors directly mutate `Goose` state, no decoupled signaling
2. **Timer class** — Every behavior computes deltas manually from `ctx.time`
3. **Resource manager** — `CGImageRef`/`CGFontRef` in static globals, no cleanup
4. **Input abstraction** — Inconsistent cursor access patterns

#### Testing Gaps
- Linux platform code: zero tests
- Headless rendering: no offscreen pixel tests
- 30 AX tests skipped (require running app)
- No integration tests (end-to-end flows)
- Config GUI panels untested (712 lines total)

---

### Implementation Plan

#### P0: Migrate ball state from statics to BallState ✅
**Problem**: `behavior_ball.cpp` had 8 static globals that bypassed `BallState`. Multi-goose broken, state didn't reset on respawn.

**Done**:
- Added physics fields to `BallState::Ball` struct (`speed`, `lastKickTime`, `lastAnimateTime`, `animationGap`)
- Replaced all static globals with `GetOrCreate<BallState>()`
- Kept shared resources (`s_ballImages`) as static (acceptable — CGImageRef assets, not state)
- Converted all coordinate math to use device coords consistently
- Extracted magic numbers to named constants

**Files**: `include/behavior.h`, `src/common/behaviors/behavior_ball.cpp`

#### P1: Extract shared mouse handler logic ✅
**Problem**: `renderer.mm` had two nearly identical hit-test + drag paths (~90% duplicated).

**Done**:
- Created `ItemDragController` class with `OnMouseDown`, `OnMouseDragged`, `OnMouseUp`
- Both handler paths (AppKit responder + NSEvent monitor) delegate to controller
- Controller takes `DevicePoint` as input (coordinate-agnostic)
- Added `dealloc` to clean up controller
- Reduced `renderer.mm` by ~60 lines of duplicated code

**Files**: `include/item_drag_controller.h`, `src/platform/macos/item_drag_controller.mm`, `src/platform/macos/renderer.mm`

#### P2: Add Timer utility class ✅
**Problem**: Every behavior computes deltas manually: `time - state->lastDropTime`, `ctx.time - state->phaseStartTime`, etc.

**Done**:
- Created `Timer` struct with `Start(time)`, `Elapsed(time)`, `IsExpired(time, duration)`, `Reset()`, `Remaining(time, duration)`, `Progress(time, duration)`
- 10 unit tests covering all methods
- Header-only, zero overhead, no dependencies

**Files**: `include/timer.h`, `tests/common/test_timer.cpp`

#### P3: Add headless rendering tests ✅
**Problem**: UI regressions (dragging Y-flip bug, coordinate scaling issues) not caught by CI. 30 AX tests skipped.

**Done**:
- Created `test_headless_rendering.mm` with 22 tests covering:
  - HitTest at different scales (0.5x, 1.0x, 2.0x) + rotation
  - Close button hit-test for non-toy items (at scale, rotated)
  - CoordTransform::WorldToDevice/DeviceToWorld at different scales + round-trip
  - ViewToDevice transform
  - ItemCoords::Center/HalfSize/Size at different scales
  - ScreenBounds::Clamp/Contains with margins
  - ItemDragController: hit/drag/release, miss, close button deletes, toy no close button, drag offset preserved, multiple items (topmost wins)
- All 22 tests pass, no AppKit dependencies, runs in CI

**Files**: `tests/platform/macos/test_headless_rendering.mm`

---

## Test Status
- **739 tests, 99 suites** — 709 pass, 30 AX skipped (require running app)
- New suites: `DraggingIntegration` (8 tests, 1 skipped), `Timer` (10 tests, all pass), `HeadlessRendering` (22 tests, all pass)
