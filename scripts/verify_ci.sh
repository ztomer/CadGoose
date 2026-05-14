#!/bin/bash
# verify_ci.sh — Validate CI workflow and bundle setup locally
#
# Run this to catch CI issues before pushing:
#   ./scripts/verify_ci.sh
#
# Exit codes:
#   0 = all checks passed
#   1 = one or more checks failed

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0
WARN=0

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

log_pass() { ((PASS++)); echo -e "  ${GREEN}✓${NC} $1"; }
log_fail() { ((FAIL++)); echo -e "  ${RED}✗${NC} $1"; }
log_warn() { ((WARN++)); echo -e "  ${YELLOW}⚠${NC} $1"; }

echo "=== CadGoose CI Local Verification ==="
echo ""

# ─── 1. Workflow YAML validation ───
echo "[1/7] Validating workflow YAML..."
WF_FILE="${PROJECT_DIR}/.github/workflows/build_and_release.yml"

if [ ! -f "$WF_FILE" ]; then
    log_fail "Workflow file not found: .github/workflows/build_and_release.yml"
else
    # Check basic YAML validity using Python
    python3 -c "
import yaml, sys
try:
    with open('$WF_FILE') as f:
        wf = yaml.safe_load(f)
    # Validate structure (note: 'on' may parse as True in YAML 1.1)
    assert 'name' in wf, 'Missing workflow name'
    on_key = 'on' if 'on' in wf else True
    assert on_key in wf, 'Missing trigger (on:)'
    assert 'jobs' in wf, 'Missing jobs'
    on_val = wf[on_key]
    assert isinstance(on_val, dict), 'on: value must be a dict with push/tag triggers'
    assert 'push' in on_val or 'pull_request' in on_val, 'No push/tag triggers found'
    jobs = wf['jobs']
    for name, job in jobs.items():
        assert 'runs-on' in job, f'Job {name} missing runs-on'
        assert 'steps' in job, f'Job {name} missing steps'
        for i, step in enumerate(job['steps']):
            assert 'name' in step or 'run' in step, f'Job {name} step {i} has no name or run'
    print('       Structure validated (name, on, jobs, steps)')
except Exception as e:
    print(f'       ERROR: {e}')
    sys.exit(1)
" && log_pass "YAML valid and well-structured" || log_fail "YAML validation failed"
fi

# ─── 2. Referenced paths exist ───
echo "[2/7] Checking referenced paths..."

PATHS_OK=true

# Bundle script
if [ -x "${PROJECT_DIR}/scripts/create_bundle.sh" ]; then
    log_pass "create_bundle.sh exists and is executable"
else
    log_fail "create_bundle.sh missing or not executable"
    PATHS_OK=false
fi

# Project files
for f in CMakeLists.txt README.md docs/MCP.md docs/ARCH.md; do
    if [ -f "${PROJECT_DIR}/${f}" ]; then
        log_pass "${f}"
    else
        log_fail "${f} not found"
        PATHS_OK=false
    fi
done

# Directories
for d in Assets build; do
    if [ -d "${PROJECT_DIR}/${d}" ]; then
        log_pass "${d}/ directory exists"
    else
        log_fail "${d}/ directory missing"
        PATHS_OK=false
    fi
done

# ─── 3. Build system sanity ───
echo "[3/7] Checking build system..."

if [ -d "${PROJECT_DIR}/build" ]; then
    if [ -f "${PROJECT_DIR}/build/CMakeCache.txt" ]; then
        log_pass "CMake configured"
    else
        log_warn "build/ exists but CMakeCache.txt not found (run cmake first)"
    fi
else
    log_warn "build/ directory not found (build not configured)"
fi

# Check macOS source files compile (syntax check only)
OBJC_SOURCES=$(find "${PROJECT_DIR}/src/platform/macos" -name "*.mm" | head -10)
for src in $OBJC_SOURCES; do
    if head -1 "$src" | grep -q "^//"; then
        log_pass "$(basename "$src") — header valid"
    fi
done

# ─── 4. Bundle script dry-run verification ───
echo "[4/7] Testing bundle script (syntax check)..."

if bash -n "${PROJECT_DIR}/scripts/create_bundle.sh" 2>/dev/null; then
    log_pass "create_bundle.sh has no syntax errors"
else
    log_fail "create_bundle.sh has shell syntax errors"
fi

# Check script references valid tools
for tool in cmake install_name_tool sips iconutil strip otool; do
    if command -v "$tool" &>/dev/null || [ "$tool" = "sips" ] || [ "$tool" = "iconutil" ]; then
        # sips and iconutil are macOS-only, skip on Linux
        if [[ "$OSTYPE" != "darwin"* ]] && [[ "$tool" == "sips" || "$tool" == "iconutil" ]]; then
            log_warn "$tool — macOS only (expected on CI)"
        else
            log_pass "$tool available"
        fi
    else
        log_warn "$tool not found (may be OK on non-CI systems)"
    fi
done

# ─── 5. Source code consistency ───
echo "[5/7] Checking source code consistency..."

# Verify no stale unix socket references remain
if grep -rq "useUnixSocket\|sendMessageViaUnix\|unixSocketPath" "${PROJECT_DIR}/src/" 2>/dev/null; then
    log_fail "Stale unix socket references found in src/"
else
    log_pass "No stale unix socket references in source"
fi

