set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. "$SCRIPT_DIR/io.sh"
. "$SCRIPT_DIR/vcpkg.sh"
. "$SCRIPT_DIR/build_tools.sh"

function _configure_and_build() {
    local build_type="$1"; shift
    local extra_cmake_flags=("$@")

    _ensure_tools
    _info "updating external submodules..."
    git submodule update --init --recursive
    _success "updated external submodules !"
    _set_vcpkg_targets
    extra_cmake_flags+=("${VCPKG_FLAGS[@]}")
    mkdir -p build
    cd build || _error "mkdir failed" "could not cd into build/"
    local build_system
    build_system=$(_choose_build_system)
    _info "configuring with CMake (${build_system}, ${build_type})..."
    cmake_cmd=(cmake .. -G "${build_system}" -DCMAKE_BUILD_TYPE="${build_type}" "${extra_cmake_flags[@]}")
    if [ "${DRY_RUN:-}" = "1" ]; then
        _success "DRY RUN: ${cmake_cmd[*]}"
        return 0
    fi
    "${cmake_cmd[@]}" || _error "cmake configuration failed" "check CMake output above"

    _info "building target splinter with ${build_system}..."
    if _run_build_tool "${build_system}" "splinter"; then
        _success "compiled splinter"
        exit 0
    else
        _error "compilation error" "failed to compile splinter"
    fi
}
