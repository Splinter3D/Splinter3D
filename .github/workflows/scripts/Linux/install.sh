#!/usr/bin/env bash
# Install system dependencies for Linux build

set -euo pipefail

PROJECT_ROOT="${1:-.}"

echo "[install.sh] Installing system dependencies..."

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    gettext \
    xz-utils

echo "[install.sh] ✓ System dependencies installed"
