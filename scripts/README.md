# scripts/

Developer tooling: building the project, formatting code, checking versions/locales, and bootstrapping local git hooks. Nothing here ships with the application.

## Contents

- `Build/` - the Python build system invoked by `build.sh`/`build.ps1`. See `Build/README.md` for its internal structure.
- `build.sh` - Unix entry point: picks a compatible Python interpreter (3.10+) and runs `Build/build.py` with the given arguments.
- `build.ps1` - Windows entry point: same role as `build.sh`, using PowerShell.
- `style.sh` - formats (or checks) all `.cpp`/`.hpp` files under `src/` and `include/` with `clang-format`, via `style.sh.d/run.sh`.
- `style.sh.d/run.sh` - implementation sourced by `style.sh`; not meant to be called directly.
- `check-versions.sh` - verifies that `vcpkg.json`'s `version-semver` matches the latest git tag (or a given tag), and can fix it in place.
- `check-locales.py` - verifies that every JSON file in `locale/` exposes the same keys and `{placeholder}` sets as the reference (first) locale file.
- `bootstrap-pre-commit.sh` - installs `pre-commit` if missing, registers the `pre-commit`/`pre-push` git hooks, and runs all hooks once against the repo.

## Usage

### Building

```bash
scripts/build.sh [options]      # Unix
scripts/build.ps1 [options]     # Windows
```

Both scripts only pick a Python interpreter and change directory before forwarding all arguments to `Build/build.py`; see `Build/README.md` for the available flags (`--tests`, `--clean`, `--rebuild`, `--ci`, etc.).

### Formatting C++ sources

```bash
scripts/style.sh            # format all files (default behavior)
scripts/style.sh --check    # check formatting without modifying files
scripts/style.sh --fix      # explicit format (same as no argument)
scripts/style.sh --help
```

### Version consistency

```bash
scripts/check-versions.sh            # compare vcpkg.json against the latest git tag
scripts/check-versions.sh v1.2.3     # compare against an explicit tag instead
scripts/check-versions.sh --fix      # align vcpkg.json to the SCM version on mismatch
```

Used by `prepare-release-pr.yml` (see `.github/workflows/README.md`) to keep `vcpkg.json` in sync with the release tag.

### Locale consistency

```bash
python3 scripts/check-locales.py
```

Run from the repository root. Exits non-zero if any `locale/*.json` file has missing/extra keys, a type mismatch, or a placeholder mismatch relative to the first file (alphabetically) in `locale/`.

### Pre-commit hooks

```bash
scripts/bootstrap-pre-commit.sh
```

One-time setup for new contributors: installs `pre-commit`, wires it to the `pre-commit`/`pre-push` git hooks (see `.pre-commit-config.yaml`), then runs it once on the whole repo.
