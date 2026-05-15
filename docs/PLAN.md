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
- Joy & Delight Features (pet goose, cursor avoidance, nighttime mode, weekend vibes, idle preening, boredom sigh, window peeking, custom affirmations, interactive drops, AI typing sounds)
- All-Behavior Profile (MEM) - vmmap + sample with all behaviors shows stable memory (~468.8M RSS)
- AI Text Meme — Full Integration Test - Osaurus server integration verified, memes generated and saved to queue
- Memory Allocator Investigation - mimalloc vs system malloc evaluated (~14MB savings with mimalloc), USE_MIMALLOC CMake option added
- Unwired Joy Features Audit - verified all joy feature toggles properly wired in code
- MTLCompilerService Bundle Crash - documented (requires Apple Developer signing + hardened runtime + com.apple.security.cs.allow-jit entitlement; workaround: use ./build/CadGoose directly)
- Additional UI Improvements - Added JOY category to Preferences → Behaviors panel with individual toggles; fixed separator crash using layer-backed approach

## ACTIVE PLAN (2026-05-15)

### Preferences Panel
- [x] **Fix AI provider selection** — Foundation→CoreML, Osaurus→osaurus, Ollama→ollama, Custom→custom
- [x] **Model switching reflected in AI chat** — refreshConnection() on window focus
- [x] **Move goose preview to the right of color selectors**
- [x] **Reorganize Behaviors tab layout** — Increased row height (44→52px), padding (12→16px), description width
- [x] **Remove yellow/green traffic lights** from preferences panel
- [x] **Sonic/Toys toggles** — Added sonicMode and toysEnabled to config + UI + tests

### Removed Features
- [x] Weekend mode (removed)
- [x] Petting behavior (removed, kept avoidance)
- [x] Nighttime mode (removed)
- [x] AI typing sounds (removed)
- [x] Banish (completely removed)
- [x] Idle Preening (completely removed)

### AI Chat
- [x] **Remove empty space between appbar and chat text**
- [x] **Add liquid glass effects to appbar**

### General
- [x] **Fix freeze on startup** — Already async
- [x] **Fix goose getting stuck in place** — Stuck detection (5px/3s → new target)
- [x] **Pomodoro rest mode improvements** — Walk to corner, sleep with bed + ZZZ animation
- [x] **Regression: Goose stopped fetching memes and texts** — Added detailed [FETCH] logging
- [x] **Add mute option** — Apple B&W icons after Honk! in menubar
- [x] **Local model detection bug on macOS 26.5** — Added logging + 4 fallback paths + config search paths
- [x] **Persist geese names across sessions** — RingBuffer<std::string, 10>
- [x] **Fix portals disappearing** — Removed state->Reset() from init()

### Code Quality
- [x] **No hardcoded values** — Extracted magic numbers to named constants in headers
- [x] **No files above 500 LOC** — Split local_llm.mm into tokenizer/model/inference modules
- [x] **Prompts in TOML** — All AI prompts moved to Assets/prompts.toml
- [x] **Local LLM search paths in config** — Added localLlmSearchPaths vector to AIConfig

### Ports/Integrations
- [x] **Port Sonigoose mod** — Analyzed, most features already exist
- [x] **Port Toys mod** — Bed assets downloaded, integrated into pomodoro rest
- [x] **Reevaluate breadcrumbs behavior** — Compared with DLL, CadGoose has improvements

## BACKLOG

All planned items from this historical plan have been completed. See [AGENTS.md](/AGENTS.md) for current project state and active development focus.
