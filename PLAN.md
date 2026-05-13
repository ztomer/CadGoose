# CadGoose Plan

## Done — May 13

### Osaurus Integration Research
Probed Osaurus API (`localhost:1337`), built Python test harness, found root cause of qwen3.6 empty responses.

**Key findings:**
- Osaurus serves 9+ models via fully OpenAI-compatible `/v1/chat/completions`
- `foundation` and `gemma-4-e4b-it-8bit` work reliably at any temperature
- **qwen3.6-27b-mxfp4 has a thinking/reasoning mode** — at low temperatures it outputs all content via `reasoning_content` (stream-only field), leaving regular `content` empty in non-streaming responses
- ztools already handles this with model-specific quirks (prepend "Output JSON now.", adjust temp/timeout)

**Profile system implemented:**
- Added `ModelProfile` struct to `config.h` (pattern, temperature, maxTokens, timeoutSecs, hasReasoningContent, prependJsonTrigger)
- Built-in profiles in `behavior_ai.mm`:
  | Pattern | Temp | MaxTokens | Timeout | Reasoning | JSON Trigger |
  |---------|------|-----------|---------|-----------|-------------|
  | `qwen*` | 0.9 | 300 | 120s | yes | yes |
  | `foundation*` | 0.8 | 200 | 60s | no | no |
  | `gemma*` | 0.7 | 200 | 60s | no | no |
  | `nemotron*` | 0.7 | 200 | 60s | no | no |
  | `laguna*` | 0.8 | 200 | 60s | no | no |
  | `minimax*` | 0.8 | 200 | 60s | no | no |
  | default | 0.8 | 200 | 30s | no | no |

- Profiles matched by model name prefix (e.g., any model starting with "qwen" gets the qwen profile)
- `reasoning_content` fallback: if `content` is empty and profile has `hasReasoningContent=true`, falls back to `reasoning_content`
- `prependJsonTrigger`: prepends "Output JSON now.\n\n" to system prompts for qwen models
- Per-profile timeout prevents premature timeout on slow thinking models

**Build status:** 331 tests passing, 59 suites (all pass)

## Todo

### AI Provider Abstraction
- [ ] Replace `useOsaurus`/`useOllama` booleans with `ProviderType` enum
- [ ] Add `customEndpoint` + `customModel` fields for Custom provider
- [ ] Add unix socket transport option

### Bugs
- [ ] HatsBehavior.ImageScale test crash (segfault, pre-existing)
- [ ] Velocity acceleration too slow (350px/s²)

### UI
- [ ] "Open themes folder" button in Appearance tab
- [ ] Persist toggle state per-user in Behaviors tab
