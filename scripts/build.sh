#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v python3 &> /dev/null; then
    echo "Python 3 is not installed. Please install Python 3 to continue."
    exit 1
fi

if ! command -v poetry &> /dev/null; then
    echo "Poetry is not installed. Please install Poetry to continue."
    exit 1
fi

poetry run -P "$SCRIPT_DIR/Build" -C "$SCRIPT_DIR/.." python3 scripts/Build/build.py "$@"
