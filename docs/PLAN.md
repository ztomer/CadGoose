# CadGoose Project Plan

## Features

### 3. Polished settings + AI panel
- Apple-tier, liquid glass everywhere interface
- Expert UI review needed — appearance and AI tabs are messy
- NSVisualEffectView with vibrant backgrounds, smooth animations, proper spacing

### 4. Font switch: Comic Sans → Maple Mono
- Bundle the font (SIL Open Font License, commercial use allowed)
- Add attribution in README for Maple Mono and toml11
- Replace all `[NSFont fontWithName:@"Comic Sans MS" ...]` fallback patterns

## Bugs

### 1. Preferences panel needs UI review
- Appearance and AI tabs specifically are messy
- Need an expert UI reviewer to audit layout, spacing, consistency

### 4. Jail only works once
- Need multiple jails, reset functionality, better UX
- Current JailState is per-goose; need multiple traps

### 5. Rest mode for goose
- Goose goes to bottom-right corner during Pomodoro work period
- Sleeps (eyes closed, lays down)
- Integrates with Pomodoro system behavior

## Maintenance

### 1. Remove DebugGoose code + config entirely
- Find and remove all debug goose related code

### 2. Linux + Uncle Bob code review
- Apply clean code principles across the board
- Linux best-effort support — make it clear in docs: not tested, patches welcome

### 3. Enforce 500-line limit per file
- No file should exceed 500 lines
- `behavior_ai.mm` (761) is over — need to split

### 4. Linux best-effort support
- Document: "Linux support is best-effort and not regularly tested. Patches welcome."

### 5. Repo code layout review
- Is `behaviors/` still descriptive? Source files are split across `src/common/behaviors/`, `src/platform/macos/`
- Evaluate directory structure for v1 clarity

### 6. Evaluate what's missing for v1 release
- Feature completeness, stability, documentation

### 7. Deep scan for hardcoded values → move to config
- Check for other hardcoded magic numbers, strings, paths

### 8. Remove stale references
- Clean up any stale enum values, comments, dead code paths

## Manual

### 1. Go over all supported behaviors — collect bug list
- Systematic test of every behavior toggle
- Document issues found

### 2. "What can make this more joyful?" — ask Gemini
- Get creative/UX suggestions for delight factor

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
