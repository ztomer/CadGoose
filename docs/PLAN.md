# Plan — Per-Goose Window Migration

## Motivation

The full-screen transparent overlay (`GooseView`/`GooseWindow`) consumes ~1GB physical memory on a 5K display (50MB IOSurface × CoreAnimation compositing overhead). Every other entity already uses small per-actor windows (BallActor, ToyActor, FlowerActor, etc. — each ~32-600px). Migrating the goose to its own small window eliminates the overlay entirely.

## Architecture

### Current Rendering Pipeline (full-screen overlay)

```
drawRect: (GooseView, full-screen, ~3360×1890)
  └─ CGContextTranslateCTM(-viewOriginDevice)  // device coords → view coords
  └─ ActorManager::renderAll(&renderer)
       └─ Goose::render(renderer)      ← draws goose body + behaviors into ctx at device coords
       └─ BallActor::render(renderer)  ← creates/updates own window (ignores renderer)
       └─ ToyActor::render(renderer)   ← creates/updates own window (ignores renderer)
       └─ ... (all other actors ignore renderer)
```

- `CADisplayLink` lives in `GooseView` (`renderer.mm:108-132`)
- `keyDown:` captured via AppKit responder chain on `GooseView`
- `GooseWindow` per screen (multi-monitor), all same size, all full-screen
- Device coordinates = view coordinates after `-viewOriginDevice` translation

### Target Pipeline (per-actor-window)

```
CADisplayLink (TickManager, standalone)
  └─ ActorManager::tickAll()
  └─ ActorManager::cleanup()
  └─ World_CleanupExpired()
  └─ EffectWindowManager::syncWindows
  └─ ItemWindowManager::syncWindows
  └─ TickManager::updateGooseWindows() for each goose:
       ├─ set frame origin = (goose->pos - windowSize/2)
       └─ setNeedsDisplay:YES on goose window

GooseWindow::drawRect: (per-goose, ~600×600)
  └─ CGContextTranslateCTM(-originDevice)  // device coords → window-local
  └─ Goose::render(&renderer)               ← draws goose at device coords (centered in window)

ActorManager::renderAll() (called from TickManager, NOT from drawRect)
  └─ BallActor::render()  ← creates/updates own window (unchanged)
  └─ ToyActor::render()   ← creates/updates own window (unchanged)
  └─ ... etc (unchanged)
```

### Coordinate Transform

Per-goose window is `windowSize × windowSize`, centered on `goose->pos`:

```
Window origin (device):  goose->pos - windowSize/2
DrawRect transform:      CGContextTranslateCTM(ctx, -origin.x, -origin.y)
```

After transform: device coord `(x, y)` maps to window coord `(x - origin.x, y - origin.y)`. Goose at `goose->pos` draws at `(windowSize/2, windowSize/2)` — center of window. All existing drawing code (goose body, behaviors, held item, nametag, hats, etc.) works unchanged since it uses device-relative coordinates.

### Window Size

`CalculateGooseWindowSize()` exists in `window.mm:35-63` — computes bounding box for goose body + held item + rotation padding. Default: 600px. Called when held item changes.

### TickManager (new)

Standalone class owning `CADisplayLink`. Contains tick logic extracted from `GooseView::tick:`. Methods:
- `+[TickManager shared]` — singleton
- `-start`, `-stop` — display link lifecycle
- `-onFrameRefresh:` — tick handler (moved from `GooseView::tick:`)

After cutover, TickManager also owns per-goose window position updates.

### Key Events

Currently `keyDown:` on `GooseView` handles 'f' for honk. Migrate to global `NSEvent` monitor (`addGlobalMonitorForEventsMatchingMask:`) so key events work without GooseView as responder.

---

## Phases

### Phase 0: Tick Extraction (A1 — additive)

**Goal**: Extract `CADisplayLink` + tick logic from `GooseView` into standalone `TickManager`. Full-screen overlay unchanged (parity).

