# Profiling Plan — 46% CPU + intermittent crash

Goal: find what's burning 46% CPU at idle, and catch the next crash
in the act.

## Tools (all native macOS, all free)

| Tool | What it gives | When to use |
|---|---|---|
| `top -pid <pid> -stats pid,command,cpu,th,ports` | Live %CPU, threads, ports | First sanity check |
| `sample <pid> 10 -file /tmp/sample.txt` | 10s of stack samples (no symbols required) | Cheap, no Xcode |
| Instruments → Time Profiler | Symbolicated CPU sampling with flame graph | The main investigation |
| Instruments → Allocations | Heap growth, leaks | If sample shows allocs in hot path |
| Instruments → System Trace | Wakeups, syscalls | If %CPU is wakeup-driven |
| `lldb -p <pid>` then `bt all` when frozen | Per-thread backtraces at the moment of trouble | For hangs |
| Console.app (Crash Reports → User) | Crash logs with stack + register state | After a crash |
| `MallocStackLogging=1` + `malloc_history` | Trace allocs of leaked / dangling addresses | Use-after-free triage |
| `ASAN` (CMake `-DENABLE_ASAN=ON`) | Catches UAF / OOB / double-free with backtrace | Once we have a repro |

## Phase order

### Phase A — characterize (≤5 min, no rebuild)

1. Launch CadGoose normally; let it sit idle for 30s with one goose.
2. `pidof CadGoose` (or grep ps).
3. `top -l 5 -pid <pid> -stats pid,cpu,th` — record CPU and thread
   count. We expect ~46%. If much lower at rest, the spike is
   feature-correlated.
4. Open the prefs window; toggle a few behaviors; record again.

This tells us whether the burn is constant or driven by a specific
behavior (e.g. autumn leaves spawning every 600 ticks * many leaf
particles).

### Phase B — cheap stack sample (≤2 min)

```
sample CadGoose 10 -file /tmp/cadgoose_sample.txt
```

Grep the result for hot frames. Look specifically for:

- `Goose::tick` / `Goose::Update` — expected, but how deep is the
  per-tick work?
- `SolveFeet` / `UpdatePhysics` — physics math at 60Hz
- `ActorManager::tickAll` / `renderAll` — actor loop
- `[GooseView drawRect:]` and CGContext-heavy paths
- `[EffectWindowManager syncWindows]` — O(N²) over actor+window list
- `[ItemWindowManager syncWindows]` — same shape
- `[BehaviorElementWindowManager …]`
- AI/HTTP threads (`ai_http_client`, `mcp_*`)
- `goose_drawing.mm::DrawGoose` — per-frame CG paint
- `BehaviorRegistry::TickAll` / `RenderAll`

Hypotheses going in:

1. **Per-frame allocation thrash.** Every behavior render allocates
   CGColorRef / CFAttributedString / CFRelease churn. The §3
   `GetImageSize` and `MeasureText` paths each construct and
   release a CTFont per call. With many active behaviors per goose
   that's a lot of CT allocs per frame.
2. **Footprint redraws every frame for every footprint.** Phase 6
   wired `g_time`; every footprint window now actually redraws on
   every tick (no early-out). With 500 cap, that's a lot of
   `drawRect` calls per second.
3. **`syncWindows` is O(actors * windows) per tick.** ItemWindow and
   EffectWindow managers both iterate every tick. With many
   dropped items / footprints this is N² per frame at 60Hz.
4. **NSWindow paint cost at full-screen.** Phase 4 made the
   `GooseWindow` screen-sized; if the entire window is redrawn each
   tick (even when nothing changed) the GPU/CPU has to repaint
   millions of pixels. Originally the window was 600x600 = 360k
   px; full-screen at 2560×1600 retina is 16M px — ~44× more.

The hot frames will discriminate between these. (3) was the
suspected leading cause until phase 4 landed; now (4) is the most
likely.

### Phase C — Time Profiler (10–15 min)

```
xcrun instruments -t "Time Profiler" -D /tmp/cg.trace -l 30000 \
    -p $(pgrep -x CadGoose)
open /tmp/cg.trace
```

Or attach via Instruments GUI. In the trace:

- Heaviest stack trace → tells us the top function.
- "Detail" pane → flame chart, look at width.
- Filter on `[GooseView drawRect:]` → see how much of the budget is
  the goose render.
- Filter on `[EffectWindow drawRect:]` → see how much is footprints
  alone.
- Switch to "Sample List" → look for unexpected callers (Foundation,
  CoreText, CoreGraphics — that signals over-allocation).

Compare two captures: one with footprints disabled (mud=off), one
on. Diff isolates footprint cost.

### Phase D — narrow with toggles

If Time Profiler points at a behavior or subsystem, validate by
toggling that subsystem off and re-running `top` / `sample`. A
specific subsystem owning ≥10 percentage points of CPU is a clear
target.

Suspected order to toggle off (cheapest reproduction first):

1. Autumn leaves
2. Mud footprints (`mud.enabled = false` in config)
3. Pomodoro
4. Hats / Rainbow / Acid (cosmetic per-frame work)
5. AI behaviors

### Phase E — crash investigation (#17)

While running Phase A–D we keep:

- **`ulimit -c unlimited`** before launch (core dumps).
- **Console.app open** filtering on `CadGoose` (live crash reports).
- **`lldb -p <pid>`** attached but detached unless we see a hang.

If a crash happens during profiling:

1. Grab the crash report from `~/Library/Logs/DiagnosticReports/`.
2. Symbolicate with `atos -p <pid> <addr>` against the same binary
   build.
3. If the symbol path doesn't have line numbers, rebuild with
   `-DCMAKE_BUILD_TYPE=RelWithDebInfo` and re-trigger.
4. If reproducible: rebuild with `cmake -DENABLE_ASAN=ON` (the flag
   already exists in CMakeLists.txt:8) and run again — ASan
   pinpoints UAF / OOB with the offending stack.

## Quick-win candidates worth attempting before any tool runs

- **GooseView::drawRect rate-limit.** Right now `setNeedsDisplay:YES`
  fires every tick regardless of state changes. Skip redraw when no
  goose moved more than 0.5 px and no behaviors mutated. This is a
  one-flag fix in `renderer.mm`.
- **GooseView region invalidation.** Use `setNeedsDisplayInRect:`
  with the union of each goose's bounding box rather than the full
  screen rect. Cuts redraw area dramatically for a few small geese
  on a 4K monitor.
- **EffectWindow::syncWindows early-out.** Skip phase 2 (new windows)
  entirely if `_lastSyncedTick == tickCount` and no actors flipped.
- **Cache `EffectContentView.setNeedsDisplay:` to only happen when
  the footprint's age band crosses a fade threshold** (5 visible
  bands × 60Hz = 300 redraws/sec vs 30000 currently for 500 prints).

If a Phase A/B sample matches one of these, fix it before going
deeper.

## Output

This doc captures the plan. Findings + fixes go into a new
`docs/PROFILING_RESULTS.md` after Phase B / C.

## Status

| Phase | Status |
|--:|---|
| A — characterize | TODO (no live instrumentation in this session) |
| B — sample | TODO |
| C — Time Profiler | TODO |
| D — toggles | TODO |
| E — crash | TODO |
| Quick wins | TODO — `setNeedsDisplay` rate-limit + region invalidation are tractable from code review alone |
