# Bug & Feature Investigation Plan

## Issues

### 1. Toys should NOT have a close button
**Status**: Not started  
**Description**: Dropped toy items (sticks/balls) currently show the close button (X) in the bottom-left corner like memes/texts. Toys should not have this — they're meant to be picked up by the goose, not dismissed by the user.  
**Files**: `src/common/goose_drawing.mm:DrawDroppedItem()`, `src/platform/macos/renderer.mm:hitTest:`, `mouseDown:`  
**Approach**: `DrawDroppedItem()` unconditionally draws the close button for all `DroppedItem`s. Need to skip it for `ItemData::TOY` type. Also need to skip close-button hit-test in `hitTest:` and `mouseDown:` for toy items.

### 2. Only sticks render for toys
**Status**: Not started  
**Description**: Ball toys (`Toy::Type::Ball`) are not visible on screen — only sticks appear.  
**Files**: `src/common/behaviors/behavior_toys.cpp:tick()` (spawn), `src/common/behaviors/behavior_toys.cpp:render()`  
**Approach**: 
- Check spawn logic: `rand() % 2` determines type — both Stick and Ball should spawn equally
- Check render logic: Ball rendering uses `STICK_WIDTH * ctx.globalScale` as radius — that's `4.0 * scale` which is very small (~4-8px). Compare with stick rendering which uses `STICK_LENGTH=24` × `STICK_WIDTH=4`
- Ball radius should be larger (e.g., 12-15px) to be visible

### 3. Dragging meme images and text doesn't work
**Status**: Fixed  
**Description**: Click-and-drag on dropped memes/texts doesn't move them.  
**Files**: `src/platform/macos/renderer.mm:hitTest:`, `mouseDown:`, `mouseDragged:`, `tick:`  
**Root cause**: `item.pos` is in **device coordinates** (top-left origin, Y-down, matching the `isFlipped=YES` view). But the hit-test code did `dy = (height - p.y) - item.pos.y` which **double-flips** Y, putting mouse and item in different coordinate spaces. The mouse was over the item but the hit-test computed a large `dy` offset and always missed.  
**Fix**: Removed Y-flip in all four hit-test locations (`tick:`, `hitTest:`, `mouseDown:`, `mouseDragged:`). Now `dx = p.x - item.pos.x`, `dy = p.y - item.pos.y` — same coordinate space.  
**Also fixed**: `world_utils.mm:ItemHitTest()` had the same bug. Updated `test_dropped_item_hit.cpp` to match the corrected model.

**Config additions needed** (memes/images only, separate from toys):
- `max_dropped_memes` (int, default 20) — auto-delete oldest when exceeded
- `max_dropped_texts` (int, default 20) — auto-delete oldest when exceeded
- `meme_lifetime` (float, default 30s) — already exists as `g_config.item.itemLifetime`
- `text_lifetime` (float, default 30s)

### 4. Sonic mode - entire screen turns blue
**Status**: Fixed  
**Description**: Instead of a trail behind the goose, the entire screen fills with blue circles.  
**Files**: `src/common/behaviors/behavior_sonic.cpp:render()`  
**Root cause**: `goose->pos` is in **device coordinates** (screen pixels), initialized from `screenW/screenH`. Sonic stores `trail.pos = goose->pos` (already device coords), then `ToDevice(trail.pos)` double-scales it by `globalScale` → positions become huge, filling the screen.  
**Fix**: Use `trail.pos` directly without `ToDevice()` since it's already in device coordinates.

### 5. Architectural reflection: what would we do differently?
**Status**: Documented  
**File**: `docs/ARCHITECTURE_REFLECTION.md`  
**Key findings**:
1. **Coordinate system** — three spaces (world/device/screen) with ambiguous boundaries caused sonic blue screen, toy position bugs, and dragging Y-flip regression
2. **Item system** — type creep (`MEME` → `MEME/TEXT/TOY`) without unified rendering strategy
3. **Config system** — four layers of indirection (TOML → registry → struct → GUI)
4. **Test coverage** — 29 AX tests skipped, UI regressions not caught by CI
5. **Platform split** — macOS vs Linux code paths diverge significantly

---

## Execution Order

| # | Issue | Dependencies | Est. Complexity | Status |
|---|-------|-------------|-----------------|--------|
| 1 | #2 Toys ball rendering | None | Low (1-line fix) | ✅ Done |
| 2 | #1 Toys NO close button | None | Low (skip for TOY type) | ✅ Done |
| 3 | #4 Sonic screen-blue | None | Low (use trail.pos directly) | ✅ Done |
| 4 | #3 Dragging Y-flip bug | None | Low (remove double-flip) | ✅ Done |
| 5 | #3 Config additions | None | Low | ✅ Done |
| 6 | #5 Architecture reflection | None | Medium (analysis) | ✅ Done |

**Rationale**: Fix the trivial visual bugs first (#2 ball size, #1 close button verify). Then tackle sonic mode (#4) since it's a single behavior file. Then the dragging investigation (#3) which requires systematic logging. Config additions are quick. Architecture reflection is independent and can be done anytime.

---

## Clarifying Questions (Answered)

1. **Toys close button**: Toys should NOT have a close button. ✅ Fixed — skipped for `ItemData::TOY`.
2. **Sonic mode**: Blue covers entire screen immediately on enabling. ✅ Fixed — `goose->pos` is device coords, removed `ToDevice()` double-scaling.
3. **Dragging**: Nothing happens when clicking. ✅ Fixed — Y-flip was applied twice in hit-test.
4. **Config for items**: Separate lifecycle for memes/texts (not toys). ✅ Done — `meme_lifetime`, `text_lifetime`, `max_dropped_memes`, `max_dropped_texts`.
5. **Architecture reflection**: Documentation first, implementation later. ✅ Done — `docs/ARCHITECTURE_REFLECTION.md`.