**New files**:
- `include/tick_manager.h`
- `src/platform/macos/tick_manager.mm`

**TickManager**:
- Singleton with `start()`/`stop()`/`isRunning()`
- Owns `CADisplayLink` (same pattern as GooseView: `displayLinkWithTarget:selector:`)
- `onFrameRefresh:` calls tick logic verbatim from GooseView (actor tick, cleanup, leaves, window sync, window position update, setNeedsDisplay on full-screen view)
- `-tick` method repeats the current `GooseView::tick:` implementation

**Modifications to existing files**:
- `renderer.mm`: `GooseView::tick:` and `startAnimation`/`stopAnimation` become thin wrappers forwarding to TickManager. Remove `CADisplayLink` from GooseView.
- `window.mm`: `createWindowsForAllScreens` starts TickManager after window creation. Or, TickManager starts on app launch separately.
- `renderer.mm`: `GooseView::drawRect:` unchanged — still renders full-screen overlay.

**Verification**:
- All 741 passing tests still pass
- Trail test: 6/6 cycles visible
- No visual regression
- PROOF: `ActorManager::renderAll()` called from same place (drawRect:), same CGRenderer, same tick state

### Phase 1: GooseActorWindow Additive (A1 — additive, A3 — dual-write)

**Goal**: Create per-goose windows alongside full-screen overlay. Both render the same goose — dual-write for parity validation.

**New files**:
- `include/goose_actor_window.h` (ObjC++ header)
- `src/platform/macos/goose_actor_window.mm`

**GooseActorWindow : NSWindow**:
- Borderless, transparent, `ignoresMouseEvents=YES`, `releasedWhenClosed=NO`
- Level: `NSStatusWindowLevel + 2` (above full-screen overlay at +1)
- `canBecomeKeyWindow=NO`, `canBecomeMainWindow=NO`
- Size: 600×600 initially, resized via `CalculateGooseWindowSize`

**GooseActorView : NSView**:
- `wantsLayer=YES`, `isFlipped=YES`
- `drawRect:`:
  ```
  float size = self.bounds.size.width;  // square window
  float centerOff = size / 2;
  float originX = goose->pos.x - centerOff;
  float originY = goose->pos.y - centerOff;
  CGContextRef ctx = ...;
  CGContextClearRect(ctx, self.bounds);
  CGContextTranslateCTM(ctx, -originX, -originY);
  CGRenderer renderer(ctx);
  goose->render(&renderer);
  ```

**Modifications to Goose**:
- Add `g_gooseWindow` member (opaque `void*` pointer to `GooseActorWindow*`)
- Add `g_windowSize` member (cached window size, avoids recalc every frame)
- Modify `Goose::render()`: after drawing into the full-screen context (existing behavior), ALSO create/update the per-goose window:
  - Create window on first call
  - Update position based on `pos`
  - Update size via `CalculateGooseWindowSize(this)` when heldItem changes
  - Mark window dirty (`[view setNeedsDisplay:YES]`)

**Note**: Goose::render() is called from the full-screen view's drawRect. Creating/releasing ObjC windows during drawRect is safe (main thread + AppKit context). Same pattern as BallActor::render() which creates BehaviorElementWindow during renderAll.

**Verification**:
- User observes goose rendered twice (one full-screen, one per-goose window above it) — visually identical
- Remove full-screen goose (disable `drawRect:`) temporarily for A/B comparison
- Held item visible in per-goose window (critical for trail test)
- PROOF: Trail test captures both windows composited — held item visibility must still be 6/6

### Phase 2: Cutover (A4 — foundation vs cutover) [COMPLETED]

**Goal**: Full-screen overlay stops drawing the goose. Per-goose windows are sole rendering path.

