set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

. "$SCRIPT_DIR/io.sh"

function _clone_vcpkg() {
    local dest="${1:-}"
    local default
    default=$(realpath "${VCPKG_ROOT}")

    if [ -z "${dest}" ]; then
        dest=$( _prompt "Where to install vcpkg? (default: ${default})" )
        dest="${dest:-$default}"
    fi
    dest=$(realpath "${dest}")
    dest="${dest%/}"
    if [[ "${dest}" != */vcpkg ]]; then
        dest="${dest}/vcpkg"
    fi
    parent_dir="$(dirname "$dest")"
    mkdir -p "$parent_dir" || return 1
    git clone https://github.com/microsoft/vcpkg.git "${dest}" || return 1
    printf '%s' "${dest}"
}

function _bootstrap_vcpkg() {
    local dir="$1"
    if [ -z "${dir}" ]; then
        _info "No directory provided to _bootstrap_vcpkg"
        return 1
    fi
    if [ ! -d "${dir}" ]; then
        _info "vcpkg directory not found: ${dir}"
        return 1
    fi
    pushd "${dir}" > /dev/null || return 1
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*|WindowsNT)
            ./bootstrap-vcpkg.bat
            ;;
        *)
            ./bootstrap-vcpkg.sh
            ;;
    esac
    popd > /dev/null || true
}

function _install_and_bootstrap_vcpkg() {
    local answer

    while true; do
        answer=$( _prompt "vcpkg not detected, would you like to install it? (y/n)" )
        case "${answer}" in
            [Yy]*)
                _info "Installing vcpkg..."
                local dir
                dir=$( _clone_vcpkg ) || { _info "vcpkg clone failed"; break; }
                _bootstrap_vcpkg "${dir}"
                _success "Successfully installed vcpkg"
                break
                ;;
            [Nn]*)
                _info "Skipping vcpkg installation."
                break
                ;;
            *)
                _info "Please answer y or n."
                ;;
        esac
    done
}

function _set_vcpkg_targets() {
    _info "Setting VCPKG targets"
    if [ -z "${VCPKG_ROOT:-}" ]; then
        VCPKG_ROOT="$HOME/vcpkg"
        _info "VCPKG_ROOT not set; defaulting to ${VCPKG_ROOT}"
    else
        local rp
        rp=$(realpath "$VCPKG_ROOT")
        if [ "$VCPKG_ROOT" != "${rp}" ]; then
            VCPKG_ROOT=$(realpath "${rp}")
            _info "Expanded VCPKG_ROOT to ${VCPKG_ROOT}"
        fi
    fi

    if [ ! -d "$VCPKG_ROOT" ]; then
        if [ "${AUTO_VCPKG:-}" = "1" ]; then
            _info "Auto-installing vcpkg into ${VCPKG_ROOT}"
            dir=$( _clone_vcpkg "$VCPKG_ROOT" ) || { _info "vcpkg clone failed"; }
            if [ -n "${dir}" ]; then
                _bootstrap_vcpkg "${dir}"
                _success "Successfully installed vcpkg"
            fi
        else
            _install_and_bootstrap_vcpkg
        fi
    fi

    if [ -z "${VCPKG_TARGET_TRIPLET:-}" ]; then
        case "$(uname -s)" in
            Darwin)
                case "$(uname -m)" in
                    arm64) VCPKG_TARGET_TRIPLET="arm64-osx" ;;
                    *) VCPKG_TARGET_TRIPLET="x64-osx" ;;
                esac
                ;;
            Linux)
                case "$(uname -m)" in
                    aarch64|arm64) VCPKG_TARGET_TRIPLET="arm64-linux" ;;
                    *) VCPKG_TARGET_TRIPLET="x64-linux" ;;
                esac
                ;;
            MINGW*|MSYS*|CYGWIN*|WindowsNT)
                case "$(uname -m)" in
                    aarch64|arm64) VCPKG_TARGET_TRIPLET="arm64-windows" ;;
                    *) VCPKG_TARGET_TRIPLET="x64-windows" ;;
                esac
                ;;
            *)
                VCPKG_TARGET_TRIPLET="x64-linux" ;;
        esac
        _info "Auto-selected VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET} based on platform"
    fi

    VCPKG_FLAGS=()
    if [ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]; then
        VCPKG_FLAGS+=("-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        if [ -n "${VCPKG_TARGET_TRIPLET:-}" ]; then
            VCPKG_FLAGS+=("-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
        fi
        _success "Using vcpkg toolchain from ${VCPKG_ROOT} (triplet=${VCPKG_TARGET_TRIPLET})"
    else
        _info "vcpkg toolchain file not found at ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake; vcpkg integration skipped"
    fi
}
