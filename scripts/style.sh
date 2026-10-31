#!/usr/bin/env bash
# Formats or checks all C++ sources under src/ and include/ using clang-format.
# The actual logic and --help text live in style.sh.d/run.sh, sourced below.

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. "$SCRIPT_DIR/style.sh.d/run.sh"

_run "$@"
