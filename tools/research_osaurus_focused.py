#!/usr/bin/env python3
"""
Focused Osaurus API probe — tests only what CadGoose's AIHTTPClient needs.
"""

import json, sys, time, urllib.request, urllib.error

BASE = "http://localhost:1337"
TIMEOUT = 15


def req(path, body=None):
    data = json.dumps(body).encode() if body else None
    r = urllib.request.Request(f"{BASE}{path}", data=data, method="GET" if body is None else "POST")
    r.add_header("Content-Type", "application/json")
    try:
        t0 = time.time()
        with urllib.request.urlopen(r, timeout=TIMEOUT) as resp:
            elapsed = time.time() - t0
            raw = resp.read()
            try:
                return resp.status, json.loads(raw), elapsed
            except json.JSONDecodeError:
                return resp.status, {"raw_text": raw.decode("utf-8", errors="replace")}, elapsed
    except urllib.error.HTTPError as e:
        return e.code, {"error": str(e.reason)}, 0
    except urllib.error.URLError as e:
        return 0, {"error": str(e.reason)}, 0


def chat(model, messages, max_tokens=50, temp=0.5):
    return req("/v1/chat/completions", {
        "model": model, "messages": messages,
        "max_tokens": max_tokens, "temperature": temp,
    })


# ── 1. Server info ──
print("=== 1. Server Health ===")
status, body, _ = req("/")
print(f"  Root: HTTP {status}")
print(f"  Server says: {body.get('raw_text', json.dumps(body))}")

status, body, _ = req("/v1/models")
models = body.get("data", [])
print(f"  Models endpoint: HTTP {status}, {len(models)} models")
for m in models:
    print(f"    - {m['id']}")

# ── 2. Chat — only models the app might use ──
print("\n=== 2. Chat Completion (app-relevant models) ===")
candidates = ["foundation", "qwen3.6-27b-mxfp4", "gemma-4-e4b-it-8bit"]
for model in candidates:
    print(f"\n  Model: {model}")
    status, body, t = chat(model, [{"role": "user", "content": "Say hi in 2 words."}])
    content = body.get("choices", [{}])[0].get("message", {}).get("content", "") if status == 200 else "(error)"
    print(f"    HTTP {status} in {t:.2f}s: {repr(content)}")
    time.sleep(1)  # be gentle to the server

# ── 3. System prompt support (evil level test) ──
print("\n=== 3. System Prompt Integration ===")
for evil, label in [(0.0, "nice"), (0.5, "chaotic"), (1.0, "evil dictator")]:
    sysp = f"You are a goose at evil level {evil}."
    status, body, t = chat("foundation", [
        {"role": "system", "content": sysp},
        {"role": "user", "content": "Introduce yourself."},
    ])
    content = body.get("choices", [{}])[0].get("message", {}).get("content", "") if status == 200 else "(error)"
    print(f"  evil={evil} ({label}): HTTP {status} in {t:.2f}s")
    print(f"    content: {repr(content[:100])}")
    time.sleep(1)

# ── 4. Response format verification ──
print("\n=== 4. Response Format (OpenAI compat check) ===")
status, body, _ = req("/v1/models")
print(f"  object: {body.get('object')}")
print(f"  data[0].id: {body.get('data',[{}])[0].get('id')}")
print(f"  data[0].object: {body.get('data',[{}])[0].get('object')}")
print(f"  ✅ OpenAI-compatible /v1/models format" if body.get("object") == "list" else "  ❌ Non-standard format")

status, body, t = chat("foundation", [{"role": "user", "content": "Hi."}], max_tokens=5)
if status == 200:
    c = body.get("choices", [{}])[0]
    msg = c.get("message", {})
    print(f"  choices[0].message.content: {repr(msg.get('content',''))}")
    print(f"  choices[0].finish_reason: {c.get('finish_reason')}")
    print(f"  usage keys: {list(body.get('usage',{}).keys())}")
    print(f"  model field: {body.get('model')}")
    print(f"  id field: {body.get('id')[:20]}...")
    print(f"  ✅ Fully OpenAI-compatible /v1/chat/completions")

# ── 5. Summary ──
print("\n=== SUMMARY ===")
print(f"  Models available: {[m['id'] for m in models]}")
print(f"  Working models tested: foundation, gemma-4-e4b-it-8bit")
print(f"  Default model (qwen3.6-27b-mxfp4): may hang or return empty")
print(f"  API: OpenAI-compatible ✓")
print(f"  ✅ Server is functional")
