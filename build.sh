#!/usr/bin/env bash
set -euo pipefail

# build.sh — macOS Release build for CadGoose.
#
# Checks for required Homebrew dependencies (installing any that are missing),
# then configures and builds with Ninja. Pass a build directory as $1
# (default: build). Set SKIP_DEPS=1 to bypass the dependency check.

BUILD_DIR="${1:-build}"

# ── Dependency check (macOS / Homebrew) ────────────────────
# Required Homebrew formulae. CMake also links system frameworks (Cocoa,
# CoreML, curl, …) which ship with macOS and need no install.
BREW_DEPS=(cmake ninja googletest mimalloc)

check_dependencies() {
    if [[ "$(uname)" != "Darwin" ]]; then
        echo "==> Non-macOS host; skipping Homebrew dependency check."
        echo "    See build_linux.sh / docs/README_LINUX.md for Linux builds."
        return
    fi

    if ! command -v brew >/dev/null 2>&1; then
        cat <<'EOF'
ERROR: Homebrew is not installed, but it is required to install build dependencies.

Install Homebrew first:

  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

Then re-run ./build.sh. (Or set SKIP_DEPS=1 to bypass this check if you have
cmake, ninja, googletest and mimalloc installed by other means.)
EOF
        exit 1
    fi

    local missing=()
    for dep in "${BREW_DEPS[@]}"; do
        if ! brew list --formula "$dep" >/dev/null 2>&1; then
            missing+=("$dep")
        fi
    done

    if (( ${#missing[@]} > 0 )); then
        echo "==> Installing missing dependencies: ${missing[*]}"
        brew install "${missing[@]}"
    else
        echo "==> All dependencies present: ${BREW_DEPS[*]}"
    fi
}

if [[ "${SKIP_DEPS:-0}" != "1" ]]; then
    check_dependencies
fi

# ── Configure & build ──────────────────────────────────────
echo "==> Configuring ($BUILD_DIR, Release, Ninja)"
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -GNinja

echo "==> Building"
cmake --build "$BUILD_DIR"

echo "==> Built $BUILD_DIR/CadGoose"
