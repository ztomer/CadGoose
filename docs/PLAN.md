# CadGoose Project Plan

## Current Session Priorities (2026-05-08)

| # | Priority | Item | Status |
|---|----------|------|--------|
| 0 | P0 | Build cache + project root cleanup | ✅ DONE |
| 1 | P0 | Fix bugs (mirrored text, behavior key detection) | ✅ DONE |
| 2 | P1 | Configuration pane (Config GUI) | ✅ DONE |
| 3 | P1 | Osaurus + Ollama (local LLM) support | ✅ DONE |
| 4 | P1 | Visual regression tests (OCR-based screenshot tests) | ✅ DONE |
| 5 | P2 | No file above 500 LOC | In Progress |
| 6 | P2 | Linus + Uncle Bob code review | In Progress |
| 7 | P2 | Performance profiling using Xcode Instruments | Todo |
| 8 | P3 | Best effort Linux support maintenance | Todo |
| 9 | P2 | Review all tests for relevance | Todo |
| 10 | P2 | Code quality pass | Todo |

### 0. Build Cache + Project Cleanup ✅
- [x] Add CMake build cache support (ccache)
- [x] Move test_*.m, test_*.png, test_*.py, test_render, test_ct, test_ct_color, test_layer, toml_test to tests/ folder
- [x] Verify tests still build and run (129 tests passing)

### 1. Fix Bugs
- [x] Debugoose/Nametag contrast
- [x] Meme images too large
- [x] Honker key detection
- [x] Jail/Drag/Banish key detection
- [x] Clicker random range
- [x] Drag animation broken
- [x] Pomodoro shows nothing
- [x] **Text mirrored on X axis - FIXED!**

### 2. Configuration Pane (Config GUI)
- **Location**: `src/platform/macos/config_gui.mm`
- **Requirements**:
  - [ ] All behaviors accessible via sliders/buttons
  - [ ] Real-time preview of changes
  - [ ] Persist to config.toml on change
  - [ ] Reset to defaults button
- **Design**: Native macOS window with NSStackView for sections

### 3. Osaurus + Ollama (Local LLM) Support
- **Purpose**: Chat with goose via local LLM (no cloud dependency)
- **Providers**: Osaurus (default port 1337), Ollama (default port 11434)
- **Config**: Both ports configurable, selectable at runtime
- **Implementation**:
  - [x] Add `ai.useOsaurus` and `ai.useOllama` toggles
  - [x] Add `ai.osaurusPort` (default: 1337) and `ai.ollamaPort` (default: 11434)
  - [x] Configurable endpoint construction from host + port
  - [ ] Add model selection for each provider
  - [ ] Auto-detect available provider on startup
  - [ ] Fallback: try Osaurus first, then Ollama
- **API**: Both use same `/v1/chat/completions` format

### 4. Visual Regression Tests
- **Purpose**: Detect rendering regressions before they reach users
- **Tool**: tesseract OCR + pixel comparison
- **Coverage**:
  - [x] Text rendering (fixed items, held items) - **FIXED, all passing**
  - [ ] Goose animation states
  - [ ] Behavior overlays (health bar, countdown, etc.)
- **Location**: `tests/platform/macos/test_renderer.mm`
- **Fix Applied**: DON'T use Y-flip for text rendering. Use manual Y conversion: `screenY = height - worldY`

### 5. File Size Limit (500 LOC)
- **Rule**: No source file exceeds 500 lines
- **Current offenders**:
  - ⚠️ `config.cpp` (1150 LOC) - Config_Load has 237-line switch statement
  - ⚠️ `ui.cpp` (1632 LOC) - Linux UI, very large
  - ⚠️ `main.mm` (564 LOC) - macOS main, borderline
  - ⚠️ `goose.cpp` (511 LOC) - borderline
- **Tool**: Add pre-commit hook or CI check

### 5b. Config System Refactor (DRY Architecture) - IN PROGRESS
- **Problem**: Config params listed 4x (struct default, Config_Load, Config_SaveNow, Config_InitRegistry)
- **Root Cause**: No single source of truth; each function duplicates field enumeration

#### Architecture Decision: Registry as Single Source of Truth

**Key Insight**: `ConfigOption` struct already holds section/key/type/ptr/min/max/step/default.
The registry is the natural single source of truth. Load/Save should iterate it.

**Design Principles** (per user: "best option for long term maintainability"):
1. **One Definition**: Each config param defined ONCE in `Config_InitRegistry()`
2. **Auto-wiring**: Load/Save iterate registry, no manual field enumeration
3. **Type Safety**: Macros generate correct ConfigOption with metadata
4. **Colors are special**: Nested TOML (color.goose.r) requires helpers, not in registry
5. **Callbacks optional**: onChange field for reactive save on edit

