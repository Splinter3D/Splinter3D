#!/usr/bin/env bash
# Create macOS release package (.dmg) for distribution

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../../" && pwd)"

BUILD_DIR="${PROJECT_ROOT}/build"
STAGING_DIR="${PROJECT_ROOT}/staging"
OUTPUT_DIR="${PROJECT_ROOT}/output"
TRIPLET="${VCPKG_TARGET_TRIPLET:-}"
APP_NAME="splinter3D"

# ============================================================================
# Helpers
# ============================================================================

die() {
    echo "ERROR: $*" >&2
    exit 1
}

msg() {
    echo "[package-macos] $*"
}

if [[ "$(uname -s)" != "Darwin" ]]; then
    die "This packaging script must run on macOS (Darwin)"
fi

# ============================================================================
# Auto-detect Version from .cz.toml
# ============================================================================

CZ_FILE="${PROJECT_ROOT}/.cz.toml"
if [[ ! -f "$CZ_FILE" ]]; then
    die "Cannot auto-detect version: .cz.toml not found at $CZ_FILE"
fi

VERSION=$(grep '^\s*version\s*=' "$CZ_FILE" | sed -E 's/.*"v?([0-9.]+)".*/\1/' | head -1)

if [[ -z "$VERSION" ]]; then
    die "Cannot parse version from .cz.toml"
fi

# ============================================================================
# Determine triplet/arch for naming
# ============================================================================

if [[ -z "$TRIPLET" ]]; then
    case "$(uname -m)" in
        arm64|aarch64) TRIPLET="arm64-osx" ;;
        *) TRIPLET="x64-osx" ;;
    esac
fi

ARCH="${TRIPLET%%-*}"
PACKAGE_DIRNAME="splinter3D-${VERSION}-macos-${ARCH}"
PACKAGE_NAME="${PACKAGE_DIRNAME}.dmg"
PACKAGE_PATH="${OUTPUT_DIR}/${PACKAGE_NAME}"
SHA256_FILE="${PACKAGE_PATH}.sha256"

msg "Auto-detected version: $VERSION"
msg "Package: $PACKAGE_NAME"
msg "Triplet: $TRIPLET"
msg "Arch: $ARCH"

# ============================================================================
# Verify Build Exists
# ============================================================================

[[ -d "$BUILD_DIR" ]] || die "Build directory not found: $BUILD_DIR"

SPLINTER_EXE=""
for search_dir in "$BUILD_DIR" "$PROJECT_ROOT"; do
    SPLINTER_EXE=$(find "$search_dir" -maxdepth 1 -type f \( -name "splinter3D" -o -name "splinter3D-app" \) 2>/dev/null | head -1)
    [[ -n "$SPLINTER_EXE" ]] && break
done

# Fallback: check for app bundle
if [[ -z "$SPLINTER_EXE" ]]; then
    SPLINTER_APP=$(find "$BUILD_DIR" "$PROJECT_ROOT" -maxdepth 2 -type d -name "splinter3D.app" 2>/dev/null | head -1)
    if [[ -n "$SPLINTER_APP" && -x "$SPLINTER_APP/Contents/MacOS/splinter3D" ]]; then
        SPLINTER_EXE="$SPLINTER_APP/Contents/MacOS/splinter3D"
    fi
fi

[[ -n "$SPLINTER_EXE" ]] || die "Executable not found in $BUILD_DIR or $PROJECT_ROOT"
msg "Found executable: $SPLINTER_EXE"

# ============================================================================
# Prepare Staging
# ============================================================================

msg "Preparing staging directory..."
[[ -d "$STAGING_DIR" ]] && rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR/Contents/MacOS" "$STAGING_DIR/Contents/Resources"

# Bundle binary
cp "$SPLINTER_EXE" "$STAGING_DIR/Contents/MacOS/${APP_NAME}"
chmod +x "$STAGING_DIR/Contents/MacOS/${APP_NAME}"

