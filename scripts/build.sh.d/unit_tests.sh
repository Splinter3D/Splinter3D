set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. "$SCRIPT_DIR/io.sh"
. "$SCRIPT_DIR/vcpkg.sh"
. "$SCRIPT_DIR/build_tools.sh"

function _tests_run()
{
    _ensure_tools
    _set_vcpkg_targets
    mkdir -p build
    cd build || _error "mkdir failed" "could not cd into build/"
    local build_system
    build_system=$(_choose_build_system)
    _info "configuring tests with CMake (${build_system}, Debug)..."
    cmake .. -G "${build_system}" -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON "${VCPKG_FLAGS[@]}" \
        || _error "cmake configuration failed" "check CMake output above"

    _info "building unit tests (splinter_unit_tests)…"
    if ! _run_build_tool "${build_system}" "splinter_unit_tests"; then
        _error "unit tests compilation error" "failed to compile splinter_unit_tests"
    fi
    cd .. || _error "cd failed" "could not cd .."

    _info "running unit tests…"
    if ! ./splinter_unit_tests; then
        _error "unit tests error" "unit tests failed!"
    fi
    _success "unit tests succeed!"

    if [ "$(uname -s)" == 'Darwin' ]; then
        _info "generating coverage (llvm-cov)…"
        xcrun llvm-profdata merge -sparse unit_tests-*.profraw -o unit_tests.profdata
        xcrun llvm-cov report ./splinter_unit_tests -instr-profile=unit_tests.profdata -object ./lib*.dylib -ignore-filename-regex='.*/tests/.*' -enable-name-compression > ../code_coverage.txt
        cd ..
    else
        if command -v gcovr >/dev/null 2>&1; then
            _info "generating coverage (gcovr)…"
            gcovr -r . --exclude tests/ > code_coverage.txt
        else
            _info "gcovr not found; skipping coverage generation."
            echo "gcovr not installed; coverage skipped." > code_coverage.txt
        fi
    fi
    cat code_coverage.txt
}
