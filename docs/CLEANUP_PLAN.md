# Code-review cleanup plan

From the deep scan (500-LOC / ring-buffers+leaks / Uncle Bob + Linus). Ordered
by value (highest first). The memory model is sound — no leaks found — so this
is maintainability/taste work, not bug fixing. Checked items are done.

## 1. Unified leveled logging  (HIGH — release noise + fragmentation)
123 raw `fprintf(stderr, ...)` plus four overlapping macros (`LOG`/`DEBUG_LOG`
duplicated in `main.mm` and `audio.mm`, `FETCH_LOG`, `DebugLog`). Release builds
spam stderr; there's no single level switch.
- [x] Add `include/log.h` + `src/common/log.cpp`: `CG_ERROR/WARN/INFO/DEBUG(tag, …)`,
      runtime `g_logLevel` (default Info; Debug via `--debug` / `CADGOOSE_VERBOSE`).
- [x] Wire `g_logLevel` in `main()`.
- [x] Convert the LLM/AI subsystem (the bulk of the noise): `local_llm_model.mm`,
      `local_llm_inference.mm`, `ai_text_meme.mm`, `ai_local_llm_adapter.mm`.
- [ ] (follow-up) Convert remaining files and retire the duplicate `DEBUG_LOG`/`LOG`.

## 2. RingBuffer hardening  (MED — prevents a future leak class)
`include/ring_buffer.h` is clean but: `push` copies twice, `Iter::count` is dead,
and it would silently leak if used with owning raw pointers.
- [x] Document the value-only ownership contract.
- [x] `push(const T&)` to avoid the double copy.
- [x] Remove the unused `Iter::count` field.

## 3. Decompose `Goose::SolveFeet`  (MED — worst single function, 91 lines)
Two near-identical halves (left/right foot IK).
- [x] Extract a per-foot `solveFoot()` helper.

## 4. CI: dedupe "Determine version"  (LOW)
Duplicated across the macOS and Linux jobs.
- [x] Factor the version logic so it isn't copy-pasted.

## Won't do now
- Global mutable state (`g_world`/`g_config`/`g_assets`/`g_time` + singletons):
  high churn, low payoff for a desktop toy. Noted for the record.
- Splitting `behavior_ai.mm`: the network client already lives in
  `ai_http_client.mm`; the rest is one cohesive chat-window concern.
