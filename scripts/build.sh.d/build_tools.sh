set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. "$SCRIPT_DIR/io.sh"
. "$SCRIPT_DIR/misc.sh"

function _ensure_tools() {
    missing=()
    for t in git cmake; do
        if ! command -v "$t" >/dev/null 2>&1; then
            missing+=("$t")
        fi
    done
    local desired_generator=""
    if [ -n "${CMAKE_GENERATOR:-}" ]; then
        desired_generator="${CMAKE_GENERATOR}"
    elif [ -n "${BUILD_SYSTEM:-}" ]; then
        desired_generator="${BUILD_SYSTEM}"
    else
        if command -v ninja >/dev/null 2>&1; then
            desired_generator="Ninja"
        else
            desired_generator="Unix Makefiles"
        fi
    fi
    local required_build_tool=""
    if [[ "${desired_generator}" == *Makefiles* ]]; then
        required_build_tool="make"
    elif [[ "${desired_generator}" == "Ninja" ]]; then
        required_build_tool="ninja"
    else
        if command -v ninja >/dev/null 2>&1; then
            required_build_tool="ninja"
        elif command -v make >/dev/null 2>&1; then
            required_build_tool="make"
        else
            required_build_tool="none"
        fi
    fi
    if [ "${required_build_tool}" = "none" ]; then
        missing+=("ninja or make")
    else
        if ! command -v "${required_build_tool}" >/dev/null 2>&1; then
            missing+=("${required_build_tool}")
        fi
    fi
    if [ ${#missing[@]} -ne 0 ]; then
        hint="Please install: ${missing[*]}"
        _error "required tool(s) missing" "$hint"
    fi
    _success "required tools found (git, cmake, ${required_build_tool})"
}

function _invoke_build_tool() {
    local build_system="$1"
    local target="$2"

    if [[ "${build_system}" == *Makefiles* ]]; then
        printf '%s' "make -j$(_cpus) ${target}"
    elif [[ "${build_system}" == "Ninja" ]]; then
        printf '%s' "ninja -j$(_cpus) ${target}"
    else
        if command -v ninja >/dev/null 2>&1; then
            printf '%s' "ninja -j$(_cpus) ${target}"
        elif command -v make >/dev/null 2>&1; then
            printf '%s' "make -j$(_cpus) ${target}"
        else
            _error "No supported build tool found" "install ninja or make"
        fi
    fi
}

function _run_build_tool() {
    local build_system="$1"
    local target="$2"
    local cmd

    cmd=$(_invoke_build_tool "${build_system}" "${target}")

    if [ "${DRY_RUN:-}" = "1" ]; then
        _success "DRY RUN: ${cmd}"
        return 0
    fi

    if [[ "${build_system}" == *Makefiles* ]]; then
        make -j"$(_cpus)" "${target}"
        return $?
    elif [[ "${build_system}" == "Ninja" ]]; then
        ninja -j"$(_cpus)" "${target}"
        return $?
    else
        if command -v ninja >/dev/null 2>&1; then
            ninja -j"$(_cpus)" "${target}"
            return $?
        elif command -v make >/dev/null 2>&1; then
            make -j"$(_cpus)" "${target}"
            return $?
        else
            _error "No supported build tool found" "install ninja or make"
        fi
    fi
}

function _choose_build_system() {
    if [ -n "${CMAKE_GENERATOR:-}" ]; then
        printf '%s' "${CMAKE_GENERATOR}"
    elif ! command -v ninja >/dev/null 2>&1; then
        printf '%s' "Unix Makefiles"
    else
        printf '%s' "Ninja"
    fi
}
