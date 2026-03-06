#!/usr/bin/env bash
# Build project and create Linux package

set -euo pipefail

PROJECT_ROOT="${1:-.}"

echo "[build.sh] Starting build (ProjectRoot: $PROJECT_ROOT)"

# Call the existing build script with --auto-vcpkg to skip interactive prompts in CI
echo "[build.sh] Calling ./scripts/build.sh --auto-vcpkg..."
cd "$PROJECT_ROOT"
./scripts/build.sh --auto-vcpkg

# Then call packaging
echo "[build.sh] Creating package..."
chmod +x ./.github/workflows/scripts/Linux/package-linux.sh
./.github/workflows/scripts/Linux/package-linux.sh

echo "[build.sh] ✓ Build and packaging complete"
