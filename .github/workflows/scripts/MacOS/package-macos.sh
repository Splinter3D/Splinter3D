#!/usr/bin/env bash
# Create macOS release package (.zip) for distribution

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../../" && pwd)"

BUILD_DIR="${PROJECT_ROOT}/build"
STAGING_DIR="${PROJECT_ROOT}/staging"
OUTPUT_DIR="${PROJECT_ROOT}/output"
TRIPLET="${VCPKG_TARGET_TRIPLET:-}"

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
PACKAGE_NAME="${PACKAGE_DIRNAME}.zip"
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
mkdir -p "$STAGING_DIR/bin"

cp "$SPLINTER_EXE" "$STAGING_DIR/bin/splinter3D"
chmod +x "$STAGING_DIR/bin/splinter3D"

if [[ -d "${PROJECT_ROOT}/locale" ]]; then
    msg "Copying locale files..."
    cp -r "${PROJECT_ROOT}/locale" "$STAGING_DIR/locale"
    find "$STAGING_DIR/locale" -name "*.po" -delete 2>/dev/null || true
fi

if [[ -d "${PROJECT_ROOT}/assets" ]]; then
    msg "Copying assets..."
    cp -r "${PROJECT_ROOT}/assets" "$STAGING_DIR/assets"
fi

for file in README.md LICENSE.md LICENSE; do
    [[ -f "${PROJECT_ROOT}/$file" ]] && cp "${PROJECT_ROOT}/$file" "$STAGING_DIR/$file"
done

# ============================================================================
# Create Archive
# ============================================================================

mkdir -p "$OUTPUT_DIR"
rm -f "$PACKAGE_PATH" "$SHA256_FILE"

PACKAGE_DIR_PATH="${PROJECT_ROOT}/${PACKAGE_DIRNAME}"
[[ -d "$PACKAGE_DIR_PATH" ]] && rm -rf "$PACKAGE_DIR_PATH"
mv "$STAGING_DIR" "$PACKAGE_DIR_PATH"

msg "Creating ZIP archive..."
cd "$PROJECT_ROOT"
zip -r -9 "$PACKAGE_PATH" "$(basename "$PACKAGE_DIR_PATH")"

[[ -f "$PACKAGE_PATH" ]] || die "Failed to create package: $PACKAGE_PATH"
msg "Package created: $PACKAGE_PATH"

msg "Computing SHA256..."
shasum -a 256 "$PACKAGE_PATH" > "$SHA256_FILE"
msg "SHA256: $(cat "$SHA256_FILE")"

msg "Cleaning up temporary package directory..."
rm -rf "$PACKAGE_DIR_PATH"

msg "✓ Packaging complete"