**Changes made**:
- Split `Goose::render()` into `render()` (full-screen path) and `draw()` (per-goose window path)
- `Goose::render()` returns early when `g_cutoverMode` is true (no-op in full-screen pass)
- `Goose::draw()` contains the actual drawing body — always executes
- Added `g_cutoverMode` global (defined in `config.cpp`, declared in `config.h`), default `true` from Phase 2
- Per-goose window draw blocks call `goose->draw(&r)` instead of `goose->render(&r)`
- `ActorManager::renderAll()` still called in drawRect: for other actors (BallActor, ToyActor, etc.)
- Added `Render_NoopInCutoverMode` unit test verifying `render()` is no-op

**Verification**:
- [x] No goose visible in full-screen overlay
- [x] Goose visible in per-goose window at correct position, correct size
- [x] 18 GooseRender tests pass (including Render_NoopInCutoverMode)
- [x] 391 key tests pass
- [x] Build succeeds with zero warnings

### Phase 3: Cleanup [COMPLETED]

**Removed dead code**:
- `renderer.mm`: `drawRect:` removed, entire `GooseView` class removed, `GooseView @interface` removed from `renderer.h`
- `window.h`/`window.mm`: `GooseWindow` class removed, `updateWindowPositionsForGeese` removed, `WindowManager` is now a stub (no full-screen windows)
- `main.mm`: No longer creates full-screen windows or accesses GooseView; starts TickManager directly
- `tick_manager.h`/`.mm`: `setPrimaryView:` removed, `primaryView` property removed, display link uses `[NSScreen mainScreen]` directly
- `Presence_SetGooseWindowVisible`: Now toggles per-goose BehaviorElementWindows instead of full-screen GooseWindows
- `test_renderer.mm`: 7 GooseView-specific tests removed (class no longer exists)

**Migration to TickManager**:
- `ActorManager::renderAll(nullptr)` called from TickManager::tick instead of drawRect:
- Per-goose window creation/update loop moved from drawRect: to TickManager::tick
- `[[ItemWindowManager shared] showPendingWindows]` moved to tick
- Global NSEvent key monitor added to TickManager for 'f' honk key (Phase 4 ride-along)

**Result**:
- No full-screen backing store (~50MB IOSurface saved)
- No full-screen compositing overhead (~1GB virtual memory saved)
- Every entity uses small per-actor windows
- Total memory expected to drop from ~985MB to ~150-200MB

**Verification**:
- [x] Build succeeds with zero warnings (both targets)
- [x] 385 tests pass (GooseView-dependent tests intentionally removed)
- [x] 4 Rendering tests pass (text rendering, Y-axis flip)

### Phase 4: Key Event Migration (ride-along with any phase)

**Problem**: Removing `GooseView` removes the responder chain for key events. Need alternate path.

**Solution**: Global NSEvent monitor:
```objc
[NSEvent addGlobalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                      handler:^(NSEvent* event) {
    unichar key = [[event characters] characterAtIndex:0];
    if (key == 'f' || key == 'F') {
        for (auto* g : ActorManager::Instance().getGeese()) {
            Honcker_Honk(g, g_time);
        }
    }
}];
```

Available anytime — can be added in Phase 0 or Phase 3.

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Per-goose window draws at wrong position | Medium | High — goose invisible | Unit test: verify transform maps device→window center correctly. Visual A/B with full-screen. |
| Held item clipped by small window | Low | Medium — item invisible | `CalculateGooseWindowSize` accounts for rotation + padding. Increase padding if needed. |
| Behavior rendering breaks (health, anger, etc.) | Low | Medium — visual artifacts | Same CGRenderer, same transform context. Test each behavior visually. |
| Tick extraction breaks frame timing | Low | High — desync or crashes | Same CADisplayLink, same code path. Paranoia: test all behaviors work. |
| Multi-monitor regressions | Low | Low — per-goose windows are per-goose, not per-monitor | One window per goose, positioned at goose position. No monitor-awareness needed. |
| Key events lost after GooseView removal | Medium | Low — 'f' honk breaks | Phase 4 adds NSEvent global monitor. Can be done in Phase 0. |

---

## Future: Pluggable Character Skins

