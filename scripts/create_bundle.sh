#!/bin/bash
# create_bundle.sh — Build CadGoose as a proper macOS .app bundle
#
# Usage:
#   ./scripts/create_bundle.sh [--clean] [--build]
#
# Options:
#   --clean   Remove existing bundle before rebuilding
#   --build   Run cmake + make before bundling (for local dev)
#
# In CI, build the binary separately and just run:
#   ./scripts/create_bundle.sh
#
# The bundle structure follows Apple's conventions:
#   CadGoose.app/
#     Contents/
#       Info.plist
#       MacOS/
#         CadGoose          <- compiled binary
#       Resources/
#         Assets/           <- symlink to ../../Assets
#         app.icns          <- icon (from project root or auto-generated)
#
# Environment:
#   BUNDLE_ID   Override bundle identifier (default: com.desktoppad.CadGoose)
#   BUNDLE_NAME Override app display name (default: CadGoose)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"
BUNDLE_DIR="${BUILD_DIR}/CadGoose.app"
CONTENTS_DIR="${BUNDLE_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"

BUNDLE_ID="${BUNDLE_ID:-com.desktoppad.CadGoose}"
BUNDLE_NAME="${BUNDLE_NAME:-CadGoose}"

# Parse options
CLEAN=false
DO_BUILD=false
for arg in "$@"; do
    case "$arg" in
        --clean) CLEAN=true ;;
        --build) DO_BUILD=true ;;
        --help|-h)
            echo "Usage: $0 [--clean] [--build] [--help]"
            echo ""
            echo "Build CadGoose as a macOS .app bundle."
            echo ""
            echo "Options:"
            echo "  --clean   Remove existing bundle before rebuilding"
            echo "  --build   Run cmake + make before bundling"
            echo ""
            echo "Environment variables:"
            echo "  BUNDLE_ID   Bundle identifier (default: com.desktoppad.CadGoose)"
            echo "  BUNDLE_NAME App display name (default: CadGoose)"
            exit 0
            ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
done

echo "=== CadGoose macOS App Bundle Builder ==="
echo ""

# Clean if requested
if [ "$CLEAN" = true ] && [ -d "$BUNDLE_DIR" ]; then
    echo "Removing existing bundle..."
    rm -rf "$BUNDLE_DIR"
fi

# Build the binary if requested (for local dev)
if [ "$DO_BUILD" = true ]; then
    echo "Building binary..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release --parallel $(sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
    echo ""
fi

# Verify binary exists
if [ ! -f "${BUILD_DIR}/CadGoose" ]; then
    echo "ERROR: Binary not found at ${BUILD_DIR}/CadGoose"
    echo "Run with --build to build first, or run cmake/make manually."
    exit 1
fi

# Create bundle directory structure
echo "Creating bundle structure..."
mkdir -p "$MACOS_DIR"
mkdir -p "$RESOURCES_DIR"

# Copy binary
echo "Copying binary..."
cp "${BUILD_DIR}/CadGoose" "${MACOS_DIR}/CadGoose"
chmod +x "${MACOS_DIR}/CadGoose"

# Preserve debug symbols (optional)
if [ -f "${BUILD_DIR}/CadGoose.dSYM" ]; then
    echo "Copying debug symbols..."
    cp -R "${BUILD_DIR}/CadGoose.dSYM" "${CONTENTS_DIR}/CadGoose.dSYM"
fi

# Symlink Assets so the app can find them at runtime
echo "Linking assets..."
if [ -d "${PROJECT_DIR}/Assets" ]; then
    ln -sf "${PROJECT_DIR}/Assets" "${RESOURCES_DIR}/Assets"
else
    echo "WARNING: Assets directory not found at ${PROJECT_DIR}/Assets"
fi

