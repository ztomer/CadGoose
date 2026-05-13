#!/usr/bin/env python3
"""
Osaurus API Research Script
Probes a running Osaurus server to understand its API behavior.
Used to inform the C++ AIHTTPClient refactoring in CadGoose.
"""

import json
import sys
import time
import urllib.request
import urllib.error

OSAURUS_BASE = "http://localhost:1337"
RESULTS = []


def log(label, value, ok=True):
    icon = "✅" if ok else "❌"
    RESULTS.append((label, value, ok))
    print(f"  {icon} {label}: {value}")


def req(path, method="GET", body=None, timeout=10):
    url = f"{OSAURUS_BASE}{path}"
    data = json.dumps(body).encode() if body else None
    req_obj = urllib.request.Request(url, data=data, method=method)
    req_obj.add_header("Content-Type", "application/json")
    try:
        t0 = time.time()
        with urllib.request.urlopen(req_obj, timeout=timeout) as resp:
            elapsed = time.time() - t0
            raw = resp.read()
            status = resp.status
            ct = resp.headers.get("Content-Type", "")
            try:
                parsed = json.loads(raw)
            except json.JSONDecodeError:
                parsed = {"raw_text": raw.decode("utf-8", errors="replace")}
            return {"status": status, "body": parsed, "elapsed": elapsed, "raw": raw, "content_type": ct}
    except urllib.error.HTTPError as e:
        return {"status": e.code, "body": {"error": str(e.reason)}, "elapsed": 0}
    except urllib.error.URLError as e:
        return {"status": 0, "body": {"error": str(e.reason)}, "elapsed": 0}
    except Exception as e:
        return {"status": 0, "body": {"error": str(e)}, "elapsed": 0}


def step(name, path, method="GET", body=None):
    print(f"\n--- {name} ---")
    r = req(path, method, body)
    ok = 200 <= r["status"] < 300
    log("HTTP status", r["status"], ok)
    log("Response time", f"{r['elapsed']:.2f}s", r["elapsed"] < 5)
    return r


# ── 1. Server health ──────────────────────────────────────────────
def test_server_health():
    print("\n" + "=" * 60)
    print("SECTION 1: Server Health")
    print("=" * 60)

    r = step("Root endpoint", "/")
    body = r.get("body", {})
    log("Response body", str(body))
    log("Server alive", "yes" if r["status"] == 200 else "no", r["status"] == 200)

    r = step("Model list endpoint", "/v1/models")
    body = r.get("body", {})
    models = body.get("data", [])
    log("Model count", len(models))
    log("Response format", f"object={body.get('object')}")
    for m in models:
        mid = m.get("id", "?")
        owner = m.get("owned_by", "?")
        log(f"  Model", f"{mid} (owned_by={owner})", "foundation" in mid or bool(mid))

    return [m.get("id") for m in models]


# ── 2. Chat completions per model ─────────────────────────────────
def test_chat_completions(models):
    print("\n" + "=" * 60)
    print("SECTION 2: Chat Completions")
    print("=" * 60)

    test_messages = [
        {"role": "user", "content": "Say hello in 3 words."},
    ]

    messages_with_system = [
        {"role": "system", "content": "You are a helpful goose."},
        {"role": "user", "content": "Say hello in 3 words."},
    ]

    for model in models:
        print(f"\n  Model: {model}")

        # User-only prompt
        r = step(f"  user-only", "/v1/chat/completions", "POST",
                 {"model": model, "messages": test_messages, "max_tokens": 50, "temperature": 0.5})
        body = r.get("body", {})
        choices = body.get("choices", [])
        if choices:
            content = choices[0].get("message", {}).get("content", "")
            finish = choices[0].get("finish_reason", "?")
            log(f"    content", repr(content) if content else "(empty)", bool(content))
            log(f"    finish_reason", finish)
            usage = body.get("usage", {})
            log(f"    usage", json.dumps(usage))
            log(f"    model response", body.get("model", "?"))
        else:
            log(f"    choices", "none", False)
            log(f"    raw", json.dumps(body)[:200])

        # System + user prompt
        r = step(f"  with-system", "/v1/chat/completions", "POST",
                 {"model": model, "messages": messages_with_system, "max_tokens": 50, "temperature": 0.5})
        body = r.get("body", {})
        choices = body.get("choices", [])
        if choices:
            content = choices[0].get("message", {}).get("content", "")
            finish = choices[0].get("finish_reason", "?")
            log(f"    content", repr(content) if content else "(empty)", bool(content))
            log(f"    finish_reason", finish)
        else:
            log(f"    choices", "none", False)

        # Test bad model
        r = step(f"  bad-model", "/v1/chat/completions", "POST",
                 {"model": "nonexistent-model", "messages": test_messages, "max_tokens": 50})
        body = r.get("body", {})
        log(f"    status on error", r["status"], r["status"] >= 400)
        log(f"    error body", str(body.get("error", body))[:100])


