#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v python3 &> /dev/null; then
    echo "Python 3 is not installed. Please install Python 3 to continue."
    exit 1
fi

if ! python3 -c "import sys; exit(sys.version_info < (3, 10))"; then
    echo "Python 3.10 or higher is required. Please upgrade your Python installation."
    exit 1
fi

cd "$SCRIPT_DIR/.." || exit 1

python3 scripts/Build/build.py "$@"
