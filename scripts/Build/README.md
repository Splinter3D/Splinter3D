# Build/

The Python build system behind `scripts/build.sh` / `scripts/build.ps1`. It configures CMake, drives vcpkg, and builds/tests/cleans the project on Windows, Linux, and macOS.

## Contents

- `build.py` - entry point (`main()`): parses arguments, then dispatches to the clean/build/test/configure actions in the order dictated by the flags.
- `ParseArgs/` - `argparse`-based CLI definition (`args`), imported by every module below. Run `python3 scripts/Build/build.py --help` for the full flag list.
- `Logger/` - `logger`: leveled, colored console output (`debug`/`info`/`warn`/`error`/`input`); `debug` messages only print when `--debug` is passed.
- `Platform/` - platform (`Platform`, `get_platform`) and architecture (`Arch`, `get_arch`) detection, plus the tools required per platform (`PLATFORM_REQUIRED_TOOLS`).
- `Requirements/` - `check_requirements()`: fails fast if a tool required on the current platform (compiler, CMake, Ninja/Make...) is missing.
- `Submodules/` - `update_submodules()`: runs `git submodule update --init --recursive`, skipped when `--skip-submodules` is passed.
- `vcpkg/` - vcpkg lifecycle: detecting an existing install (`Detect.py`), validating it targets the current platform (`Validation.py`), cloning/bootstrapping it (`Install.py`), and computing the CMake toolchain/triplet flags for it (`SetTargets.py`).
- `ConfigureBuild/` - the core configure/build pipeline:
  - `Configure.py` - `configure_build()`: checks requirements, updates submodules, resolves vcpkg, runs `cmake` to configure the `build/` directory, then builds unless `--skip-build`.
  - `BuildSystem.py` - chooses a generator (Ninja, Unix Makefiles, or a Visual Studio version) and runs `cmake --build`.
  - `Cache.py` - `ensure_compatible_build_cache()`: detects a stale/incompatible `build/CMakeCache.txt` (wrong platform, path, or vcpkg toolchain) and deletes it, prompting for confirmation unless `--auto-mode`/`--dry-run` is set.
  - `MSVC.py` - `ensure_msvc_environment()`: imports the MSVC environment variables via `VsDevCmd.bat` when building with Ninja/Make on Windows.
- `Actions/` - the top-level actions dispatched from `build.py`:
  - `Build.py` - `build_existing_target()`: builds an already-configured tree (used with `--skip-configure`).
  - `Cleanup.py` - `clean()` / `fclean()`: remove the `build/` directory, or build artifacts (binaries, coverage files, profiling data...) from the repo root.
  - `Tests.py` - `run_tests()` and `generate_coverage()`: run the `splinter3D_unit_tests` executable and produce a coverage report (`llvm-cov` on macOS, `gcovr` elsewhere).

## Usage

Not run directly by contributors; invoked through `scripts/build.sh` / `scripts/build.ps1`, for example:

```bash
scripts/build.sh --tests --coverage    # configure, build, and run unit tests with coverage
scripts/build.sh --clean               # remove the build directory
scripts/build.sh --ci                  # auto mode: no interactive prompts (used by CI)
scripts/build.sh --skip-configure      # rebuild an already-configured target without reconfiguring
```

Run `python3 scripts/Build/build.py --help` for the full, authoritative list of flags (also aliased as `-ci`/`--ci` for `--auto-mode`, `-d`/`--debug-build`, `-r`/`--rebuild`, `-c`/`--clean`, `-f`/`--fclean`...).

## Import convention

Modules import each other by package name (e.g. `from Platform import get_platform`), not by path relative to `scripts/`. This works because `build.sh`/`build.ps1` `cd` into the repository root and invoke `scripts/Build/build.py`, which makes Python add `scripts/Build/` (the script's own directory) to `sys.path`, putting every sibling subpackage on the import path.
