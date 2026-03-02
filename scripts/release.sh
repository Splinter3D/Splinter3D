#!/usr/bin/env bash
set -euo pipefail

# release.sh — Create changelog + tag + push using Commitizen (cz)
#
# Usage:
#   ./release.sh                     # auto bump + changelog + tag + push
#   ./release.sh 1.2.3               # manual tag mode
#   ./release.sh --dry-run           # simulate auto mode
#   ./release.sh 1.2.3 --dry-run     # simulate manual mode
#
# Notes:
# - If passing a manual tag, it must match your configured tag format.
# - --dry-run will NOT create commits, tags, or push.

DRY_RUN=false
TAG=""

for arg in "$@"; do
  case "$arg" in
    -h | --help)
      echo "Usage: $0 [TAG] [--dry-run]"
      echo "  TAG: Optional. If provided, creates a release with this tag. Otherwise, auto-bumps."
      echo "  --dry-run: Simulate the release process without making any changes."
      exit 0
      ;;
    --dry-run)
      DRY_RUN=true
      ;;
    *)
      TAG="$arg"
      ;;
  esac
done

die() { echo "ERROR: $*" >&2; exit 1; }

run() {
  if $DRY_RUN; then
    echo "[DRY-RUN] $*"
  else
    "$@"
  fi
}

command -v git >/dev/null 2>&1 || die "git not found"
command -v cz  >/dev/null 2>&1 || die "cz not found"

git rev-parse --is-inside-work-tree >/dev/null 2>&1 || die "Not in git repo"

if ! $DRY_RUN; then
  if ! git diff --quiet || ! git diff --cached --quiet; then
    die "Working tree not clean."
  fi
fi

REMOTE="${REMOTE:-origin}"
BRANCH="$(git rev-parse --abbrev-ref HEAD)"

if [[ -z "$TAG" ]]; then
  echo "==> Auto release mode"

  if $DRY_RUN; then
    echo "[DRY-RUN] cz bump --changelog --yes"
    cz bump --changelog --yes --dry-run || true
    echo "[DRY-RUN] git push $REMOTE $BRANCH"
    echo "[DRY-RUN] git push $REMOTE --tags"
  else
    cz bump --changelog --yes
    git push "$REMOTE" "$BRANCH"
    git push "$REMOTE" --tags
  fi

  echo "==> Done"
  exit 0
fi

if [[ "$TAG" =~ ^[0-9]+(\.[0-9]+)*$ ]]; then
  TAG="v$TAG"
  echo "Normalized tag to $TAG (prefixed 'v')"
fi

if ! [[ "$TAG" =~ ^v[0-9]+(\.[0-9]+)*$ ]]; then
  die "Invalid tag format: '$TAG'. Expected format like 'v1.2.3'."
fi

echo "==> Manual release for tag: $TAG"

if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null; then
  die "Tag '$TAG' already exists."
fi

if $DRY_RUN; then
  echo "[DRY-RUN] cz changelog --unreleased-version=\"$TAG\""
  cz changelog --unreleased-version="$TAG" --dry-run || true
  echo "[DRY-RUN] git add CHANGELOG.md"
  echo "[DRY-RUN] git commit -m \"chore(release): $TAG\""
  echo "[DRY-RUN] git tag -a \"$TAG\" -m \"Release $TAG\""
  echo "[DRY-RUN] git push $REMOTE $BRANCH"
  echo "[DRY-RUN] git push $REMOTE \"$TAG\""
else
  cz changelog --unreleased-version="$TAG"

  if ! git diff --quiet -- "CHANGELOG.md"; then
    git add CHANGELOG.md
    git commit -m "chore(release): $TAG"
  fi

  git tag -a "$TAG" -m "Release $TAG"
  git push "$REMOTE" "$BRANCH"
  git push "$REMOTE" "$TAG"
fi

echo "==> Done"
