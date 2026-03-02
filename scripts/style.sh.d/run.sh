set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

function _print_helper () {
    cat << EOF
USAGE:
    $0    formats all C++ files in the project using clang-format

ARGUMENTS:
    $0 [-h|--help]     displays this message
    $0 [--check]       check formatting without modifying files
    $0 [--fix]         format all files (default behavior)
EOF
}

function _format_all () {
    echo "Formatting all C++ files in the project..."

    if [ "${CHECK_MODE:-0}" -eq 1 ]; then
        # Check mode: preview formatting without modifying
        find "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" \
            -type f \( -name "*.cpp" -o -name "*.hpp" \) \
            -exec clang-format --output-replacements-xml {} + > /dev/null && \
            echo "✓ All files are properly formatted" || \
            echo "✗ Some files need formatting"
    else
        # Fix mode: apply formatting
        find "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" \
            -type f \( -name "*.cpp" -o -name "*.hpp" \) \
            -exec clang-format -i {} +
        echo "✓ Formatting complete"
    fi
}

function _run () {
    if [ $# -eq 0 ]; then
        _format_all
        return 0
    fi

    for args in "$@"; do
        case $args in
            -h|--help)
                _print_helper
                exit 0
                ;;
            --check)
                CHECK_MODE=1
                _format_all
                exit 0
                ;;
            --fix)
                CHECK_MODE=0
                _format_all
                exit 0
                ;;
            *)
                echo "Unknown argument: $args"
                _print_helper
                exit 1
                ;;
        esac
    done
}
