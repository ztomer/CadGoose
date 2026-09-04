# MCP (Model Context Protocol) — AI & External Control

CadGoose exposes an MCP server over a Unix socket **and** HTTP, allowing AI assistants,
external tools, and the built-in AI chat to control the goose at runtime.

---

## Transport

| Property | Value |
|---|---|
| Protocol | JSON-RPC 2.0 |
| Unix socket | `/tmp/desktop-goose-mcp.sock` (newline-delimited messages) |
| HTTP | `http://localhost:31072` (configurable via `AI.mcp_port`) |
| Stdio | `CadGoose --mcp` |

Both transports serve the same JSON-RPC methods. The HTTP server also returns
`Access-Control-Allow-Origin: *` for browser-based MCP clients.

---

## Protocol Endpoints

Send these as the `method` field of a JSON-RPC 2.0 request:

| Method | Description |
|---|---|
| `initialize` | Handshake; returns protocol version, server info, capabilities |
| `notifications/initialized` | Acknowledge; no response |
| `tools/list` | List all 12 available tools with descriptions and input schemas |
| `tools/call` | Execute a tool by name with arguments |
| `resources/list` | List all 5 resource URIs |
| `resources/read` | Read a resource by URI |

---

## Tools

### Socket-dependent (require the app to be running)

| Tool | Required args | Optional args | Description |
|---|---|---|---|
| `spawn_goose` | — | `name` (string) | Spawn a new goose; give it an optional name |
| `clear_geese` | — | — | Remove all geese from the desktop |
| `honk` | — | — | Make a goose honk |
| `fetch` | — | `type` ("meme" or "text") | Make a goose pick up an item |
| `goose_status` | — | — | Get status of the goose system |
| `open_preferences` | — | — | Open the preferences window |
| `send_chat` | `message` (string) | — | Send a chat message to the goose AI |
| `enable_behavior` | `id` (string) | — | Enable a behavior (e.g. "ball", "hats", "rainbow") |
| `disable_behavior` | `id` (string) | — | Disable a behavior |

### In-process (work without the app running)

| Tool | Required args | Optional args | Description |
|---|---|---|---|
| `get_config` | — | `key` (string) | Return all config JSON, or info about a specific key |
| `set_config` | `key` (string), `value` (string/number/bool) | — | Set a configuration toggle |
| `set_hotkey` | `hotkey` (string), `value` (string) | — | Change a behavior hotkey |

---

## Config Keys (25 toggles)

Use with `get_config`/`set_config`:

| Key | Default | Description |
|---|---|---|
| `ball_enabled` | false | Push balls around |
| `breadcrumbs_enabled` | false | Leave breadcrumb trails |
| `hats_enabled` | false | Put hats on geese |
| `rainbow_enabled` | false | Rainbow color cycling |
| `acid_enabled` | false | Wild spinning + honks |
| `anger_enabled` | false | Angry goose mode |
| `autumn_leaves_enabled` | true | Piles of leaves accumulate |
| `avoidance_enabled` | true | Goose dodges fast cursor |
| `boredom_enabled` | false | Sighs after 10+ min idle |
| `peeking_enabled` | true | Peeks from screen edges |
| `interactive_drops_enabled` | false | Puddles and flowers |
| `toys_enabled` | false | Scatter interactive toys |
| `honcker_enabled` | false | Honk on key press (F) |
| `jail_enabled` | false | Jail mode (O/P) |
| `portals_enabled` | false | Portal placement (1/2/0) |
| `drag_enabled` | false | Click-and-drag goose |
| `nametag_enabled` | false | Name label above goose |
| `presence_enabled` | false | Menu bar presence |
| `config_gui_enabled` | false | Preferences window |
| `visible_enabled` | true | Goose visible on screen |
| `health_enabled` | false | Health bar system |
| `ai_enabled` | false | AI chat |
| `pomodoro_enabled` | false | Work/rest timer |

---

## Hotkey Fields (8)

Use with `set_hotkey`:

| Field | Default | Behavior |
|---|---|---|
| `honcker_hotkey` | `f` | Honk |
| `jail_hotkey_o` | `o` | Save jail position |
| `jail_hotkey_p` | `p` | Trap goose in jail |
| `banish_hotkey` | `b` | Banish goose |
| `portal_hotkey_0` | `right shift` | Portal modifier |
| `portal_hotkey_1` | `1` | Portal 1 |
| `portal_hotkey_2` | `2` | Portal 2 |
| `breadcrumbs_hotkey` | `right shift` | Breadcrumb trail |

---

## Resources

Read config sections via `resources/read`:

| URI | Content |
|---|---|
| `config://behaviors` | Full config JSON (all toggles + hotkeys) |
| `config://behaviors/fun` | Fun behavior toggles |
| `config://behaviors/control` | Control behavior toggles |
| `config://behaviors/info` | Info behavior toggles |
| `config://behaviors/systems` | System behavior toggles |

---

## AI Chat Function Calling

When the **Enable MCP Server** toggle is on in Preferences > AI, the built-in AI chat
includes all MCP tools as OpenAI-compatible function definitions in every chat request.
This lets the LLM call goose tools directly during conversation.

**How it works:**

1. Your message is sent to the LLM with all 12 tool definitions
2. If the LLM decides a tool is needed, it responds with a `tool_calls` array
3. Each tool call is executed locally via `MCP_CallTool()`
4. The tool result is fed back to the LLM as a `"role": "tool"` message
5. The LLM produces a final text response displayed in the chat window
6. A recursion guard (max 5 turns) prevents runaway tool loops

**Example conversation:**

