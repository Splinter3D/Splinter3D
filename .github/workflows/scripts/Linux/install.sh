#!/usr/bin/env bash
# Install system dependencies for Linux build

set -euo pipefail

echo "[install.sh] Installing system dependencies..."

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    xz-utils \
    libxinerama-dev \
    libxcursor-dev \
    xorg-dev \
    libglu1-mesa-dev \
    autoconf \
    autoconf-archive \
    automake \
    libtool \
    libltdl-dev

echo "[install.sh] ✓ System dependencies installed"
