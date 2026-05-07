# Project Plan for CadGoose

## 1. Performance Profiling

- **Tool**: Use Xcode Instruments profiler.
- **Steps**:
  - Profile CPU usage, memory allocation, rendering bottlenecks.
  - Identify hotspots (e.g., in rendering loop, asset loading).
  - Optimize: Reduce draw calls, cache assets, minimize coordinate transforms.
  - Benchmark before/after changes.
  - Target 60fps stability.

## 2. Quality of Life & Ideas

- [x] Add interactable items: soccer balls, beachball. **Completed (soccer, beach, generic balls).**
- [x] Implement Pomodoro timer mode (e.g., wanders for 25 minutes, then attacks constantly for 5 minutes, repeat). This should be an explicit mode toggle. Menu bar icon should rotate clockwise and complete a full rotation during the work/rest duration. **Completed.**
- Explore and implement behaviors from [Desktop Goose ResourceHub](https://desktopgooseunofficial.github.io/ResourceHub/mods/explore/mods.html), remembering to add attribution in README.md. See Phase 4 below for implementation plan.

## 3. Overall Timeline and Priorities

- **Phase 1**: Fix bugs (Image and Text Placement) - High priority.
- **Phase 2**: Implement testing (Behavioral & Rendering) - Completed.
- **Phase 3**: Performance optimization - Skipped (Xcode instruments not suitable for CLI/unnecessary given current frame rate).
- **Phase 4**: Implement behavior system (see below).
- **Milestones**: Weekly reviews, ensure no regressions via tests.
- **Risks**: Coordinate system complexity; mitigate with logging and unit tests.

---

## Completed Tasks

### Autumn Leaves

- **Task**: Add autumn leaves from the v0.3.1 update, referencing `autumn.dll`.
- **Solution**:
  - Since standard decompilation tools (like dotPeek) weren't immediately available to fully rip out image assets, and analysis of `autumn.cs` (generated via `ilspycmd`) proved the leaves were procedurally drawn ellipses with varying Z-levels, I implemented `LeafPile` and `Leaf` structs directly in `world.h`/`world.cpp`.
  - They spawn randomly around the screen, render using `CGContextFillEllipseInRect` with realistic autumn colors, and get kicked up in a cloud of physics particles whenever the goose runs over them, complete with simulated gravity and bounce.
- **Status**: Completed.

### Behavioral and Rendering Testing

- **Task**: Create unit tests for behavior and rendering.
- **Solution**: Existing tests in `test_main.cpp` and `test_renderer.mm` cover the math, physics, configurations, and core loops for both goose behaviors and Y-axis/bounds logic. Tests pass successfully in the `CadGooseTests` binary.
- **Status**: Completed.

### Refactored Hardcoded Values

- **Task**: Hardcoded values in `goose.h`, `items.h`, and `renderer.mm` should be moved to `config.ini`.
- **Solution**:
  - Cleaned up unused fields like `maxForce` in `Goose`.
  - Added `itemLifetime` to `ItemConfig` for `items.h` to use instead of a magic number.
  - Substituted raw layout numbers in `renderer.mm` with newly added `RenderConfig` fields (`bodyHeight`, `bodyWidth`, `neckSize`, `head1Size`, `head2Size`, `eyeOffsetXFront`, etc.).
- **Status**: Completed.

### Slower Wander Speed

- **Issue**: The goose was running around the screen when in wander mode due to distance-based running.
- **Solution**: Restored distance-based speed determination for WANDER state, but increased the `runDistanceThreshold` (from 600 to 1200) and reduced the frequencies for fetching memes (`memePickupChance`, `fetchBaseChance`) and chasing the cursor (`chaseChance`) in `config.h`. This ensures the goose walks more often and spends less time targeting items/cursor so it has more time to wander.
- **Status**: Fixed.

### Stuck Goose on Startup Bug

- **Issue**: The goose sometimes got stuck on startup and only resolved upon a restart.
- **Root Cause**: The random number generator was not seeded (`srand` was never called), causing the goose to generate identical random numbers in certain states and loop indefinitely or hit boundary conditions predictably.
- **Solution**: Added `srand((unsigned int)time(NULL));` to the initialization of `main()`.
- **Status**: Fixed.

### Interactive Memes and Images

- **Issue**: Memes and text notes could not be moved or dismissed manually.
- **Solution**:
  - Refactored `GooseWindow` to allow mouse events.
  - Implemented `hitTest:`, `mouseDown:`, `mouseDragged:`, and `mouseUp:` in `GooseView` to enable dragging of items.
  - Added an interactive 'X' close button to dismiss items.
  - Prevented goose from picking up items that are currently pinned/dragged by the user.
- **Status**: Fixed.

### Image and Text Placement Bug

- **Issue**: Very similar to the muddy footprints problems, probably the same cause. We need to be careful here, since we don't want the image and text to be flipped as well - right now the text and images are fully readable.
- **Tasks**:
  - Investigate coordinate system for text/image rendering.
  - Apply fix while ensuring current readability and placement are preserved.
- **Status**: Fixed. Placement is correct and readable.

### Debug Exit Bug

- **Issue**: The program would forcefully exit after 120 seconds of running.
- **Root Cause**: Leftover debug code (`exit(0)`) in the render loop (`renderer.mm`).
- **Solution**: Removed the debug exit condition.
- **Status**: Fixed.

### Footprint Bug Fix

- **Issue**: Muddy footprints appeared far from goose's feet due to coordinate mismatch.
- **Root Cause**: macOS renderer did not render footprints in the flipped coordinate system and lacked rotation.
- **Solution**: Moved footprint rendering inside the flipped Y block and implemented `CGContextRotateCTM` for accurate footprints.

### Animation and Wander Mode

- **Issue**: Goose appeared to always be running with extended neck; should show upright neck in wander mode.
- **Root Cause**: `targetState` was based only on `currentSpeed >= runSpeedThreshold`, not considering goose state.
- **Solution**: Modified neck animation logic: `targetState = (state == WANDER) ? 0 : ((currentSpeed >= runSpeedThreshold) ? 1 : 0)`.
- **Status**: Implemented with debug logging. Wander state now correctly shows upright neck.

### Configuration Management

- **Features**:
  - Added "Open Configuration" menu option to launch config.ini in default editor.
  - Added "Reload Configuration" option to reload settings without restart.
- **Implementation**:
  - Integrated with macOS NSWorkspace for opening files.
  - Added config reload logic in world.cpp or main.mm.
- **Status**: Implemented in macOS menubar with `openURL:` and `Config_InitRegistry()`.

### Configuration Documentation

- **Task**: Enhance config.ini with extensive comments.
- **Content**:
  - Documented all settings (e.g., debug mode, asset paths).
  - Included examples and valid ranges.
  - Added sections for behavior tuning, rendering options.
- **Status**: Updated `Config_SaveNow()` in `config.cpp` to autogenerate section headers, descriptions, and valid ranges directly into `config.toml`.

### Configuration Format Migration (INI → TOML)

- **Task**: Convert all configuration from INI to TOML format with per-behavior config files.
- **Solution**:
  - Added `vendor/toml11` as git submodule for TOML parsing library
  - Created 17 behavior TOML config files in `config/behaviors/*.toml`
  - Created main `config/config.toml` configuration file
  - Rewrote `config.cpp` with `Config_Load()` and `Config_SaveNow()` using toml11 library
  - Updated CMakeLists.txt to include toml11 paths for both CadGoose and CadGooseTests
  - Updated README.md paths from `.ini` to `.toml`
- **Status**: Completed. Build succeeds with 114 tests passing.

---

## Phase 4: Behavior System

### Overview
Integrate ResourceHub behaviors directly into the main binary. All behaviors are user-togglable via menu and config.

### Implementation Status

**Completed:**
- [x] Behavior system architecture (`include/behavior.h`)
- [x] BehaviorStateManager with thread-safe per-goose state storage
- [x] BehaviorRegistry with init/tick/render hooks
- [x] DT-normalized physics helpers (rotation, velocity, gravity)
- [x] macOS AccessibilityManager with AXIsProcessTrusted() checks
- [x] FailsafeHotkeyMonitor (Cmd+Shift+Escape global hotkey)
- [x] Cursor hijack protection with 2s timeout
- [x] Unit tests (114 tests passing)
- [x] All behaviors use per-behavior config structs (no hardcoded magic numbers)

**Implemented Behaviors:**
- [x] Honcker - F key honk
- [x] Drag - Click and drag goose
- [x] Jail - O=set, P=trap
- [x] Portal - P+1/2 place, P+0 toggle
- [x] Clicker - Random cursor clicks
- [x] Anger - Anger/punch system (OnePunchGoose)
- [x] Ball - Push balls around
- [x] BreadCrumbs - Leave trail of crumbs
- [x] Hats - Put hats on geese
- [x] Acid - Spin with honks
- [x] Rainbow - Cycle through colors
- [x] Health - Health bar system
- [x] Banish - Ctrl+Alt+Middle Click to banish goose
- [x] Nametag - Shows goose name above head
- [x] Debugoose - Debug overlay with log entries
- [x] Presence - Shows goose state in menu bar
- [x] GooseManager - Control goose tasks and speeds
- [x] AI - Chat with goose via AI
- [x] Color Picker - Change goose color via color panel

**Config Structs (BehaviorConfig namespace):**
- [x] `HonckerConfig` - key binding (default F=0x24)
- [x] `DragConfig` - radius (default 45.0f)
- [x] `JailConfig` - keyO, keyP, size (default 150.0f)
- [x] `AngerConfig` - increaseRate, decreaseRate, punchCooldown, punchDuration, cursorRadius
- [x] `ClickerConfig` - chance (default 300), key
- [x] `HealthConfig` - maxHealth, regenRate, damageCooldown
- [x] `AcidConfig` - spinSpeed, honkInterval, rotationTotal, triggerChance
- [x] `RainbowConfig` - hueSpeed (default 120.0f)
- [x] `BallConfig` - count, size, speed, friction
- [x] `BreadCrumbsConfig` - maxCrumbs, lifetime, spawnDist, size
- [x] `HatsConfig` - path, sizeX, sizeY, offsetX, offsetY

**Menu & GUI:**
- [x] Behaviors menu with Fun/Control/Info/Systems submenus
- [x] Config GUI for behavior parameters (Settings > Open Config GUI)
- [x] Open Color Picker menu item

**In Progress:**
- (none)

**Completed Behaviors (all implemented):**
- [x] AI - Chat with goose via AI (basic chat window, OpenAI integration ready)
- [x] Color Picker - Change goose color via macOS color panel

**Completed:**
- [x] Menu items for behavior toggles (Behaviors > Fun/Control/Info/Systems submenus)
- [x] Config GUI for behavior parameters (Settings > Open Config GUI)

### Architecture

**Starter Pack Defaults**
Behaviors enabled by default on fresh install: Ball, Rainbow

**Component-Based Behavior Registry**

```cpp
struct Behavior {
    const char* id;           // unique identifier (e.g., "ball")
    const char* name;          // display name (e.g., "Ball")
    bool* enabledPtr;         // pointer to config flag

    // C++11 std::function with Goose* context (not void* function pointer)
    using InitFunc = std::function<void(BehaviorContext&)>;
    using TickFunc = std::function<void(Goose*, BehaviorContext&, double dt, double time)>;
    using RenderFunc = std::function<void(Goose*, BehaviorContext&, void* ctx)>;
    using CleanupFunc = std::function<void(BehaviorContext&)>;

    InitFunc init;             // called on startup if enabled
    TickFunc tick;            // called each frame with dt
    RenderFunc render;        // called during render pass
    CleanupFunc cleanup;      // called on goose removal

    const char** conflicts;
    int priority;
    Config config;            // accessibility/network requirements
};
```

**Per-Entity State Management**
Each goose gets its own behavior state via `BehaviorStateManager`:

```cpp
class BehaviorStateManager {
    // Singleton instance
    static BehaviorStateManager& Instance();

    // Get or create state for a specific goose + behavior
    template<typename T>
    T* GetOrCreate(Goose* goose, const char* behaviorId);

    // Thread-safe state retrieval
    template<typename T>
    T* Get(Goose* goose, const char* behaviorId);

    // Cleanup when goose is removed
    void RemoveForGoose(int gooseId);
};

// Example: Per-goose jail state
auto* jailState = BehaviorStateManager::Instance().GetOrCreate<JailState>(goose, "jail");
jailState->jailPos = position;
jailState->isJailed = true;
```

**Accessibility & Failsafe**

```cpp
class AccessibilityManager {
    bool IsProcessTrusted();                              // AXIsProcessTrusted()
    bool CanEnableBehavior(const char* behaviorId);      // checks + prompts
    void ShowPermissionDeniedAlert(const char* id);      // system settings link
};

// Failsafe hotkey: Cmd+Shift+Up (runs independently of event loop)
void StartFailsafeHotkey();   // Global monitor on separate thread
void StopFailsafeHotkey();

// Behavior enablement with accessibility check
bool CheckAccessibilityForBehavior(const char* behaviorId);
```

**Config Organization**
```toml
struct Config {
    // ... other config structs ...

    struct PortalConfig { float p1Width = 80.0f; float p1Height = 80.0f; float p2Width = 80.0f; float p2Height = 80.0f; } portal;
    struct AIConfig { std::string endpoint; std::string model = "gpt-3.5-turbo"; std::string keychainService; } ai;
    struct GooseManagerConfig {
        bool taskWander = true, taskFetch = true, taskChase = true, taskSnatch = true;
        bool speedWalk = true, speedRun = true;
    } gooseManager;

    struct BehaviorConfig {
        struct Fun {
            bool ball = false;
            bool breadCrumbs = false;
            bool hats = false;
            bool rainbow = false;
            bool acid = false;
            bool anger = false;
        } fun;

        struct Control {
            bool honcker = false;
            bool jail = false;
            bool portals = false;
            bool drag = false;
            bool banish = false;
        } control;

        struct Info {
            bool nametag = false;
            bool debugoose = false;
            bool presence = false;
            bool configGUI = false;
            bool colorPicker = false;
            bool clicker = false;
            bool gooseManager = false;
        } info;

        struct Systems {
            bool health = false;
            bool ai = false;
        } systems;

        // Behavior-specific settings
        struct BallConfig { int count = 5; float size = 25.0f; float speed = 300.0f; float friction = 0.98f; } ball;
        struct HatsConfig { std::string path; float sizeX = 32.0f; float sizeY = 24.0f; float offsetX = 0.0f; float offsetY = -15.0f; } hats;
        struct BreadCrumbsConfig { int maxCrumbs = 50; float lifetime = 10.0f; float spawnDist = 15.0f; float size = 5.0f; std::string triggerKey = "RightShift"; } breadCrumbs;
        struct ClickerConfig { int chance = 300; int key = 0x24; } clicker;
        struct HonckerConfig { int key = 0x24; } honcker;
        struct DragConfig { float radius = 45.0f; } drag;
        struct JailConfig { int keyO = 0x2D; int keyP = 0x1E; float size = 150.0f; } jail;
        struct AngerConfig { float increaseRate = 15.0f; float decreaseRate = 8.0f; float punchCooldown = 2.0; float punchDuration = 0.3f; float cursorRadius = 100.0f; } anger;
        struct HealthConfig { float maxHealth = 100.0f; float regenRate = 0.5f; float damageCooldown = 2.0f; } health;
        struct AcidConfig { float spinSpeed = 720.0f; float honkInterval = 0.15f; float rotationTotal = 1080.0f; int triggerChance = 300; } acid;
        struct RainbowConfig { float hueSpeed = 120.0f; } rainbow;
    } behaviors;
};
```

**Security**
- AI API key stored in macOS Keychain, not config.toml
- Config references keychain service name: `aiKeychainService = "com.cadgoose.ai"`

**Config Organization**
- Config stored in TOML format (`config.toml`) using toml11 library
- Per-behavior config files in `config/behaviors/*.toml`
- Config file locations documented in README.md

**Permissions**
- Clicker behavior requires Accessibility permission (Accessibility API)
- Check permission status on enable; prompt user if denied, link to System Preferences

**Versioning**
- Add `behaviorConfigVersion` to detect stale configs
- Warn user if config version < binary version (behavior may have changed)

### Deterministic Physics Normalization

All spatial and rotational logic **must use dt-scaled operations**. Frame-locked logic breaks on variable refresh rate displays (ProMotion 120Hz).

**Rules:**
1. Rotation: `angle += degreesPerSecond * dt;` (NOT `angle += 4.0f;`)
2. Velocity decay: `vel *= pow(friction, dt * 60.0f);` (NOT `vel *= friction;`)
3. Gravity: `vel.y += gravity * dt;` (NOT `vel.y += gravity;`)

**Helper macros:**
```cpp
#define DT_SCALED_ROTATION(current, degPerSec, dt) (current + (degPerSec) * (dt))
#define DT_SCALED_VELOCITY(vel, friction, dt) (vel * std::pow(friction, dt * 60.0f))
```

**Example - Acid rotation refactor:**
```cpp
// BEFORE (frame-locked, breaks at 120Hz):
void tick(double dt) { goose->direction += 4.0f; }  // 4° per frame = 240°/sec @ 60fps
                                                      // but 480°/sec @ 120fps!

// AFTER (deterministic, works at any refresh rate):
void tick(Goose* goose, BehaviorContext&, double dt, double time) {
    goose->direction += 240.0f * dt;  // 240°/sec regardless of frame rate
}
```

### UX Guidelines

1. **Starter Pack**: Ship with Fun group behaviors enabled by default (Ball, Rainbow)
2. **Grouped Menu**: Organize by user intent, not implementation complexity
3. **On-Enable Feedback**: Show toast notification explaining how to use newly-enabled behavior
4. **Conflict Detection**: If enabling conflicts with running behavior, show confirmation dialog

### Testing Requirements

Each behavior requires:
- **Unit test**: Core logic (e.g., Ball physics, Portal teleport math)
- **Integration test**: Behavior with goose (e.g., goose kicks ball)
- **UI test**: Menu toggle persists across restart

---

## ResourceHub Behaviors Implementation Plan

### Easy (1-2 days) - Direct Config/Feature Addition

| Behavior | Group | How to Use | Key Details |
|-----|------|------------|-------------|
| **Honcker** | Control | Press **F** key | Triggers `Audio_PlayHonk()`. Cooldown: 0.5s. |
| **Nametag** | Info | Always visible when enabled | `nametag` field on Goose struct. Renders above head in device coords. Config: `nametagText`, `nametagColor` (hex). |
| **Color Picker** | Info | Dialog on launch (or from menu) | Show `NSColorPanel`. Store as RGB hex in `gooseColor`. Applies to all geese. |
| **Config GUI** | Info | Menu → Behaviors → Config GUI | Opens modal window with sliders for movement, rendering, behavior settings. Changes write to config.toml. |
| **Debugoose** | Info | Always visible when enabled | Shows 10 most recent `UiLogPush()` entries + writes all to `log.txt`. Renders as overlay in top-left. |
| **GooseManager** | Info | Edit `GooseTasks.txt` or config directly | Disables behaviors by state. `taskWander`, `taskFetch`, `taskChase`, `taskSnatch` flags. `speedWalk`, `speedRun` flags. **Conflicts**: If all tasks disabled, force-enable all. |

### Medium (3-5 days) - New Gameplay Mechanics

| Behavior | Group | How to Use | Key Details |
|-----|------|------------|-------------|
| **Ball** | Fun | Mouse cursor kicks ball | `Ball` struct in `world.h`. Supports three ball types: **Soccer** (white with black pentagon pattern), **Beach** (colorful stripes), and **Generic** (blue). Configurable size, speed, bounce factor, and friction for each type. Physics: velocity, bounce, friction. Collision: cursor (radius kick), goose feet (direction kick). Render: `CGContextFillEllipseInRect` with `ball.size` config. |
| **BreadCrumbs** | Fun | **Right Shift** throws crumbs at cursor | Particle system: 5-8 crumb particles per throw. Particles follow parabolic arc (gravity). Goose chases nearest crumb, calms if in frenzy. Sound: nom. |
| **Rainbow** | Fun | Always visible when enabled | Hue cycles 0→360° at configurable speed (`rainbowSpeed`). Render: multiply base color by HSL conversion. **Starter enabled**. |
| **Hats** | Fun | Always visible when enabled | `hatImage` on Goose (CGImageRef). Config: `hatPath` (load from file), `hatSizeX`, `hatPosX/Y` offset from head. Default hats in `Assets/Hats/`. |
| **Clicker** | Info | Triggers automatically when enabled | Timer: random interval between `clickerIntervalMin/Max`. Uses `CGEventCreateMouseEvent` at current cursor. Requires Accessibility permission. Fallback: warn if no permission. |
| **Jail** | Control | **O** = set jail position, **P** = trap goose | `jailPos` (Vector2), `jailRadius` (default 80). When trapped: goose wanders inside circle. Sad animation (slower movement). Press P again to release. |
| **OnePunch** | Control | Triggered when goose anger maxes | Anger builds when: cursor moves away during chase, goose can't catch cursor. Max anger → punch cursor (cursor flies off screen), spawn particles. Config: `angerTimeToMax` (default 10s), `punchForce`. |
| **Portals** | Control | **P+1** = portal A, **P+2** = portal B, **P+0** = toggle off | `Portal` struct: position, active. Cursor entering portal A exits portal B (and vice versa). Goose carrying meme through portal: meme teleports too. |
| **Banish** | Control | **Ctrl+Alt+Middle Click** | Goose position → center of screen, triggers fade-out animation (0.5s). Respawns after `banishRespawnTime` (default 30s) at random edge. |
| **Acid** | Fun | Always visible when enabled | Continuous rotation (`direction += 4°/frame`). Honk every 700ms. Confuses goose AI (random target). **Use with caution** - disorienting. |
| **Presence** | Info | Always visible when enabled | Discord RPC: show "Playing with CadGoose" + current behavior state. Fallback: menu bar tooltip shows goose state. Requires `discord-rpc` library or simplified menu bar status. |

### Complex (1-2 weeks) - Major New Systems

| Behavior | Group | How to Use | Key Details |
|-----|------|------------|-------------|
| **Health** | Systems | Damage from mouse clicks on goose | Health bar floats above goose. Damage triggers: middle-click goose (5), cursor grab (2/s), bite fail (3). Config: `healthMax` (default 100), `healthRegen` (1/s when idle). |
| **Drag** | Control | **Click and drag** goose with mouse | Drag state: `dragPos`, `dragVel`. Goose may resist: random chance to break free if dragged too fast. Success: goose panics (faster movement). Bite if drag fails. |
| **AI** | Systems | Click goose → chat opens | Opens text input panel. Sends to `aiEndpoint` (OpenAI-compatible). Response displayed as speech bubble. Config: `aiEndpoint`, `aiModel`, `aiKeychainService`. |

### Implementation Order (by UX priority)

1. **Honcker** - immediate feedback, low complexity
2. **Rainbow** - visual delight, minimal risk
3. **Ball** - core gameplay loop, high engagement
4. **Hats** - simple render layer, fun customization
5. **Nametag** - simple visual feature
6. **BreadCrumbs** - particle system practice
7. **Acid** - easy chaos mode
8. **Jail** - movement constraint
9. **Portals** - teleport logic
10. **OnePunch** - anger/reaction system
11. **Drag** - mouse interaction
12. **Banish** - emergency removal
13. **Clicker** - cursor control practice
14. **Presence** - Discord RPC
15. **Config GUI** - developer tooling
16. **Color Picker** - developer tooling
17. **GooseManager** - fine-grained control
18. **Health** - health bar system
19. **AI** - async AI integration

### Behavior Feature Architecture

All behaviors must be gated behind:
1. **Config flag** in `BehaviorConfig` struct (`config.h`)
2. **Menu item** in "Behaviors" submenu (`main.mm`)
3. **Toggle action** method in `AppDelegate`
4. **Feature logic** in appropriate game systems

Each behavior registered in BehaviorRegistry with init/tick/render hooks. See Phase 4 Architecture above for full details.

**Coordinate System**
All behaviors operate in **world coordinates**. Use `WorldCoord` helpers from `world.h` for conversions:

```cpp
// Screen (device) → World
Vector2 screenPos = GetMousePosition(); // in device coords
Vector2 worldPos = screenPos / g_config.general.globalScale;

// World → Screen (device)
Vector2 devicePos = WorldCoord::ToDevice(worldPos, goose);

// Common patterns:
WorldCoord::ToDevice(worldPos)                    // generic
WorldCoord::GoosePos(goose)                      // goose position in device
WorldCoord::RigNeckHead(goose)                   // neck head in device
WorldCoord::DeviceSize(pixels)                   // size in device coords
WorldCoord::ItemCenter(item)                     // item center in device
```

This is critical for behaviors like Ball, Portals, BreadCrumbs that interact with cursor position or goose body parts.

**Multi-Goose Support**
All behaviors operate per-goose. Loop over `g_geese` list:
```cpp
for (Goose& g : g_geese) {
    if (g.behaviorEnabled) {
        // per-goose behavior logic
    }
}
```
Use `g.id` to track per-goose state if needed.

**Existing Patterns to Follow**
- `LeafPile` / `Leaf`: Reference for particle systems (BreadCrumbs, OnePunch particles)
- `Footprint`: Reference for spawn-on-ground objects (Jail cage)
- `DroppedItem`: Reference for draggable world objects (Ball, Portals)

### Implementation Files

| File | Description |
|------|-------------|
| `include/behavior.h` | Behavior structs, BehaviorStateManager, BehaviorRegistry |
| `src/common/behavior.cpp` | Physics helpers, state management, DT-scaled operations |
| `src/platform/macos/behavior_mac.mm` | AccessibilityManager, FailsafeHotkeyMonitor, cursor protection |
| `tests/common/test_behavior.cpp` | 51 unit tests (all passing) |

### Behavior Registration Pattern

```cpp
// behaviors.h
class BehaviorRegistry {
public:
    static BehaviorRegistry& Instance();
    void Register(Behavior& behavior);
    void InitAll(Goose* goose);
    void TickAll(Goose* goose, double dt, double time);
    void RenderAll(Goose* goose, void* ctx);
    Behavior* Get(const char* id);
};
#define REGISTER_BEHAVIOR(b) /* static init registration */

// behaviors_ball.cpp
static bool s_enabled = false;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BallState>(ctx.goose->id, "ball");
    state->balls.resize(g_config.behaviors.ball.count);
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BallState>(goose->id, "ball");
    for (auto& ball : state->balls) {
        UpdateBallPhysics(ball, g_screenWidth, g_screenHeight, ctx.globalScale, dt);
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().Get<BallState>(goose->id, "ball");
    if (!state) return;
    for (auto& ball : state->balls) {
        // Draw ball at WorldCoord::ToDevice(ball.pos, goose)
    }
}

static Behavior g_ballBehavior = {
    .id = "ball",
    .name = "Ball",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};
REGISTER_BEHAVIOR(g_ballBehavior);
```

### Menu Structure (add to main.mm)

```
CadGoose Menu
├── About CadGoose
├── ─────────────
├── Spawn Goose (⌘G)
├── Clear All Geese
├── ─────────────
├── Honk! (⌘H)
├── ─────────────
├── Settings ▶
│   ├── Enable Mud Footprints
│   ├── Enable Cursor Chase
│   ├── Enable Memes
│   ├── Canada Goose Colors
│   ├── ─────────────
│   ├── Open Configuration
│   └── Reload Configuration
├── Behaviors ▶                    ← GROUPED BY USER INTENT
│   ├── Fun ▶
│   │   ├── Ball                      ✓ (starter)
│   │   ├── BreadCrumbs
│   │   ├── Hats
│   │   ├── Rainbow                   ✓ (starter)
│   │   └── Acid
│   ├── Control ▶
│   │   ├── Honcker
│   │   ├── Jail
│   │   ├── Portals
│   │   ├── Drag
│   │   ├── Banish
│   │   └── OnePunch
│   ├── Info ▶
│   │   ├── Nametag
│   │   ├── Debugoose
│   │   ├── Presence
│   │   ├── Config GUI
│   │   ├── Color Picker
│   │   ├── Clicker
│   │   └── GooseManager
│   └── Systems ▶
│       ├── Health
│       └── AI
├── ─────────────
└── Quit (⌘Q)
```

All behaviors ported from Desktop Goose ResourceHub must include attribution in README.md per the original mod authors.