**Goal**: Allow the goose to be replaced with alternative character skins (e.g., "Baby Stalin" with stalin mustache, stalin eyebrows, and stalin cap). The character system should be pluggable so any actor type can use any skin.

### Design

**Layer 1: Renderer Skins**
- `CharacterSkin` abstract interface (pure virtual):
  - `DrawBody(IRenderer&, const Goose&)`
  - `DrawHead(IRenderer&, const Goose&)`
  - `DrawBeak(IRenderer&, const Goose&)` — could become nose/mouth
  - `DrawWings(IRenderer&, const Goose&)` — could become arms
  - `DrawFeet(IRenderer&, const Goose&)`
  - `DrawHat(IRenderer&, const Goose&)` — Stalin cap
  - `DrawFacialHair(IRenderer&, const Goose&)` — stalin mustache
  - `DrawEyebrows(IRenderer&, const Goose&)` — stalin eyebrows
- All have default implementations that draw the standard goose
- `Goose::draw()` delegates to `m_skin->DrawBody(...)`, etc.

**Layer 2: Skin Registry**
- `SkinRegistry` singleton maps string key → `CharacterSkin` factory
- `"goose"` → renders classic goose (same as today)
- `"baby_stalin"` → baby body + stalin facial features + military cap
- Selected via config: `g_config.render.characterSkin = "baby_stalin"`
- Hot-swappable at runtime via MCP/command socket: `set_skin baby_stalin`

**Layer 3: Baby Stalin Skin**
- Baby-sized body (smaller, rounder, no wings — just arms)
- Stalin mustache: thick dark brush under nose
- Stalin eyebrows: heavy dark unibrow/eyebrows
- Stalin cap: olive/khaki military cap (ushanka or peaked cap)
- Expressions: angry/neutral/smiling (using existing rig as foundation)
- Same behavior system, same animations — just different visuals

**Asset Requirements**
- `Assets/Skins/baby_stalin/` directory
  - `body.png` — baby torso
  - `head.png` — baby head  
  - `mustache.png` — stalin mustache
  - `cap.png` — peaked military cap
  - `eyebrows.png` — heavy eyebrows
  - `arm_left.png`, `arm_right.png` — baby arms (replacing wings)
- Fallback: programmatic drawing via `CGRenderer` if no PNGs

### Implementation Priority

1. `CharacterSkin` abstract class + `Goose::draw()` delegation — no-op change
2. `SkinRegistry` + config key — skin selection works
3. `GooseSkin` class — existing goose rendering extracted into skin
4. `BabyStalinSkin` class — programmatic drawing (circles + fills for baby, quads for mustache/cap)
5. PNG asset loading for Baby Stalin (optional, polish)

### Files (new)
- `include/character_skin.h` — `CharacterSkin` interface + `SkinRegistry`
- `src/common/character_skin.cpp` — registry + skin lookup
- `include/skins/goose_skin.h` — default goose skin
- `src/common/skins/goose_skin.cpp` — goose rendering extracted from `goose_drawing.mm`
- `include/skins/baby_stalin_skin.h` — baby stalin skin
- `src/common/skins/baby_stalin_skin.cpp` — programmatic baby stalin drawing

### Verification
- [ ] `goose_skin` renders identically to current `DrawGoose()` — A/B compare screenshots
- [ ] `set_skin goose` → no visual change
- [ ] `set_skin baby_stalin` → baby stalin visible, moves, fetches, drops
- [ ] Multi-goose: different skins per goose works
- [ ] All existing unit tests still pass (rendering tests use default skin)

---

## Verification Checklist

After each completed phase:
- [ ] Build succeeds with zero warnings
- [ ] All passing tests still pass
- [ ] Trail test: exit code 0
- [ ] No visual regression (goose body, behaviors, held items)
- [ ] Multi-goose regression test: all 3 geese visible
- [ ] Held item: visible, correct rotation, not clipped
- [ ] Behaviors: anger marks, health bar, nametag, hats, peeking eye, etc. render correctly
