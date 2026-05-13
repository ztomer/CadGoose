# CadGoose Project Plan

## Current Session Priorities (2026-05-10)

| # | Priority | Item | Status |
|---|----------|------|--------|
| 0 | P0 | Fix behavior bugs (ball, breadcrumbs, hats) | Done |
| 1 | P1 | Write automated tests for behaviors | Done |
| 2 | P2 | No file above 500 LOC | Done |
| 3 | P2 | Linux support maintenance | Done |
| 4 | P1 | Theme Setting Panel & Editor | Todo |

## Bug Fixes (2026-05-10)

### Ball Behavior - FIXED

- **Issue**: Ball spins in place, doesn't bounce, goose doesn't make contact
- **Root Cause**: Coordinate space mismatch (cursor in screen coords vs ball in world coords), missing goose chase integration
- **Fix**:
  - Use screen coordinates directly for cursor position
  - Fixed ball collision detection with proper distance calculation
  - Goose targets ball center and chases it
- **Reference**: BallModv1.0.dll decompiled

### Hats Behavior - FIXED

- **Issue**: Hat too large, upside down
- **Root Cause**: Y-flip coordinate transform, incorrect scaling
- **Fix**:
  - Calculate screenY from worldY properly
  - Scale hat based on config, render upright
- **Reference**: HatGoos by DaNike

### BreadCrumbs Behavior - FIXED

- **Issue**: Nothing happens when key pressed
- **Root Cause**: Key detection rate limiting, render coordinate issues
- **Fix**:
  - Use screen coordinates directly for cursor
  - Render crumbs at cursor position using backend
- **Reference**: BreadCrumbs.dll decompiled

## Behavior Testing

### Unix Socket Commands (for automated testing)

```bash
# Enable/disable behaviors
echo -e "enable\tball" | nc -q1 -U /tmp/desktop-goose.sock
echo -e "disable\tball" | nc -q1 -U /tmp/desktop-goose.sock
echo -e "enable\tbreadcrumbs" | nc -q1 -U /tmp/desktop-goose.sock
echo -e "enable\thats" | nc -q1 -U /tmp/desktop-goose.sock
```

### Test Plan

- [x] Ball: enable, verify ball renders, click on ball, verify bounce, verify goose chases
- [x] BreadCrumbs: enable, hold Right Shift, verify crumbs appear at cursor
- [x] Hats: enable, verify hat renders above goose head, correct size and orientation

---

## Archive: Completed Items

### 0. Build Cache + Project Cleanup ✅ (2026-05-08)

- Add CMake build cache support (ccache)
- Move test files to tests/ folder
- Verify tests still build and run

### 1. Fix Bugs ✅ (2026-05-08)

- Debugoose/Nametag contrast
- Meme images too large
- Honker key detection
- Jail/Drag/Banish key detection
- Clicker random range
- Drag animation broken
- Pomodoro shows nothing
- Text mirrored on X axis - FIXED!

### 2. Configuration Pane ✅ (2026-05-08)

- Implemented via menu toggles and Unix socket commands
- All 20 behaviors accessible

### 3. Osaurus + Ollama (Local LLM) Support ✅ (2026-05-08)

- `ai.useOsaurus` and `ai.useOllama` toggles
- Configurable ports

### 4. Visual Regression Tests ✅ (2026-05-08)

- OCR-based screenshot tests
- Text rendering fixed

---

## File Size Status

| File | Current LOC | Target |
|------|-------------|--------|
| config.cpp | ~140 | <200 |
| goose.cpp | ~404 | <500 |
| main.mm | ~480 | <500 |
| ui.cpp | ~492 | <500 |
| All behaviors | <150 each | <500 |

---

## Test Results

- **182 tests passing**, 0 failing
- Build: `build/CadGoose`
- Tests: `build/CadGooseTests`

---

## Mod Attribution

All behaviors based on [Desktop Goose ResourceHub](https://desktopgooseunofficial.github.io/ResourceHub/).

| Behavior | Author | Source |
|----------|--------|--------|
| Honcker | DesktopGooseUnofficial | ResourceHub |
| Drag | Straaft | ResourceHub |
| Jail | WackyModer | GitHub |
| Portal | Moonaliss1 | GitHub |
| Clicker | Wolf/NE1W01F | GitHub |
| Anger | VisualError | GitHub |
| Ball | TheOrlando | GitHub |
| BreadCrumbs | Straaft | ResourceHub |
| Hats | DaNike | GoosMods |
| Acid | F!NN | ResourceHub |
| Rainbow | - | ResourceHub |
| Health | - | ResourceHub |
| Banish | - | ResourceHub |
| Nametag | - | ResourceHub |
| Debugoose | - | ResourceHub |
| Presence | - | ResourceHub |
| GooseManager | - | ResourceHub |
| AI | - | ResourceHub |
| Color Picker | - | ResourceHub |
