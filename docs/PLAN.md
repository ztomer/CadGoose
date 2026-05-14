# Plan

> **Note**: This plan is historical. See [AGENTS.md](/AGENTS.md) for current project state and active context.

## DONE ✅
- Memory growth fixes (@autoreleasepool, CoreText caching, hat bitmap caching)
- CI/CD pipeline (PR check + build/release workflows)
- macOS bundle crash documentation
- Fetch loop stuck fix (edge margin > screen clamp)
- AI text meme generator (paper canvas, cooldown fix, timeout 30→60s)
- Breadcrumbs render order (two-pass ground/overlay)
- Breadcrumbs eating mechanic (goose proximity removes crumbs + chewing animation)
- RightShift reference file fix
- Ring buffer for all unbounded containers
- Local CoreML LLM (direct MLModel integration)
- Two-pool text system (AI + file pool)
- Local LLM API contract tests (11 tests)
- Chewing/swallowing animation (split-beak open/close)

## BACKLOG

### Joy & Delight Features (from docs/JOY_SUGGESTIONS.md)
- **Pet the goose**: Wiggle cursor over head → closed eyes, heart, purr sound
- **Cursor avoidance**: Fast cursor toward goose → surprised reaction, dodge
- **Nighttime mode**: After 11 PM → slower walking, sleepy animations (yawn, nightcap)
- **Weekend vibes**: Friday afternoon → boombox, sunglasses
- **Idle preening**: No interaction for a few minutes → feather cleaning
- **Boredom sigh**: 10+ min idle → dramatic sigh, lie down
- **Window peeking**: At monitor edge → peek head around bezel
- **Custom affirmations**: Configurable positive message drops
- **Interactive drops**: Puddles that splash, flowers that grow
- **AI typing sounds**: Pitch-shifted honk noises per evil level during chat

### All-Behavior Profile (MEM)
Rerun vmmap + sample with all 16 behaviors enabled. Verify no growth.

### AI Text Meme — Full Integration Test
- Start Osaurus server, enable text meme, verify generated texts appear in queue
- Cycle through all available models

### Memory Allocator Investigation
Evaluate mimalloc/tcmalloc vs system malloc.

### Unwired Joy Features Audit
Audit features/config toggles vs actual behavior code paths.

### MTLCompilerService Bundle Crash
Only fixable with Apple Developer signing + hardened runtime + JIT entitlement.