#### File Responsibilities

| File | Purpose | Size Target |
|------|---------|-------------|
| `config.h` | ConfigOption struct, macros, externs | ~400 LOC |
| `config.cpp` | Globals (g_config, g_time, g_configRegistry), utility functions (Trim, ParseBool, etc), Config_GetPath, ConfigDirPath, Config_Get/SetValueByKey | <200 LOC |
| `config_helpers.h` | TOML parsing helpers (get_bool/int/float, section_has_key), template for color helpers | ~100 LOC |
| `config_load.cpp` | `Config_Load(toml)`, `Config_LoadAll()` - iterates registry, calls helpers | <100 LOC |
| `config_save.cpp` | `Config_SaveAll()` - iterates registry, builds TOML table, writes file | <100 LOC |
| `config_registry.cpp` | `Config_InitRegistry()` - ALL config entries in ONE place | ~380 LOC |

#### How Load/Save Work

**Load**: Iterate registry → for each option, call appropriate helper based on type → cast ptr to correct type → assign

**Save**: Iterate registry → for each option, cast ptr to correct type → insert into toml::table[section][key]

**Colors**: Explicit helper calls for nested TOML (color.goose.r, etc.) - these are not in the registry since TOML structure differs

#### Adding New Config Param

1. Add member to struct in `config.h` with default value
2. Add ONE line to `Config_InitRegistry()` in `config_registry.cpp`:
   ```
   CONFIG_FLOAT("Section", "key_name", "Display Label", &g_config.section.memberName, min, max, step, OnConfigChange)
   ```
3. Load/Save auto-handled by iteration

#### Implementation Status
- [x] `config_helpers.h` - helpers for TOML parsing, colors
- [x] `config_load.cpp` - iteration-based load (calls Config_LoadAll)
- [x] `config_save.cpp` - iteration-based save
- [x] `config_registry.cpp` - Config_InitRegistry with all entries
- [ ] `config.cpp` - stripped to globals + utilities (<200 LOC target)
- [ ] Verify: build succeeds, tests pass, app runs correctly
- [ ] Lint: all source files <500 LOC

### Project Structure (Updated 2026-05-08)
```
docs/
├── PLAN.md                    # Current project priorities
├── COMPLETED_TASKS.md          # Historical completed items
├── README_LINUX.md            # Linux build instructions
└── references/
    └── MOD_IMPLEMENTATION_GUIDE.md

src/
├── common/
│   ├── config.cpp            ⚠️ 1150 LOC - needs refactor
│   ├── goose.cpp              ⚠️ 511 LOC - borderline
│   └── behaviors/             # All behavior implementations
├── platform/
│   ├── macos/                 # AppKit + CoreGraphics
│   │   ├── main.mm           ⚠️ 564 LOC - borderline
│   │   └── renderer.mm
│   └── linux/
│       ├── ui.cpp             ⚠️ 1632 LOC - very large
│       └── protocols/         # Wayland protocol definitions
└── include/                   # Headers

tests/
├── common/                    # Unit tests
├── platform/macos/            # macOS rendering tests
└── test_main.cpp             # Main test suite

tools/
└── Extractor/                # Asset extraction tool

vendor/
└── toml11/                   # TOML parsing library
```

### 6. Code Review (Linus + Uncle Bob)
- **Principles**:
  - Linus: Clarity over cleverness, no magic numbers, explicit over implicit
  - Uncle Bob: Single Responsibility, small functions, meaningful names
- **Checklist**:
  - [x] Goose::Update() refactored into smaller functions
  - [x] GooseState converted to enum class
  - [x] Menubar building extracted to setupMenubar()
  - [x] onChange function pointer replaced with std::function<void()>
  - [x] Config macros added (CONFIG_BOOL/INT/FLOAT with proper ranges)
  - [ ] Review all public functions have clear purpose
  - [ ] No file >500 LOC (in progress)
  - [ ] No magic numbers in logic
  - [ ] Functions <30 lines preferred
  - [ ] Tests for all behavior logic

### 7. Performance Profiling (Xcode Instruments)
- **Tool**: Instruments.app (Time Profiler, Memory Profiler)
- **Targets**:
  - [ ] 60fps rendering (no drops below 58fps)
  - [ ] Memory stable (no leaks over 10min runtime)
  - [ ] Asset loading <100ms
- **Workflow**:
  1. Build with Release config
  2. Run under Instruments
  3. Profile typical usage (wander, chase, behaviors)
  4. Fix top 3 hotspots
  5. Re-benchmark