# ── 3. Evil level prompt integration test ─────────────────────────
def test_evil_prompts():
    print("\n" + "=" * 60)
    print("SECTION 3: Evil Level Prompt Integration")
    print("=" * 60)

    evil_prompts = [
        (0.0, "You are an adorable fluffy gosling. You love everyone and want to be best friends."),
        (0.25, "You are a mischievous prankster goose."),
        (0.5, "You are a chaotic neutral goose."),
        (0.75, "You are a villainous goose scheming against humanity."),
        (1.0, "You are an absurdly eloquent goose dictator."),
    ]

    for level, prompt_prefix in evil_prompts:
        print(f"\n  Evil level {level:.2f}")
        messages = [
            {"role": "system", "content": prompt_prefix},
            {"role": "user", "content": "Introduce yourself in character."},
        ]
        r = step(f"  evil={level}", "/v1/chat/completions", "POST",
                 {"model": "foundation", "messages": messages, "max_tokens": 80, "temperature": 0.8})
        body = r.get("body", {})
        choices = body.get("choices", [])
        if choices:
            content = choices[0].get("message", {}).get("content", "")
            log(f"    response", repr(content)[:80] if content else "(empty)", bool(content))
            log(f"    tokens_used", body.get("usage", {}).get("total_tokens", "?"))


# ── 4. Parameter exploration ──────────────────────────────────────
def test_parameters():
    print("\n" + "=" * 60)
    print("SECTION 4: Parameter Exploration")
    print("=" * 60)

    messages = [{"role": "user", "content": "List 3 colors."}]

    # Temperature variations
    for temp in [0.0, 0.5, 1.0, 1.5]:
        r = step(f"  temperature={temp}", "/v1/chat/completions", "POST",
                 {"model": "foundation", "messages": messages, "max_tokens": 30, "temperature": temp})
        body = r.get("body", {})
        choices = body.get("choices", [])
        content = choices[0].get("message", {}).get("content", "") if choices else ""
        log(f"    response", repr(content)[:60] if content else "(empty)", bool(content))

    # Max tokens variations
    for mt in [5, 50, 200]:
        r = step(f"  max_tokens={mt}", "/v1/chat/completions", "POST",
                 {"model": "foundation", "messages": [{"role": "user", "content": "Tell me a story about a goose."}],
                  "max_tokens": mt, "temperature": 0.5})
        body = r.get("body", {})
        choices = body.get("choices", [])
        content = choices[0].get("message", {}).get("content", "") if choices else ""
        actual_tokens = body.get("usage", {}).get("completion_tokens", 0)
        log(f"    response_len", f"{len(content)} chars, {actual_tokens} tokens")
        log(f"    truncated?", actual_tokens >= mt, actual_tokens < mt)


