#!/usr/bin/env bash
# One-time setup for local git hooks: installs pre-commit if missing, registers the
# pre-commit/pre-push hooks, then runs all hooks once against the whole repository.
set -euo pipefail
if ! command -v pre-commit &> /dev/null; then
    python3 -m pip install --user pre-commit || echo "Could not install pre-commit. Please install it with your package manager." && exit 1
fi
pre-commit install
pre-commit install --hook-type pre-push || true
pre-commit run --all-files || true
echo "pre-commit hooks installed."
