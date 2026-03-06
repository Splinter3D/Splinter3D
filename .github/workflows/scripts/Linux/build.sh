#!/usr/bin/env bash
# Build project and create Linux package

set -euo pipefail

PROJECT_ROOT="${1:-.}"

echo "[build.sh] Starting build (ProjectRoot: $PROJECT_ROOT)"

# Call the existing build script (it handles vcpkg + compilation)
echo "[build.sh] Calling ./scripts/build.sh..."
cd "$PROJECT_ROOT"
./scripts/build.sh

# Then call packaging
echo "[build.sh] Creating package..."
./.github/workflows/scripts/Linux/package-linux.sh

echo "[build.sh] ✓ Build and packaging complete"
