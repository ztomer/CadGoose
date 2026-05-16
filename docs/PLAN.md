# CadGoose Forward-Looking Plan

## Pending Work

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
