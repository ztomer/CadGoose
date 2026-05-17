# CadGoose Forward-Looking Plan

## Pending Work

### Item Dragging — History of Attempts
The goal: drag fetched meme/text items while maintaining click-through everywhere else.

**Attempt 1: `ignoresMouseEvents = NO` + responder chain**
- Set window to receive mouse events natively
- `hitTest:` returns self for item areas, nil for empty areas
- **Result**: Window blocks all mouse events to apps underneath when `ignoresMouseEvents=NO`, even with `hitTest:` returning nil. Regression — can't click other apps.

**Attempt 2: `ignoresMouseEvents = YES` + `addLocalMonitorForEventsMatchingMask`**
- Window stays click-through, local monitors intercept events at app level
- **Result**: Local monitors DON'T fire when `ignoresMouseEvents=YES` — events bypass the app entirely. Drag never triggers.

**Attempt 3: `ignoresMouseEvents = YES` + `addGlobalMonitorForEventsMatchingMask`**
- Global monitors fire regardless of window settings
- **Result**: Global monitors can't consume events (can't return `nil` to swallow them). Events pass through to apps underneath AND to the goose window — causes double-processing and can't start a proper drag session.

**Attempt 4: `ignoresMouseEvents = NO` + `hitTest:` selective + responder chain**
- `hitTest:` returns self only when mouse is over an item, nil otherwise
- **Result**: `hitTest:` returning nil still doesn't make the window click-through when `ignoresMouseEvents=NO`. The window still captures the event, just doesn't handle it.

**Root cause analysis:**
- `ignoresMouseEvents=YES` → events pass through entirely, app never sees them
- `ignoresMouseEvents=NO` → window always captures events, even in empty areas
- `hitTest:` only controls which view receives the event, not whether the window captures it
- Local monitors only see events delivered to the app's windows
- Global monitors see all events but can't consume/block them

**Potential solutions to explore:**
1. **`canBecomeKeyWindow` + `sendEvent:` override** — Override `GooseWindow sendEvent:` to selectively forward unhandled events to the next responder
2. **`NSWindowDelegate` + `windowWillMove:`** — Not applicable
3. **`CGEventTap`** — System-level event tap that can filter/consume events before they reach any app. Requires Accessibility permission. Heavy-handed but would work.
4. **Dynamic `ignoresMouseEvents` toggle** — Set to `NO` only during active drag, `YES` otherwise. Problem: there's a race between mouse-down and the toggle.
5. **`NSWindow level` manipulation** — Not relevant to event routing.
6. **`acceptsMouseMovedEvents` + `mouseEntered:/mouseExited:`** — Only for tracking, not for click-through.

### Linux CI Testing
- Enable unit tests on Linux CI runner
- Requires abstracting remaining CoreGraphics dependencies in test code
- Consider mock renderer for headless Linux tests

### Config Code Generation Integration
- Integrate generated files (`config_registry_generated.cpp`, `config_gui_generated.mm`) into CMake build
- Replace manual registry files with generated ones
- Add pre-build step to run `tools/generate_config.py`
- Expand schema to cover remaining sections (Behaviors, AI, Portal, Color)

### Behavior Conflict System
- `conflicts` field in `Behavior` struct is never populated
- Pomodoro + Drag can fight over `goose->vel`
- Define conflict rules and implement resolution at `BehaviorRegistry` level

### Resource Manager
- `CGImageRef`/`CGFontRef` in static globals across behaviors
- No cleanup on behavior disable/re-enable
- Centralize asset lifecycle management

### Input Abstraction
- Inconsistent cursor access patterns across behaviors
- Some use `g_cursorProvider`, others use backend directly
- Unify through a single `InputManager` interface

### Integration Tests
- No end-to-end flow tests
- Test complete behavior cycles (e.g., fetch → return → drop)
- Test behavior interactions (e.g., Pomodoro + Toys)

### Linux Platform Tests
- Zero tests for Linux platform code
- Consider GTK test framework or headless rendering tests

### Config GUI Panel Tests
- 712 lines of GUI code untested
- Test panel creation, toggle state sync, slider value changes

### Performance Optimization
- Profile behavior tick time under load (20+ behaviors, multiple geese)
- Consider spatial partitioning for item pickup detection
- Optimize EventBus for high-frequency events

### Accessibility Improvements
- 30 AX tests skipped (require running app)
- Investigate headless AX testing or CI-friendly approach
