# .github/workflows/

GitHub Actions automation: compiling PRs, packaging and releasing builds, and enforcing the branching policy described below.

## Branching and release flow

- Feature/fix branches target `dev`. Hotfixes target `main` from a `hotfix/*` branch.
- `main-pr-policy.yml` rejects any PR into `main` that isn't from `dev` or `hotfix/*`.
- `prepare-release-pr.yml` turns a merge into `dev`/`main` into a release branch (`release/dev` or `hotfix/release-main`) and opens a PR back into that same branch, bumping `CHANGELOG.md` and `vcpkg.json`.
- `tag-on-merge.yml` tags the repo when that release PR is merged, then triggers `ci-release.yml` for the new tag (an RC tag from `dev`, a stable tag from `main`).
- `ci-release.yml` builds every platform, publishes Docker images, and creates the GitHub Release.
- `backmerge-main-to-dev.yml` opens a PR to bring a merged `hotfix/*` back into `dev` so it isn't lost on the next `dev` release.

## Contents

- `ci-release.yml` - **CI Release (matrix)**. Triggers on `v*.*.*` tag pushes or manual `workflow_dispatch` (with a `tag` input). Resolves the release environment (`production` for tags reachable from `main`, `dev` otherwise), builds every OS/arch target through `reusable-build.yml`, builds and pushes multi-arch Docker images to GHCR, then creates the GitHub Release (uploading all packages) and assembles the multi-arch Docker manifest (plus `:latest` for stable releases).
- `compile-on-pr.yml` - **Compile on PR onto main (matrix)**. Triggers on PRs into `main`; only runs when the source branch is `dev` or `hotfix/*` (excluding `hotfix/release-main`) - i.e. it validates a release PR compiles on every platform before it can be merged. Uses `reusable-build.yml` with `build_only: true` (no signing/notarization/artifact upload) and additionally builds (without pushing) a Docker image.
- `main-pr-policy.yml` - **main-pr-policy**. Triggers on PR open/reopen/sync/edit/ready-for-review into `main`. Closes and comments on any PR whose head branch isn't `dev` or `hotfix/*`, enforcing where `main` PRs may come from.
- `pre-commit-on-pr.yml` - **Pre-commit on PR**. Triggers on PRs into `dev` or `main` (skipped for `hotfix/release-main` and `main` heads). Installs `pre-commit` and runs `pre-commit run --all-files --show-diff-on-failure`, i.e. the same checks as `.pre-commit-config.yaml` / `scripts/bootstrap-pre-commit.sh`, enforced in CI.
- `prepare-release-pr.yml` - **prepare-release-pr**. Triggers when a PR into `dev` or `main` is merged, or via manual `workflow_dispatch` (choosing the target branch and optionally an explicit tag). Computes the next tag with `scripts/next-tag.sh` (`rc` for `dev`, `stable` for `main`), regenerates `CHANGELOG.md` with `commitizen`, aligns `vcpkg.json` with `scripts/check-versions.sh --fix`, pushes the changes to `release/dev` (targeting `dev`) or `hotfix/release-main` (targeting `main`), and opens or updates the corresponding release PR. A `concurrency` group per target branch prevents overlapping runs.
- `reusable-build.yml` - **Build package (reusable)**, called via `workflow_call` from `ci-release.yml` and `compile-on-pr.yml` (never triggered directly). Checks out the requested ref, imports the Apple Developer ID signing certificate on macOS when provided, runs the caller-supplied `install_command`/`build_command`/`verify_command`, then (unless `build_only`) notarizes and staples the macOS `.dmg` and uploads the package + its `.sha256` as a build artifact.
- `backmerge-main-to-dev.yml` - **backmerge-main-to-dev**. Triggers when a PR into `main` is merged. If the merged branch was `hotfix/*` (excluding `hotfix/release-main`), opens a PR merging `main` back into `dev` (skipped if one is already open), so hotfixes aren't lost on the next `dev`-based release.
- `tag-on-merge.yml` - **tag-on-merge**. Triggers when a PR into `dev` or `main` is merged. `tag-dev`: when `release/dev` is merged into `dev`, extracts the RC tag recorded in the PR body, creates and pushes that git tag, and dispatches `ci-release.yml` for it. `tag-main`: same for `hotfix/release-main` merged into `main`, with a stable tag. A `concurrency` group per base branch serializes releases.

### scripts/

Helper scripts used only by the workflows above (as opposed to `../../scripts/`, used by contributors and CI alike).

- `next-tag.sh <stable|rc>` - computes the next release tag from git tags and `commitizen`. `stable` returns the next `vX.Y.Z` after clearing pending RC tags for that version; `rc` returns the next `vX.Y.Z-rc.N` for the upcoming version. Used by `prepare-release-pr.yml`.
- `Linux/install.sh` - installs Linux build dependencies via `apt-get` (build tools, CMake, X11/OpenGL headers, autotools) on the GitHub-hosted Ubuntu runners.
- `Linux/package-linux.sh` - stages the built `splinter3D` binary with `locale/`, `assets/`, and top-level docs, then archives them into `output/splinter3D-<version>-linux-<arch>.tar.xz` plus a `.sha256` file. The version is read from `git describe --tags`.
- `MacOS/install.sh` - installs macOS build dependencies (`cmake`, `ninja`, `pkg-config`) via Homebrew, skipping packages already present.
- `MacOS/package-macos.sh` - assembles a `Splinter3D.app` bundle (binary, `Info.plist`, icon, locale, assets), code-signs it when `CODESIGN_IDENTITY` is set, and packages it into `output/splinter3D-<version>-macos-<arch>.dmg` plus a `.sha256` file.
- `Windows/install.ps1` - clones and bootstraps vcpkg into the project root, installs Chocolatey (plus any packages listed in `.choco-dependencies`) and falls back to `winget` for missing tools, imports the MSVC environment, then runs `vcpkg install` in manifest mode against `vcpkg.json`.
- `Windows/package-windows.ps1` - locates the built `splinter3D.exe` (and its runtime DLLs) under `build/`, stages it with `locale/`, `assets/`, and top-level docs, then compresses it into `output/splinter3D-<version>-windows-<arch>.zip` plus a `.sha256` file.

These platform scripts are consumed as matrix entries (`install_command`/`build_command`) by `ci-release.yml` and `compile-on-pr.yml`, and executed inside `reusable-build.yml`.