# Create Info.plist with environment variables
echo "Generating Info.plist..."
cat > "${CONTENTS_DIR}/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>CadGoose</string>
    <key>CFBundleIconFile</key>
    <string>app.icns</string>
    <key>CFBundleIdentifier</key>
    <string>${BUNDLE_ID}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>${BUNDLE_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSRequiresAquaSystemAppearance</key>
    <false/>
    <key>NSAppleEventsUsageDescription</key>
    <string>CadGoose needs access to accessibility features for cursor control.</string>
    <key>NSAccessibility</key>
    <string>YES</string>
</dict>
</plist>
PLIST

# Create minimal PBOARD
cat > "${CONTENTS_DIR}/PkgInfo" <<PKGTYPE
APPL????
PKGTYPE

# Set bundle icon — prefer project root app.icns, fall back to auto-generated
ICON_COPIED=false
if [ -f "${PROJECT_DIR}/app.icns" ]; then
    cp "${PROJECT_DIR}/app.icns" "${RESOURCES_DIR}/app.icns"
    ICON_COPIED=true
    echo "Icon copied from project root."
else
    # Attempt to generate from first available project image
    for candidate in "${PROJECT_DIR}/Assets/Images/OtherGfx/"*.png \
                      "${PROJECT_DIR}/Assets/Images/"*.png; do
        if [ -f "$candidate" ]; then
            echo "Generating icon from $candidate..."
            ICON_SET="${BUILD_DIR}/icon.iconset"
            mkdir -p "$ICON_SET"
            sips -z 16 16   "$candidate" --out "${ICON_SET}/icon_16x16.png"  2>/dev/null || true
            sips -z 32 32   "$candidate" --out "${ICON_SET}/icon_16x16@2x.png" 2>/dev/null || true
            sips -z 32 32   "$candidate" --out "${ICON_SET}/icon_32x32.png" 2>/dev/null || true
            sips -z 64 64   "$candidate" --out "${ICON_SET}/icon_32x32@2x.png" 2>/dev/null || true
            sips -z 128 128 "$candidate" --out "${ICON_SET}/icon_128x128.png" 2>/dev/null || true
            sips -z 256 256 "$candidate" --out "${ICON_SET}/icon_128x128@2x.png" 2>/dev/null || true
            sips -z 256 256 "$candidate" --out "${ICON_SET}/icon_256x256.png" 2>/dev/null || true
            sips -z 512 512 "$candidate" --out "${ICON_SET}/icon_256x256@2x.png" 2>/dev/null || true
            sips -z 512 512 "$candidate" --out "${ICON_SET}/icon_512x512.png" 2>/dev/null || true
            cp "$candidate"  "${ICON_SET}/icon_512x512@2x.png" 2>/dev/null || true
            iconutil -c icns "$ICON_SET" -o "${RESOURCES_DIR}/app.icns" 2>/dev/null && \
                ICON_COPIED=true || \
                echo "WARNING: Could not generate .icns"
            rm -rf "$ICON_SET"
            break
        fi
    done
fi

if [ "$ICON_COPIED" = false ]; then
    echo "WARNING: No icon found - bundle will use default macOS icon"
fi

# Fix rpath so dynamic libraries (libcurl, etc.) are resolved from within the bundle
echo "Fixing library paths..."
DEPS=$(otool -L "${MACOS_DIR}/CadGoose" 2>/dev/null \
    | grep -E "^\s+" \
    | awk '{print $1}' \
    | grep -v "@rpath" \
    | grep -v "@executable" \
    | grep -v "/usr/lib" \
    | grep -v "/System" || true)

if [ -n "$DEPS" ]; then
    mkdir -p "${CONTENTS_DIR}/Frameworks"
    for dep in $DEPS; do
        if [ -f "$dep" ]; then
            basename_dep=$(basename "$dep")
            cp "$dep" "${CONTENTS_DIR}/Frameworks/" 2>/dev/null || true
            install_name_tool -change "$dep" "@rpath/${basename_dep}" \
                "${MACOS_DIR}/CadGoose" 2>/dev/null || true
        fi
    done
fi

# Add rpaths for executable path resolution
install_name_tool -add_rpath "@executable_path/../Frameworks" \
    "${MACOS_DIR}/CadGoose" 2>/dev/null || true
install_name_tool -add_rpath "@executable_path" \
    "${MACOS_DIR}/CadGoose" 2>/dev/null || true

# Note: Skipping strip - removing symbol tables can break Metal shader JIT compilation
# on certain macOS versions with ad-hoc code signing
if false && command -v strip &>/dev/null; then
    echo "Stripping symbols..."
    strip -S "${MACOS_DIR}/CadGoose" 2>/dev/null || true
fi

# Ad-hoc code sign the bundle
# Note: Using minimal entitlements to avoid Metal JIT issues with ad-hoc signing
if command -v codesign &>/dev/null; then
    echo "Code signing bundle..."
    codesign --sign - \
        --timestamp=none \
        --force \
        --deep \
        "${BUNDLE_DIR}" 2>/dev/null && \
        echo "Bundle signed." || \
        echo "WARNING: Code signing failed"
fi

# Summary
echo ""
echo "=== Build Complete ==="
echo "Bundle:  ${BUNDLE_DIR}"
echo "Binary:  ${MACOS_DIR}/CadGoose"
echo "Size:    $(du -sh "${BUNDLE_DIR}" | cut -f1)"
echo "Plist:   ${CONTENTS_DIR}/Info.plist"
echo ""

# Offer to open (only in interactive terminal)
if [ -t 0 ] && command -v open &>/dev/null; then
    read -p "Open the app? (y/N) " -n 1 -r REPLY
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        open "$BUNDLE_DIR"
    fi
fi