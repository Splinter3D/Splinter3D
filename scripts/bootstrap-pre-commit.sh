#!/usr/bin/env bash
set -euo pipefail
if ! command -v pre-commit &> /dev/null; then
    python3 -m pip install --user pre-commit || echo "Could not install pre-commit. Please install it with your package manager." && exit 1
fi
pre-commit install
pre-commit install --hook-type pre-push || true
pre-commit run --all-files || true
echo "pre-commit hooks installed."
