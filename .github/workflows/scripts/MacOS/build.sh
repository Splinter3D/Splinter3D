#!/usr/bin/env bash
# Build project and create macOS package

set -euo pipefail

PROJECT_ROOT="${1:-.}"

echo "[build.sh] Starting build (ProjectRoot: $PROJECT_ROOT)"

# Call the existing build script with --auto-vcpkg to skip interactive prompts in CI
cd "$PROJECT_ROOT"
echo "[build.sh] Calling ./scripts/build.sh --auto-vcpkg..."
./scripts/build.sh --auto-vcpkg

echo "[build.sh] Creating package..."
chmod +x ./.github/workflows/scripts/MacOS/package-macos.sh
./.github/workflows/scripts/MacOS/package-macos.sh

echo "[build.sh] ✓ Build and packaging complete"
