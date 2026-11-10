#!/usr/bin/env bash
# Computes the next release tag from git tags and commitizen.
# `stable` -> next vX.Y.Z (after discarding pending RC tags for that version).
# `rc`     -> next vX.Y.Z-rc.N for the upcoming version.
# Used by .github/workflows/prepare-release-pr.yml.
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
  git fetch --force --tags origin >/dev/null || die "could not fetch tags from origin"
fi

latest_stable_tag="$(
  git tag --list 'v[0-9]*.[0-9]*.[0-9]*' --sort=-version:refname \
    | { grep -E '^v[0-9]+\.[0-9]+\.[0-9]+$' || true; } \
    | head -n 1
)"
base_version="$DEFAULT_VERSION"

rc_tag_refs="$(git for-each-ref --format='%(refname:short) %(objectname)' 'refs/tags/v*-rc.*')"

restore_rc_tags() {
  while read -r name object; do
    [[ -n "$name" ]] || continue
    git update-ref "refs/tags/${name}" "$object"
  done <<< "$rc_tag_refs"
}
trap restore_rc_tags EXIT

if [[ -n "$latest_stable_tag" ]]; then
  while read -r name _; do
    [[ -n "$name" ]] || continue
    git tag -d "$name" >/dev/null
  done <<< "$rc_tag_refs"
  base_version="$(cz bump --get-next --yes)"
  restore_rc_tags
fi

if [[ -z "$base_version" ]]; then
  die "unable to determine next version"
fi

if [[ "$MODE" == "stable" ]]; then
  echo "v${base_version}"
  exit 0
fi

latest_rc_number="$(
  { grep -E "^v${base_version//./\\.}-rc\.[0-9]+ " <<< "$rc_tag_refs" || true; } \
    | sed -E 's/^.*-rc\.([0-9]+) .*$/\1/' \
    | sort -n \
    | tail -n 1
)"

if [[ -z "$latest_rc_number" ]]; then
  latest_rc_number=0
fi

echo "v${base_version}-rc.$((latest_rc_number + 1))"