# ── 5. Error handling ─────────────────────────────────────────────
def test_errors():
    print("\n" + "=" * 60)
    print("SECTION 5: Error Handling")
    print("=" * 60)

    cases = [
        ("empty body", {}),
        ("missing model", {"messages": [{"role": "user", "content": "hi"}]}),
        ("missing messages", {"model": "foundation"}),
    ]

    for label, body in cases:
        print(f"\n  {label}")
        r = step(f"  {label}", "/v1/chat/completions", "POST", body)
        log(f"    body", str(r.get("body", {}))[:120])


# ── 6. Performance ────────────────────────────────────────────────
def test_performance():
    print("\n" + "=" * 60)
    print("SECTION 6: Performance")
    print("=" * 60)

    times = []
    for i in range(5):
        r = req("/v1/chat/completions", "POST", {
            "model": "foundation",
            "messages": [{"role": "user", "content": "Say hi."}],
            "max_tokens": 10,
            "temperature": 0.0
        }, timeout=30)
        times.append(r["elapsed"])
        log(f"  request {i+1}", f"{r['elapsed']:.3f}s", r["elapsed"] < 3)

    avg = sum(times) / len(times)
    log("avg latency", f"{avg:.3f}s")
    log("min latency", f"{min(times):.3f}s")
    log("max latency", f"{max(times):.3f}s")


# ── 7. Model list format ──────────────────────────────────────────
def test_model_list_format():
    print("\n" + "=" * 60)
    print("SECTION 7: Model List Format Compatibility")
    print("=" * 60)

    r = req("/v1/models")
    body = r.get("body", {})
    if not isinstance(body, dict):
        log("body type", type(body).__name__)
        return

    # OpenAI format: {"object":"list","data":[{"id":"...","object":"model",...}]}
    if "data" in body and isinstance(body["data"], list):
        log("has data[]", "yes", True)
        if body.get("object") == "list":
            log("object=list", "yes", True)
        for m in body["data"]:
            if "id" in m:
                log("  id field", "present", True)
                break
        else:
            log("  id field", "missing", False)
    else:
        log("data[] format", "NOT OpenAI-compatible!", False)

    # Check if non-OpenAI format also works
    if "models" in body and isinstance(body["models"], list):
        log("has models[] (Ollama format)", "yes")


# ── Report ────────────────────────────────────────────────────────
def print_summary():
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)

    total = len(RESULTS)
    passed = sum(1 for _, _, ok in RESULTS if ok)
    failed = total - passed
    print(f"\n  Total checks: {total}")
    print(f"  Passed:       {passed}")
    print(f"  Failed:       {failed}")

    if failed:
        print(f"\n  Failures:")
        for label, value, ok in RESULTS:
            if not ok:
                print(f"    ❌ {label}: {value}")

    print(f"\n  Recommendations:")
    print(f"    1. Default model should be 'foundation' (qwen3.6-27b-mxfp4 returns empty)")
    print(f"    2. Osaurus uses OpenAI-compatible /v1/chat/completions endpoint")
    print(f"    3. Need customEndpoint field for Custom provider")
    print(f"    4. Add ProviderType enum to replace useOsaurus/useOllama booleans")
    print(f"    5. Consider adding unix domain socket transport")


def main():
    print("Osaurus API Research Tool")
    print(f"Target: {OSAURUS_BASE}")
    print(f"Time:   {time.strftime('%Y-%m-%d %H:%M:%S')}")

    # Quick health check first
    try:
        r = req("/")
        if r["status"] != 200:
            print(f"\n❌ Osaurus not responding at {OSAURUS_BASE}")
            print("   Start it first, then re-run this script.")
            sys.exit(1)
        body_type = type(r.get("body")).__name__
        print(f"\n✅ Osaurus is running! (response type: {body_type})")
    except Exception as e:
        print(f"\n❌ Cannot connect: {e}")
        sys.exit(1)

    models = test_server_health()
    if models:
        test_chat_completions(models)
    test_evil_prompts()
    test_parameters()
    test_errors()
    test_performance()
    test_model_list_format()
    print_summary()


if __name__ == "__main__":
    main()