| You type | AI sees | AI does |
|---|---|---|
| "enable ball mode" | `enable_behavior` tool | HONK! Ball behavior enabled! |
| "what's your status?" | `goose_status` tool | Shows goose state, then replies as a goose |
| "spawn a goose named Gerald" | `spawn_goose` tool | Spawns Gerald, says hi |
| "turn on hats and rainbow" | two `set_config` calls | Enables both, describes the fabulous goose |

If the LLM doesn't support function calling or returns no tool calls, the fallback
chain still works: MCP bridge → Foundation keyword matching.

---

## AI Chat Natural-Language Commands

When the AI chat assistant cannot reach its model or function calling is off, it falls
back to a keyword bridge that maps natural-language messages to MCP tools.

| Say this… | …and the goose does this |
|---|---|
| `enable <behavior>` / `turn on <behavior>` | Enable that behavior |
| `disable <behavior>` / `turn off <behavior>` | Disable that behavior |
| `honk` | HONK! |
| `spawn` / `spawn <name>` | Spawn a goose (optionally named) |
| `clear geese` / `remove geese` | Clear all geese from the desktop |
| `status` / `report` / `goose status` | Show goose system status |
| `open preferences` / `show settings` / `open config` | Open the preferences window |
| `fetch` / `fetch meme` / `fetch text` | Make a goose fetch something |

### What won't trigger a command

The bridge only matches the **first word** of your message. Normal sentences are safe:

| You type | Result |
|---|---|
| "please enable ball" | Not matched (first word is "please") |
| "I want to honk" | Not matched (first word is "i") |
| "tell me a joke" | Not matched (not a command verb) |
| "you should disable ball" | Not matched (first word is "you") |

---

## Fallback Chain

When you send a message, the AI chat follows this priority chain:

1. **LLM with function calling** (if MCP enabled + LLM supports tools) — the LLM can invoke any MCP tool directly
2. **LLM text response** (if function calling unavailable or not triggered) — the LLM responds as a goose
3. **MCP bridge** (on LLM connection error) — natural-language commands like "enable ball" are routed to MCP tools
4. **Keyword fallback** — last-resort keyword matching for common phrases (greetings, emotions, etc.)

If the LLM returns a connection error and the MCP bridge can't handle the message, the error
text is displayed in the chat window so you know the server is unreachable.

---

## Connection Health Check

When the chat window opens, `AIHTTPClient` sends a lightweight health check to the LLM endpoint:

- **Request**: `POST /v1/chat/completions` with `{"model":"...","messages":[{"role":"user","content":"ping"}],"max_tokens":1,"temperature":0}`
- **Timeout**: 5 seconds
- **Success**: status bar shows the model name (normal operation)
- **Failure**: status bar shows `[!] Can't connect: <reason>` (e.g., connection refused, timeout, HTTP error)

The health check does not modify the chat history and costs a minimal token for a single-token
response. It runs once when the window opens; subsequent messages use the normal send flow.

---

## Think Block Stripping

Some models (Gemma, Qwen with CoT, etc.) wrap reasoning in `<think>...</think>` tags within
the `content` field. The AI chat automatically strips these blocks from *all* LLM responses:

| Input | Output |
|---|---|
| `<think>I should answer kindly</think>Hello!` | `Hello!` |
| `<think>\nlong reasoning\n</think>\nHi there` | `Hi there` |
| `No thinking here` | `No thinking here` |

**Edge cases:**
- Multiple `<think>` blocks: all are stripped
- Unclosed `<think>` tag: left as-is (no stripping)
- Only `<think>` block with no actual content: original text preserved (content not empty)
- Empty input: returned as-is

Stripping uses `NSRegularExpression` with `DotMatchesLineSeparators` to handle
multi-line reasoning. It runs after receiving the LLM response, so the clean text
is stored in chat history and displayed to the user.

---

## Transport Architecture

```
┌─────────────────┐     HTTP (port 31072)     ┌──────────────┐
│  External MCP    │ ─────────────────────── → │              │
│  clients (curl,  │                           │  MCP Server  │
│  browser, etc.)  │     Unix socket           │  (JSON-RPC)  │
│                  │ ─────────────────────── → │              │
└─────────────────┘     /tmp/desktop-goose-    └──────┬───────┘
                         mcp.sock                      │
                                                       │ MCP_CallTool()
                                                       ↓
┌─────────────────┐                            ┌──────────────┐
│  AI Chat Window  │     HTTP POST              │  Goose Core  │
│  (behavior_ai)   │ ────────────────────────→  │  (behaviors, │
│                  │     /v1/chat/completions   │   config)    │
│  AIHTTPClient    │ ←────────────────────────  │              │
│  (ai_http_client)│     JSON response          └──────────────┘
└─────────────────┘
```

The AI chat connects to the LLM server **only over HTTP**. The MCP server exposes
both HTTP (port 31072) and Unix socket (`/tmp/desktop-goose-mcp.sock`) for external
clients. The AI chat's MCP integration uses `MCP_CallTool()` directly (in-process),
not through either transport.

---

## Example Usage

### HTTP (curl)
```bash
curl -X POST http://localhost:31072 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list"}'
```

### List tools (JSON-RPC over Unix socket)
```bash
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | nc -U /tmp/desktop-goose-mcp.sock
```

### Enable ball behavior
```json
{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"enable_behavior","arguments":{"id":"ball"}}}
```

### Get all config
```json
{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"get_config","arguments":{}}}
```

### Read fun behaviors resource
```json
{"jsonrpc":"2.0","id":4,"method":"resources/read","params":{"uri":"config://behaviors/fun"}}
```
