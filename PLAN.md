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

- Add interactable items: soccer balls, beachball.
- Implement Pomodoro timer mode (e.g., wanders for 25 minutes, then attacks constantly for 5 minutes, repeat). This should be an explicit mode toggle. Menu bar icon should rotate clockwise and complete a full rotation during the work/rest duration.
- Explore and implement mods from ResourceHub, remember to add attribution in the readme.md file:
  - RainbowStrobe
  - Shaggy's Config Menu
  - Shaggy's Nametag Mod
  - SizzurpMods
  - Honcker
  - Color Picker
  - Goose Acid
  - Goose hatgoos
  - DragGoose
  - GooseManager
  - BreadCrumbs
  - Debugoose
  - Sonigoose
  - Ball Toys
  - PortalGoos
  - Kamikagoose
  - GoosePresence
  - GooseJail
  - OnePunchGoose
  - Clicker
  - GooseAI

## 3. Overall Timeline and Priorities

- **Phase 1**: Fix bugs (Image and Text Placement) - High priority.
- **Phase 2**: Implement testing (Behavioral & Rendering) - Completed.
- **Phase 3**: Performance optimization - Skipped (Xcode instruments not suitable for CLI/unnecessary given current frame rate).
- **Phase 4**: Implement new QoL features and ideas.
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
- **Status**: Updated `Config_SaveNow()` in `config.cpp` to autogenerate section headers, descriptions, and valid ranges directly into `config.ini`.
