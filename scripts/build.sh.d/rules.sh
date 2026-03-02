set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. "$SCRIPT_DIR/configure_build.sh"

function _all() {
    _configure_and_build "Release"
}

function _debug() {
    _fclean
    _configure_and_build "Debug" -DENABLE_DEBUG=ON
}

function _clean()
{
    rm -rf build
}

function _fclean()
{
    _clean
    rm -rf ./*.so ./*.dylib ./*.dll ./*.lib ./*.exp ./*.a ./*.ilk ./*.pdb \
        splinter_unit_tests splinter_unit_tests.exe splinter splinter.exe \
        unit_tests plugins code_coverage.txt unit_tests-*.profraw unit_tests.profdata vgcore* cmake-build-debug
}
