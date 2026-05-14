# Plan

## PHASE 1: Memory Growth ✅ DONE
- **Fixes**: @autoreleasepool in tick, CoreText object caching (pomodoro/nametag/jail), hat bitmap caching
- **Results**: CoreAnimation dirty 3.9MB→192KB (95%↓), allocation count 306K→124K (60%↓), RSS stable

## PHASE 2: CI/CD Pipeline ✅ DONE
- Created `.github/workflows/pr_check.yml` — PR build check (Linux + macOS, builds + runs tests)
- Updated `build_and_release.yml` — graceful bundle skip, test step, caveat comments

## PHASE 3: macOS Bundle Crash (LOW PRIORITY)
Bundled `.app` crashes with "Unable to reach MTLCompilerService" on macOS 26.5 (M4 Max) — likely ad-hoc signing limitation. Non-bundled `./build/CadGoose` runs fine.

## OPEN QUESTIONS (user feedback needed)

### Memory Allocator Investigation (LOW)
Evaluate mimalloc/tcmalloc vs system malloc for reduced fragmentation.

### All-Plugins Performance Profile (MED)
Rerun memory & CPU profiles with all 16 behaviors enabled.

### Breadcrumbs Interaction (MED)
Crumbs are purely visual markers — goose doesn't interact. RightShift vs `i` key mapping issue. Needs user clarification: what behavior is desired?

### Unwired Joy Features (HIGH)
Audit features/config toggles vs actual behavior code paths. Some may be registered but not producing observable effects.

### Fetch stopped working (FIXED)
**Root cause**: `fetchEdgeMargin=80` > `screenClampExpanded=50` — goose clamped before reaching off-screen fetch target, permanently stuck in FETCHING.
**Fixes**: ClampToScreen now uses `max(screenClampExpanded, fetchEdgeMargin)`, plus FETCHING timeout safety net.
**Tests**: 3 new regression tests in `test_goose_behavior.cpp`.