# Bundle resources
if [[ -d "${PROJECT_ROOT}/assets" ]]; then
    msg "Copying assets..."
    cp -r "${PROJECT_ROOT}/assets" "$STAGING_DIR/Contents/Resources/assets"
fi

# App icon (optional)
ICON_SRC="${PROJECT_ROOT}/assets/Splinter3D.icns"
if [[ -f "$ICON_SRC" ]]; then
    msg "Adding app icon Splinter3D.icns"
    cp "$ICON_SRC" "$STAGING_DIR/Contents/Resources/Splinter3D.icns"
fi

if [[ -d "${PROJECT_ROOT}/locale" ]]; then
    msg "Copying locale files..."
    cp -r "${PROJECT_ROOT}/locale" "$STAGING_DIR/Contents/Resources/locale"
    find "$STAGING_DIR/Contents/Resources/locale" -name "*.po" -delete 2>/dev/null || true
fi

# Metadata and docs placed at the root of the DMG next to the app
DOCS_DIR="$STAGING_DIR/../${PACKAGE_DIRNAME}-docs"
rm -rf "$DOCS_DIR"
mkdir -p "$DOCS_DIR"
for file in README.md LICENSE.md LICENSE; do
    [[ -f "${PROJECT_ROOT}/$file" ]] && cp "${PROJECT_ROOT}/$file" "$DOCS_DIR/$file"
done

# ==========================================================================
# Create Info.plist
# ==========================================================================

INFO_PLIST="$STAGING_DIR/Contents/Info.plist"
cat > "$INFO_PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>splinter3D</string>
    <key>CFBundleDisplayName</key><string>splinter3D</string>
    <key>CFBundleIdentifier</key><string>com.splinter3d.splinter3d</string>
    <key>CFBundleVersion</key><string>${VERSION}</string>
    <key>CFBundleShortVersionString</key><string>${VERSION}</string>
    <key>CFBundleExecutable</key><string>splinter3D</string>
    <key>CFBundleIconFile</key><string>Splinter3D.icns</string>
    <key>LSMinimumSystemVersion</key><string>11.0</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
EOF

# ==========================================================================
# Create DMG
# ==========================================================================

mkdir -p "$OUTPUT_DIR"
rm -f "$PACKAGE_PATH" "$SHA256_FILE"

PACKAGE_DIR_PATH="${PROJECT_ROOT}/${PACKAGE_DIRNAME}"
[[ -d "$PACKAGE_DIR_PATH" ]] && rm -rf "$PACKAGE_DIR_PATH"
mkdir -p "$PACKAGE_DIR_PATH"

# Place the app bundle at the root of the image
APP_BUNDLE="$PACKAGE_DIR_PATH/${APP_NAME}.app"
mv "$STAGING_DIR" "$APP_BUNDLE"

# Codesign if identity is provided (CI or local)
if [[ -n "${CODESIGN_IDENTITY:-}" ]]; then
    msg "Codesigning app with identity: ${CODESIGN_IDENTITY}"
    codesign --force --deep --options runtime --timestamp --sign "$CODESIGN_IDENTITY" "$APP_BUNDLE"
    codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"
fi

# Place docs alongside the app in the image
[[ -d "$DOCS_DIR" ]] && cp -R "$DOCS_DIR"/* "$PACKAGE_DIR_PATH/" || true

msg "Creating DMG archive..."
cd "$PROJECT_ROOT"
hdiutil create -volname "splinter3D" -srcfolder "$(basename "$PACKAGE_DIR_PATH")" -ov -format UDZO "$PACKAGE_PATH"

[[ -f "$PACKAGE_PATH" ]] || die "Failed to create package: $PACKAGE_PATH"
msg "Package created: $PACKAGE_PATH"

msg "Computing SHA256..."
shasum -a 256 "$PACKAGE_PATH" > "$SHA256_FILE"
msg "SHA256: $(cat "$SHA256_FILE")"

msg "Cleaning up temporary package directory..."
rm -rf "$PACKAGE_DIR_PATH" "$DOCS_DIR"

msg "✓ Packaging complete"