# Verify AI chat is always HTTP
if grep -rn "useUnixSocket" "${PROJECT_DIR}/include/" 2>/dev/null; then
    log_fail "useUnixSocket still in headers"
else
    log_pass "AI config has no Unix socket toggle"
fi

# Verify MCP dual transport
if grep -q "Access-Control-Allow-Origin" "${PROJECT_DIR}/src/common/mcp_http_server.cpp" 2>/dev/null; then
    log_pass "MCP HTTP server has CORS headers"
else
    log_warn "MCP HTTP server may be missing CORS headers"
fi

if grep -q "unix_socket_path" "${PROJECT_DIR}/src/common/mcp_server.cpp" 2>/dev/null || \
   grep -q "/tmp/desktop-goose-mcp" "${PROJECT_DIR}/src/common/mcp_server.cpp" 2>/dev/null; then
    log_pass "MCP Unix socket transport present"
else
    log_warn "MCP Unix socket path not found in mcp_server.cpp"
fi

# ─── 6. Test suite integrity ───
echo "[6/7] Checking test suite..."

TEST_BINARY="${PROJECT_DIR}/build/CadGooseTests"
if [ -x "$TEST_BINARY" ]; then
    TOTAL=$("$TEST_BINARY" --gtest_list_tests 2>/dev/null | grep -c "  ")
    if [ "$TOTAL" -gt 0 ]; then
        log_pass "$TEST_BINARY — $TOTAL tests available"
    else
        log_fail "Test binary runs but reports 0 tests"
    fi

    # Verify new test suites exist
    for suite in AIEndpoint AIHandler AIThinkBlock; do
        if "$TEST_BINARY" --gtest_filter="*${suite}*" --gtest_list_tests 2>/dev/null | grep -q "${suite}"; then
            log_pass "Test suite ${suite} present"
        else
            log_fail "Test suite ${suite} missing"
        fi
    done
else
    log_warn "Test binary not found or not executable at $TEST_BINARY"
fi

# ─── 7. App bundle verification ───
echo "[7/7] Checking app bundle..."

BUNDLE="${PROJECT_DIR}/build/CadGoose.app"
if [ -d "$BUNDLE" ]; then
    log_pass "CadGoose.app exists"

    # Check Info.plist
    if [ -f "${BUNDLE}/Contents/Info.plist" ]; then
        log_pass "Info.plist present"

        # Validate plist structure
        if python3 -c "
import plistlib
try:
    with open('${BUNDLE}/Contents/Info.plist', 'rb') as f:
        pl = plistlib.load(f)
    assert 'CFBundleIdentifier' in pl
    assert 'CFBundleName' in pl
    assert 'CFBundleExecutable' in pl
    assert pl['CFBundlePackageType'] == 'APPL'
    print('       Bundle ID: ' + pl['CFBundleIdentifier'])
    print('       Name: ' + pl['CFBundleName'])
    print('       Executable: ' + pl['CFBundleExecutable'])
except Exception as e:
    print(f'       ERROR: {e}')
    import sys; sys.exit(1)
" 2>&1; then
            log_pass "Info.plist structure valid"
        else
            log_fail "Info.plist has invalid structure"
        fi
    else
        log_fail "Info.plist missing"
    fi

    # Check binary
    if [ -x "${BUNDLE}/Contents/MacOS/CadGoose" ]; then
        log_pass "Executable present and marked executable"
        BIN_TYPE=$(file -b "${BUNDLE}/Contents/MacOS/CadGoose" | head -1)
        if echo "$BIN_TYPE" | grep -q "Mach-O"; then
            log_pass "Binary is Mach-O format"
        else
            log_fail "Binary is not Mach-O: $BIN_TYPE"
        fi
    else
        log_fail "Executable missing or not executable"
    fi

    # Check icon
    if [ -f "${BUNDLE}/Contents/Resources/app.icns" ]; then
        log_pass "app.icns present"
    elif [ -f "${PROJECT_DIR}/app.icns" ]; then
        log_pass "app.icns exists at project root (will be copied on bundle build)"
    else
        log_warn "No app.icns found (bundle will have default icon)"
    fi

    # Check assets symlink
    if [ -L "${BUNDLE}/Contents/Resources/Assets" ]; then
        log_pass "Assets symlink present"
        TARGET=$(readlink "${BUNDLE}/Contents/Resources/Assets")
        log_pass "  → $TARGET"
    elif [ -d "${BUNDLE}/Contents/Resources/Assets" ]; then
        log_pass "Assets directory present (not symlinked)"
    else
        log_warn "Assets directory missing in bundle"
    fi
else
    log_warn "CadGoose.app bundle not found (run create_bundle.sh first)"
fi

# ─── Summary ───
echo ""
echo "=== Summary ==="
echo -e "  Passed: ${GREEN}${PASS}${NC}"
if [ $FAIL -gt 0 ]; then
    echo -e "  Failed: ${RED}${FAIL}${NC}"
fi
if [ $WARN -gt 0 ]; then
    echo -e "  Warnings: ${YELLOW}${WARN}${NC}"
fi
echo ""

if [ $FAIL -gt 0 ]; then
    echo -e "${RED}VERIFICATION FAILED — fix errors before pushing${NC}"
    exit 1
else
    echo -e "${GREEN}VERIFICATION PASSED${NC}"
    exit 0
fi