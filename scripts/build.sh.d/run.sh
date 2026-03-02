set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. "$SCRIPT_DIR/rules.sh"
. "$SCRIPT_DIR/unit_tests.sh"

function _print_helper () {
    cat << EOF
USAGE:
    $0    builds splinter3D project

ARGUMENTS:
    $0 [-h|--help]    displays this message
    $0 [-d|--debug]   debug flags compilation
    $0 [-c|--clean]   clean the project
    $0 [-f|--fclean]  fclean the project
    $0 [-t|--tests]   run unit tests
    $0 [-r|--re]      fclean then rebuild (release)
EOF
}

function _run () {
    if [ $# -eq 0 ]; then
        _all
    fi

    for args in "$@"; do
        case $args in
            --auto-vcpkg)
                AUTO_VCPKG=1
                if [ $# -eq 1 ]; then
                    _all
                fi
                ;;
            --dry-run)
                DRY_RUN=1
                if [ $# -eq 1 ]; then
                    _all
                fi
                ;;
            -h|--help)
                _print_helper "$@"
                exit 0
                ;;
            -c|--clean)
                _clean
                exit 0
                ;;
            -f|--fclean)
                _fclean
                exit 0
                ;;
            -d|--debug)
                _debug
                exit 0
                ;;
            -t|--tests)
                _tests_run
                exit 0
                ;;
            -r|--re)
                _fclean
                _all
                exit 0
                ;;
            *)
                _error "Invalid arguments:" "$args"
                ;;
        esac
    done
}
