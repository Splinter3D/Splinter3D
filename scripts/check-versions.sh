#!/usr/bin/env bash
set -euo pipefail

FIX=false
TAG=""

for arg in "$@"; do
  case "$arg" in
    -h | --help)
        echo "Usage: $0 [--fix] [TAG]"
        echo "  --fix: Check versions and attempt to fix mismatches by aligning all version sources to the SCM version."
        echo "  TAG: Optional. If provided, uses this tag as the SCM version for comparison. Otherwise, uses the latest git tag."
        exit 0
        ;;
    --fix)
        FIX=true
        ;;
    *)
        TAG="$arg"
        ;;
  esac
done

if ! command -v git &> /dev/null; then
    echo "git is required but not found. Please install git and try again."
    exit 1
fi

if ! command -v jq &> /dev/null; then
    echo "jq is required but not found. Please install jq and try again."
    exit 1
fi

die() { echo "ERROR: $*" >&2; exit 1; }

getSCMVersion() {
    if [[ -n "$TAG" ]]; then
        if [[ "$TAG" =~ ^[0-9]+(\.[0-9]+)*$ ]]; then
            TAG="v$TAG"
            echo "Normalized tag to $TAG (prefixed 'v')" >&2
        fi

        if ! [[ "$TAG" =~ ^v[0-9]+(\.[0-9]+)*(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$ ]]; then
            die "Invalid tag format: '$TAG'. Expected format like 'v1.2.3' or 'v1.2.3-rc.1'."
        fi
        echo "${TAG#v}"
        return
    fi

    latest="$(git tag --list 'v[0-9]*.[0-9]*.[0-9]*' --sort=-v:refname | head -n1)"
    if [[ -z "$latest" ]]; then
        latest="v0.0.0"
    fi
    echo "${latest#v}"
}

getVcpkgVersion() {
    jq -r '."version-semver"' vcpkg.json
}

setVcpkgVersion() {
    local new_version="$1"
    tmp_file="$(mktemp)"
    jq --arg version "$new_version" '."version-semver" = $version' vcpkg.json > "$tmp_file"
    mv "$tmp_file" vcpkg.json
}

SCM_V=$(getSCMVersion)
VCPKG_V=$(getVcpkgVersion)

if [[ "$VCPKG_V" != "$SCM_V" ]]; then
    echo "Version mismatch detected!"
    echo "  vcpkg.json: $VCPKG_V"
    echo "  SCM: $SCM_V"

    if [[ "$FIX" == true ]]; then
        echo "Attempting to fix version mismatches..."
        setVcpkgVersion "$SCM_V"

        # Verify the fix
        NEW_V=$(getVcpkgVersion)
        if [[ "$NEW_V" != "$SCM_V" ]]; then
            echo "Failed to fix version mismatches. Please check the files manually."
            exit 1
        else
            echo "All versions are now consistent with SCM version: $SCM_V"
            exit 0
        fi
    else
        exit 1
    fi
fi

echo "All versions are consistent: $SCM_V"
exit 0
