# CadGoose May 12 Plan

## Priority Order

### P0: Bug Fixes (critical — user-reported broken)
| # | Issue | Fix |
|---|-------|-----|
| 1 | Breadcrumbs don't work | Fix key detection; ensure crumbs render |
| 6 | Pomodoro goose doesn't rest | During break phases, stop goose movement |
| 7 | AI NSXPC connection errors | Fix NSVisualEffectView bridge crash |

### P1: Feature Improvements (important)
| # | Issue | Fix |
|---|-------|-----|
| 2 | Goose reddish when angry | Tint body colors red in `goose_drawing.mm` |
| 3 | Jail traps not visible | Brighter cage + "JAIL" label + pulse |
| 4 | Portal keys confusing | Use `1` / `2` / `0` directly (no P-modifier) |
| 5 | Banish activation unclear | Add configurable key binding; add label hint |
| 8 | Autumn leaves toggle | Add config bool + Preferences toggle |

### P2: UI Polish
| # | Issue | Fix |
|---|-------|-----|
| 9 | Settings panel too wide | Auto-width description labels |
| 10 | Canada goose appearance selector | Dropdown: Light / System / Dark / Custom |
| 11 | Goose color editor | New Preferences tab with RGB pickers + preview |

---

## Implementation Plan

### 1. Breadcrumbs Fix
**Files:** `behavior_breadcrumbs.cpp`
- Investigate why key detection fails (maybe `kCGEventSourceStateHIDSystemState` vs `kCGEventSourceStateCombinedSessionState`)
- Ensure `s_crumbImage` loads from correct asset path
- Fix tick guard `time - s_lastKeyCheck < 0.016` to use `ctx.time` (the context's time, not a local clock)

### 2. Angry Goose Tint
**Files:** `goose_drawing.mm`, `behavior_anger.cpp`
- In `DrawGoose`, after computing body/neck/head colors, check if goose has active anger state
- If angerLevel > threshold, blend body colors toward red (lerp with `(1,0,0)`)
- Ensure it works with rainbow mode (rainbow overrides everything) and canada goose mode

### 3. Jail Visibility
**Files:** `behavior_jail.cpp`
- Change cage color from gray to bright orange/yellow
- Add "JAIL" text label above cage
- Add pulsing animation (alpha oscillates with sin(time))

### 4. Portal Keys
**Files:** `behavior_portal.cpp`
- Change key bindings: `1` = place/remove portal A, `2` = place/remove portal B, `0` = toggle portals on/off
- Remove P-modifier requirement

### 5. Banish Activation
**Files:** `behavior_banish.cpp`, `config.h`, `config_gui.mm`
- Add `int key` to `BanishConfig` (default = some key)
- Add detail panel showing key binding
- On key press, start banish

### 6. Pomodoro Rest
**Files:** `behavior_pomodoro.cpp`
- During Break/LongBreak phases, set goose target to current position (stop moving)
- Maybe add a "zzz" or sleeping animation
- On phase end (back to Work), clear the hold

### 7. AI NSXPC Fix
**Files:** `behavior_ai.mm`
- Remove `NSVisualEffectView` (use solid color instead)
- Or check if `com.apple.view-bridge` error is from `NSTextView` inside `NSScrollView` — replace with simpler components
- Add error logging to identify exact source

### 8. Autumn Leaves Toggle
**Files:** `config.h`, `world.cpp`/`world_utils.mm`, `config_gui.mm`
- Add `bool autumnLeaves = true` to `BehaviorConfig.Fun`
- Gate leaf spawning + kick in `world.cpp` and `world_utils.mm`
- Add Preferences toggle row

### 9. Tighter Settings Panel
**Files:** `config_gui.mm`
- Measure the actual width needed for each description label
- Set row layout to use `[desc sizeWithAttributes:]` for width
- Or just tighten the widths empirically

### 10. Canada Goose Appearance
**Files:** `config.h`, `config_gui.mm`, `goose_drawing.mm`
- Replace `bool canadaGooseMode` with `enum AppearanceMode { Light, Dark, System, Custom }`
- Add dropdown in Preferences (e.g. in a new "Appearance" section at top)
- `goose_drawing.mm` reads the enum

### 11. Goose Color Editor
**Files:** `config.h`, `config_gui.mm`, `goose_drawing.mm`
- Add new tab/page in Preferences panel
- RGB sliders for: Body, Neck, Head, Beak, Eyes, Outline, Shadow
- Rendered goose preview using `DrawGoose` to an NSImage/CGImage
- Save/load color packs as config files
