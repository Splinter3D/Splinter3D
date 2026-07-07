#!/usr/bin/env bash
# Install system dependencies for macOS build

set -euo pipefail

PROJECT_ROOT="${1:-.}"

echo "[install.sh] Installing system dependencies for macOS..."

if ! command -v brew >/dev/null 2>&1; then
    echo "[install.sh] ERROR: Homebrew is required but not available on this runner" >&2
    exit 1
fi

packages=(cmake ninja pkg-config)
for pkg in "${packages[@]}"; do
    if brew list --versions "$pkg" >/dev/null 2>&1; then
        echo "[install.sh] $pkg already installed"
    else
        echo "[install.sh] Installing $pkg..."
        brew install "$pkg"
    fi
done

echo "[install.sh] ✓ Dependencies ready"
