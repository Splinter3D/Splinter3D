#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-}"
DEFAULT_VERSION="${DEFAULT_VERSION:-0.1.0}"

die() {
  echo "ERROR: $*" >&2
  exit 1
}

command -v git >/dev/null 2>&1 || die "git not found"
command -v cz >/dev/null 2>&1 || die "cz not found"

if [[ "$MODE" != "stable" && "$MODE" != "rc" ]]; then
  die "Usage: $0 <stable|rc>"
fi

if git remote get-url origin >/dev/null 2>&1; then
  git fetch --force --tags origin >/dev/null 2>&1
fi

latest_stable_tag="$(git tag --list 'v[0-9]*.[0-9]*.[0-9]*' --sort=-version:refname | head -n 1)"
base_version="$DEFAULT_VERSION"

if [[ -n "$latest_stable_tag" ]]; then
  while IFS= read -r rc_tag; do
    [[ -n "$rc_tag" ]] || continue
    git tag -d "$rc_tag" >/dev/null
  done < <(git tag --list 'v*-rc.*')
  base_version="$(cz bump --get-next --yes)"
fi

if [[ -z "$base_version" ]]; then
  die "unable to determine next version"
fi

if [[ "$MODE" == "stable" ]]; then
  echo "v${base_version}"
  exit 0
fi

latest_rc_number="$(
  git tag --list "v${base_version}-rc.*" \
    | sed -E 's/.*-rc\.([0-9]+)$/\1/' \
    | sort -n \
    | tail -n 1
)"

if [[ -z "$latest_rc_number" ]]; then
  latest_rc_number=0
fi

echo "v${base_version}-rc.$((latest_rc_number + 1))"
