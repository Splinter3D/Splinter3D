#!/usr/bin/env bash
# Create Linux release package (.tar.xz) for distribution

set -euo pipefail

# Get project root (navigate from .github/workflows/scripts/Linux back to root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../../" && pwd)"

ARCH=$(uname -m)

case "$ARCH" in
    x86_64|amd64)
        ARCH="x64"
        ;;
    aarch64|arm64)
        ARCH="arm64"
        ;;
    *)
        die "Unsupported architecture: $ARCH"
        ;;
esac

BUILD_DIR="${PROJECT_ROOT}/build"
STAGING_DIR="${PROJECT_ROOT}/staging"
OUTPUT_DIR="${PROJECT_ROOT}/output"
TRIPLET="${ARCH}-linux"
VERSION=""

# ============================================================================
# Helpers
# ============================================================================

die() {
    echo "ERROR: $*" >&2
    exit 1
}

msg() {
    echo "[package-linux] $*"
}

# ============================================================================
# Auto-detect Version from scm
# ============================================================================

VERSION=$(git describe --tags --abbrev=0 2>/dev/null || true)

if [[ -z "$VERSION" ]]; then
    die "Cannot parse version from git tags"
fi

# Extract architecture from triplet (e.g., x64-linux -> x64)
PACKAGE_NAME="splinter3D-${VERSION}-linux-${ARCH}.tar.xz"
PACKAGE_PATH="${OUTPUT_DIR}/${PACKAGE_NAME}"
SHA256_FILE="${PACKAGE_PATH}.sha256"

msg "Auto-detected version: $VERSION"
msg "Package: $PACKAGE_NAME"

# ============================================================================
# Verify Build Exists
# ============================================================================

if [[ ! -d "$BUILD_DIR" ]]; then
    die "Build directory not found: $BUILD_DIR"
fi

# Try to find executable in build directory first, then project root
# (CMake may output executable to either location depending on configuration)
SPLINTER_EXE=""
for search_dir in "$BUILD_DIR" "$PROJECT_ROOT"; do
    SPLINTER_EXE=$(find "$search_dir" -maxdepth 1 -type f \( -name "splinter3D" -o -name "splinter3D-app" \) 2>/dev/null | head -1)
    [[ -n "$SPLINTER_EXE" ]] && break
done

if [[ -z "$SPLINTER_EXE" ]]; then
    die "Executable not found in $BUILD_DIR or $PROJECT_ROOT"
fi

msg "Found executable: $SPLINTER_EXE"

# ============================================================================
# Prepare Staging
# ============================================================================

msg "Preparing staging directory..."
[[ -d "$STAGING_DIR" ]] && rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR/bin"

# Copy executable
cp "$SPLINTER_EXE" "$STAGING_DIR/bin/splinter3D"
chmod +x "$STAGING_DIR/bin/splinter3D"

# Copy locale
if [[ -d "${PROJECT_ROOT}/locale" ]]; then
    msg "Copying locale files..."
    cp -r "${PROJECT_ROOT}/locale" "$STAGING_DIR/locale"
    find "$STAGING_DIR/locale" -name "*.po" -delete 2>/dev/null || true
fi

# Copy assets
if [[ -d "${PROJECT_ROOT}/assets" ]]; then
    msg "Copying assets..."
    cp -r "${PROJECT_ROOT}/assets" "$STAGING_DIR/assets"
fi

# Copy documentation
for file in README.md LICENSE.md LICENSE; do
    [[ -f "${PROJECT_ROOT}/$file" ]] && cp "${PROJECT_ROOT}/$file" "$STAGING_DIR/$file"
done

# ============================================================================
# Create Archive
# ============================================================================

mkdir -p "$OUTPUT_DIR"

msg "Creating TAR.XZ archive..."
cd "$STAGING_DIR/.."
tar --xz -cf "$PACKAGE_PATH" \
    --transform="s/^staging/splinter3D-${VERSION}-linux-${TRIPLET}/" \
    "staging"

if [[ ! -f "$PACKAGE_PATH" ]]; then
    die "Failed to create package: $PACKAGE_PATH"
fi

msg "Package created: $PACKAGE_PATH"

# Calculate SHA256
msg "Computing SHA256..."
sha256sum "$PACKAGE_PATH" > "$SHA256_FILE"
msg "SHA256: $(cat "$SHA256_FILE")"

msg "✓ Packaging complete"
