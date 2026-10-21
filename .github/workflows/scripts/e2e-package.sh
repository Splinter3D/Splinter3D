#!/usr/bin/env bash
# Runs the end-to-end tests (tests/end_to_end) against the binary inside a release
# package rather than the one in build/, to catch packaging mistakes (missing assets,
# libraries, wrong layout). Usage: e2e-package.sh <package.tar.xz|package.dmg>

set -euo pipefail

PACKAGE="${1:?usage: e2e-package.sh <package.tar.xz|package.dmg>}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
WORK_DIR="$(mktemp -d)"
MOUNT_POINT=""

cleanup() {
    [[ -n "$MOUNT_POINT" ]] && hdiutil detach "$MOUNT_POINT" -quiet || true
    rm -rf "$WORK_DIR"
}
trap cleanup EXIT

case "$PACKAGE" in
    *.tar.xz)
        tar -xJf "$PACKAGE" -C "$WORK_DIR"
        BINARY="$(find "$WORK_DIR" -type f -name splinter3D -perm -u+x | head -1)"
        ;;
    *.dmg)
        MOUNT_POINT="$WORK_DIR/mnt"
        mkdir -p "$MOUNT_POINT"
        hdiutil attach "$PACKAGE" -mountpoint "$MOUNT_POINT" -nobrowse -quiet
        BINARY="$(find "$MOUNT_POINT" -path '*.app/Contents/MacOS/splinter3D' -type f | head -1)"
        ;;
    *)
        echo "ERROR: unsupported package type: $PACKAGE" >&2
        exit 1
        ;;
esac

if [[ -z "$BINARY" ]]; then
    echo "ERROR: no splinter3D executable found in $PACKAGE" >&2
    exit 1
fi
echo "[e2e-package] testing $BINARY"

RUNNER=()
# The GUI smoke test needs a display; batch-mode tests run headless either way.
if [[ "$(uname)" == "Linux" && -z "${DISPLAY:-}" ]] && command -v xvfb-run > /dev/null; then
    RUNNER=(xvfb-run --auto-servernum --server-args="-screen 0 1920x1080x24")
fi

cd "$PROJECT_ROOT"
SPLINTER3D_BIN="$BINARY" LIBGL_ALWAYS_SOFTWARE=1 \
    ${RUNNER[@]+"${RUNNER[@]}"} python3 -m unittest discover -s tests/end_to_end -v
