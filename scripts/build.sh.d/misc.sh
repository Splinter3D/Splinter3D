set -euo pipefail

function _cpus() {
    if command -v nproc >/dev/null 2>&1; then
        nproc
    elif [ "$(uname -s)" = "Darwin" ]; then
        sysctl -n hw.ncpu
    else
        echo 4
    fi
}
