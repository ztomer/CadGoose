# CadGoose Project Plan

## Status: All P0/P1 Items Complete (2026-05-11)

### Completed This Session

#### 1. Removed Useless Behaviors ✅
- Removed ColorPicker, Debugoose, Clicker
- Updated CMakeLists.txt, config.h, tests
- 17 active behaviors remain

#### 2. Settings Panel Redesign ✅ (macOS only)
- **Top-level menu**: "Settings" is now a top-level menu item (not under Behaviors)
- **Simplified UI**: Shows only behavior toggles with descriptions
- **Layout**: `[Toggle] [Description] [⚙ Detail Button]` per row
- **Sections**: Fun, Control, Info, Systems
- **Removed**: "Open Configuration" and "Reload Configuration" menu items (moved to config file)
- **Removed**: Settings toggle from Behaviors > Info menu

#### 3. AI Chat Window ✅ (macOS only)
- Added `AI_OpenChat` with C linkage (`extern "C"`)
- Added AI Chat menu item under Settings
- Fixed linkage issues, builds successfully

#### 4. Hat Position Fix ✅ (macOS only)
- Changed from relative `neckHead` to absolute position: `pos + neckHead`
- Fixed Y-flip: `screenY = screenH - (goose->pos.y - headRel.y + offsetY)`
- Added debug logging for head/hat positions

#### 5. Config Option Explanations ✅
- Added `explanation` field to `ConfigOption` struct
- Added `_EX` macro variants: `CONFIG_BOOL_EX`, `CONFIG_INT_EX`, `CONFIG_FLOAT_EX`
- Backward-compatible: original macros use empty string for explanation
- Config GUI shows explanations on hover

---

## Current Behavior Status (17 total)

| Behavior | Status | Notes |
|----------|--------|-------|
| Honcker | ✅ Done | F key honk |
| Drag | ✅ Done | Click and drag |
| Jail | ✅ Done | O=set, P=trap |
| Portal | ✅ Done | P+1/2 place, P+0 toggle |
| Anger | ✅ Done | Anger/punch system |
| Ball | ✅ Done | Push balls around |
| BreadCrumbs | ✅ Done | RightShift drops crumbs |
| Hats | ✅ Fixed | Y-flip corrected |
| Acid | ✅ Done | Spin with honks |
| Rainbow | ✅ Done | Cycle colors |
| Health | ✅ Done | Health bar system |
| Banish | ✅ Done | Ctrl+Alt+Middle click |
| Nametag | ✅ Done | Shows goose name |
| Presence | ✅ Done | Discord RPC |
| GooseManager | ✅ Done | Control tasks/speeds |
| AI | ✅ Fixed | Chat window opens via menu |
| Pomodoro | ✅ Done | Timer behavior |

---

## Test Results
- **204 tests passing** from 42 test suites
- 0 failures

---

## File Size Status

| File | Current LOC | Target |
|------|-------------|--------|
| config.cpp | ~140 | <200 |
| goose.cpp | ~404 | <500 |
| main.mm | ~480 | <500 |
| All behaviors | <150 each | <500 |

---

## Mod Attribution

All behaviors based on [Desktop Goose ResourceHub](https://desktopgooseunofficial.github.io/ResourceHub/).

| Behavior | Author | Source |
|----------|--------|--------|
| Honcker | DesktopGooseUnofficial | ResourceHub |
| Drag | Straaft | ResourceHub |
| Jail | WackyModer | GitHub |
| Portal | Moonaliss1 | GitHub |
| Anger | VisualError | GitHub |
| Ball | TheOrlando | GitHub |
| BreadCrumbs | Straaft | ResourceHub |
| Hats | DaNike | GoosMods |
| Acid | F!NN | ResourceHub |
| Rainbow | - | ResourceHub |
| Health | - | ResourceHub |
| Banish | - | ResourceHub |
| Nametag | - | ResourceHub |
| Presence | - | ResourceHub |
| GooseManager | - | ResourceHub |
| AI | - | ResourceHub |
| Pomodoro | - | ResourceHub |