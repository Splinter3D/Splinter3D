#!/usr/bin/env bash
# Unix entry point for the build system: locates a compatible Python 3.10+ interpreter,
# then forwards all arguments to Build/build.py, run from the repository root.
# See scripts/Build/README.md for the available flags.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PYTHON_CMD=""

# Some environments only expose newer interpreters under a versioned name
# (python3.12, python3.13...); probe common names for one that satisfies 3.10+.
for command in python python3 python3.14 python3.13 python3.12 python3.11 python3.10; do
    if command -v "$command" &> /dev/null || ! python3 -c "import sys; exit(sys.version_info < (3, 10))"; then
        PYTHON_CMD="$command"
        break
    fi
done

echo "Using Python command: $PYTHON_CMD (version: $($PYTHON_CMD --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+'))"

if [ -z "$PYTHON_CMD" ]; then
    echo "Python 3.10 or higher is required. Please install a compatible version of Python."
    exit 1
fi

cd "$SCRIPT_DIR/.." || exit 1

$PYTHON_CMD scripts/Build/build.py "$@"