### 8. Linux Support (Best Effort)
- **Priority**: P3 (lowest)
- **Scope**: Maintain compatibility, fix critical bugs only
- **Current State**: Basic X11/Wayland rendering, no native AI integration
- **Known Limitations**:
  - AI chat not implemented on Linux (no NSURLSession equivalent)
  - Audio may have platform-specific issues
- **Goals**:
  - [ ] Keep Linux build compiling
  - [ ] Fix critical rendering bugs if reported
  - [ ] Document platform differences in README_LINUX.md

### 9. Code Cleanup & Maintenance
- **Tasks**:
  - [ ] Review all tests in `tests/` folder for relevance
  - [ ] Remove obsolete test files
  - [ ] Verify all 132 tests still pass
  - [ ] Add missing tests for new behaviors

### Project Structure (Updated 2026-05-08)
```
docs/
├── PLAN.md                    # Current project priorities
├── COMPLETED_TASKS.md          # Historical completed items
├── README_LINUX.md            # Linux build instructions
└── references/
    └── MOD_IMPLEMENTATION_GUIDE.md

src/platform/linux/
├── protocols/                  # Wayland protocol definitions
│   └── wlr-virtual-pointer-unstable-v1.xml
└── ...

tests/
├── test_main.cpp              # Main test suite
├── test_renderer.mm            # macOS rendering tests
├── output_test*.toml           # Config test files
├── CadGoose.sln               # VS solution (legacy)
├── config.ini                  # Legacy config
├── check_img.py               # Image debug script
└── out_img.png                # Image debug output
```

---

## Mirrored Text Investigation

### Root Cause
Test `Rendering.TextNotMirrored` confirmed text IS mirrored:
- OCR output: "HEF TO MOBID" (clearly mirrored "HELLO WORLD")
- Tesseract CAN read mirrored text, it just produces reversed output

### Current Transform (renderer.mm)
```objc
CGContextTranslateCTM(ctx, 0, self.bounds.size.height);  // Move origin to top
CGContextScaleCTM(ctx, 1.0, -1.0);                      // Flip Y
```

### Possible Causes
1. **Coordinate System Mismatch**: World coordinates (Y up) vs CGContext (Y down)
2. **drawInRect:/drawAtPoint: behavior**: These AppKit methods may interpret transforms differently
3. **Font rendering at negative Y**: When flipped, world Y values map to negative screen Y

### Next Steps
- [ ] Investigate why Y-flip causes X-mirror effect in AppKit text
- [ ] Compare with v1.1 rendering code if available
- [ ] Consider: don't flip Y for text rendering, use coordinate conversion instead

---

## Behavior Implementation Status

All 19 ResourceHub behaviors implemented:

| Behavior | Status | Notes |
|----------|--------|-------|
| Honcker | ✅ Done | F key honk |
| Drag | ✅ Done | Click and drag |
| Jail | ✅ Done | O=set, P=trap |
| Portal | ✅ Done | P+1/2 place, P+0 toggle |
| Clicker | ✅ Done | Random cursor clicks |
| Anger | ✅ Done | Anger/punch system |
| Ball | ✅ Done | Push balls around |
| BreadCrumbs | ✅ Done | Leave trail |
| Hats | ✅ Done | Render hat above head |
| Acid | ✅ Done | Spin with honks |
| Rainbow | ✅ Done | Cycle colors |
| Health | ✅ Done | Health bar system |
| Banish | ✅ Done | Ctrl+Alt+Middle click |
| Nametag | ✅ Done | Shows goose name |
| Debugoose | ✅ Done | Debug overlay |
| Presence | ✅ Done | Menu bar status |
| GooseManager | ✅ Done | Control tasks/speeds |
| AI | ✅ Done | Chat window + HTTP client |
| Color Picker | ✅ Done | Opens color panel |

---

## Still Needs Testing

| Behavior | Status |
|----------|--------|
| Jail | Manual test needed |
| Drag | Manual test needed |
| Banish | Manual test needed |
| Portals | Manual test needed |
| Acid | Manual test needed |
| BreadCrumbs | crumbs.png rendering |
| Hats | Hat images needed |
| AI | Configured for Osaurus/Ollama - needs local LLM running |

---

## Test Results

- **132 tests passing**, 0 failing (all regression tests passing!)
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
| Hats | - | ResourceHub |
| Acid | F!NN | ResourceHub |
| Rainbow | - | ResourceHub |
| Health | - | ResourceHub |
| Banish | - | ResourceHub |
| Nametag | - | ResourceHub |
| Debugoose | - | ResourceHub |
| Presence | - | ResourceHub |
| GooseManager | - | ResourceHub |
| AI | - | ResourceHub |

See `COMPLETED_TASKS.md` for historical completed items.