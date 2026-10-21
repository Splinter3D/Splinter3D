# tests/

Automated tests for Splinter3D, in three suites that mirror the layers of `src/` (see the root `README.md`):

| Suite | Folder | What it exercises | Needs a display | Framework |
|---|---|---|---|---|
| **unit** | `unit/` | one class or function at a time, no window, no event loop | no | Catch2 (C++) |
| **integration** | `integration/` | several layers together inside a real wxWidgets app: widgets, events, the OCCT 3D viewer, the whole `MainWindow` | yes | Catch2 (C++) |
| **end-to-end** | `end_to_end/` | the real `splinter3D` executable, from the outside | only the GUI smoke tests | Python `unittest` (standard library only) |

## Running the tests

```bash
scripts/build.sh --tests               # configure (Debug), build every suite + the app, run unit -> integration -> end-to-end
scripts/build.sh --tests --coverage    # same, instrumented, then write a coverage report
```

`--tests` builds the `splinter3D_tests` CMake target (every suite plus the app the end-to-end tests run) and then `scripts/Build/Actions/Tests.py` runs the suites in order. On a Linux machine without `DISPLAY` (CI, Docker), the integration and GUI end-to-end tests are wrapped in `xvfb-run` automatically, with Mesa's software OpenGL (`LIBGL_ALWAYS_SOFTWARE=1`) for the 3D viewer.

Everything the runs produce lands in **`build/test-output/`** (git-ignored, removed by `--clean`/`--fclean`): the suites run from there, and it holds the raw coverage profiles and the report, `code_coverage.txt`.

Once built, each suite can also be run on its own:

```bash
cd build && ctest -L unit           # or -L integration, -L e2e; one CTest test per Catch2 test case
build/bin/splinter3D_unit_tests "[scene]"                   # Catch2 tag filter
build/bin/splinter3D_integration_tests --rng-seed 1234      # replay a run (Catch2 orders tests randomly by default)
SPLINTER3D_BIN=build/bin/splinter3D python3 -m unittest discover -s tests/end_to_end -v
```

`ctest -L integration` also runs `scripts/check-locales.py` (registered as `integration.check-locales`).

## How the build is wired

- **`splinter3D_lib`** - everything in `src/` except `app/main.cpp` is a static library; the app and every test executable link it (`cmake/5-Sources.cmake`, `cmake/7-Target.cmake`).
- **`cmake/8-Tests.cmake`** (only when `ENABLE_TESTS=ON`) - one executable per suite folder (`splinter3D_unit_tests`, `splinter3D_integration_tests`), built from every `.cpp` under it plus `tests/support/*.cpp`; Catch2 tests are registered with CTest at test time (`DISCOVERY_MODE PRE_TEST`), so listing them never needs a display. A suite folder without `.cpp` files is skipped.
- **Catch2** comes from the `tests` feature of `vcpkg.json`, installed only for test builds.
- **`SPLINTER3D_TESTING`** is defined on `splinter3D_lib` (PUBLIC) in test builds only. Test-only APIs are wrapped in `#if defined(SPLINTER3D_TESTING)` - currently `EventBus::clear()` and `EventBus::subscriberCount<Event>()`.
- **Compile definitions** available to tests: `SPLINTER3D_TEST_ASSETS_DIR` (the repo's `assets/`) and `SPLINTER3D_TEST_LOCALE_DIR` (the repo's `locale/`).
- **Coverage** (`ENABLE_COVERAGE=ON`, via `--coverage`) - Clang source-based coverage (`-fprofile-instr-generate -fcoverage-mapping`) or GCC `--coverage`, see `apply_coverage()` in `cmake/3-Warnings.cmake`.
- `scripts/Build/ConfigureBuild/Configure.py` always passes `ENABLE_TESTS`, `ENABLE_COVERAGE`, `ENABLE_DEBUG` and `VCPKG_MANIFEST_FEATURES` explicitly (ON or OFF): CMake caches options, so a release build run after a test or debug build in the same `build/` would otherwise silently keep them.

## Layout

```
tests/
├── support/                       shared by every C++ suite (include as "support/...")
│   ├── temp_dir.hpp               TempDir (self-deleting temp folder), TempCwd (also the cwd while alive)
│   ├── reset_globals_listener.cpp Catch2 listener: clears the EventBus before/after every test case
│   └── wx_helpers.hpp             pumpEvents(), pumpUntil(), findDescendant<T>(), ScopedFrame<T>, sendMenuCommand()
├── unit/
│   ├── core/          EventBus, findDataRoot, Singleton/NonCopyable, signal flag
│   ├── platform/      PreferenceManager, LocaleManager, Logger
│   ├── geometry/      Shape::fromSTL, Transform maths (toTrsf, gizmo drag deltas)
│   ├── rendering/     Color, Palette
│   └── application/   SceneService, BatchRunner
├── integration/
│   ├── support/wx_environment.cpp  Catch2 listener: starts wxWidgets once per run (see below)
│   ├── platform/      the real locale/*.json files through LocaleManager
│   └── ui/            EventManager, TransformToolbar, ModelViewerPanel (real OCCT viewer),
│                      MainWindow wiring, layouts + registries, theme, tr(), window lifecycle
└── end_to_end/
    ├── splinter3d_e2e.py   locating/running the binary, STL reader, BatchTestCase base class
    ├── test_batch_mode.py  `splinter3D --batch` scenarios, exit codes, exported geometry
    └── test_gui_smoke.py   the GUI starts, stays up, and quits cleanly on SIGTERM / Ctrl+C
```

## Writing tests

### Unit tests

- Name the file `<thing>_test.cpp` in the folder matching `src/`; it is picked up automatically (re-run CMake configure, which `--tests` does).
- Tag test cases with their layer and subject, e.g. `"[platform][preference]"`.
- Never depend on the current directory or on files outside the repo: use `tests::support::TempDir` / `TempCwd`, and `SPLINTER3D_TEST_ASSETS_DIR` for fixtures (`assets/stl/ascii/cube.stl` is a unit cube spanning 0..1; `assets/stl/binary/cube.stl` is centered on the origin).
- Process-wide singletons: the `EventBus` is reset around every test case by `reset_globals_listener.cpp`. `LocaleManager` has per-instance state - construct your own instead of using `LocaleManager::get()`. Restore anything else global you touch (e.g. `ThemeManager`), since tests run in random order.

### Integration tests

`integration/support/wx_environment.cpp` starts wxWidgets once for the whole run (`wxEntryStart`, no main loop) and routes wx log messages to stderr - the default GUI log target would open a modal dialog and hang the run. Consequently:

- Drive widgets by sending them events (`HandleWindowEvent`, `ProcessWindowEvent`, `sendMenuCommand`), then `pumpEvents()` if anything was deferred.
- Own top-level windows with `ScopedFrame<T>`, which destroys them at scope exit. A window that closes itself (e.g. File > Exit) must not be in a `ScopedFrame`; track it with `wxWeakRef` and `pumpUntil([&] { return !ref; })`.
- Call `ScopedFrame::showAndWait()` before anything needs OpenGL (the OCCT viewer is created on first use, and needs a realized window).
- Anything that opens a modal dialog (`ModelViewerPanel::openFile()`/`saveFile()`, Help > About, `ui::dialogs::*` except `progress()`) cannot be driven; test the code behind it instead (e.g. `loadFile(path)`/`exportFile(path)`).
- Code that saves preferences (theme and language menus) writes `./splinter3D.cfg`: run it inside a `TempCwd`.

### End-to-end tests

- Subclass `BatchTestCase` (scratch folder in `self.tmp`, `self.run_batch(steps)`) or use `run_app(*args)` directly.
- The binary under test is `$SPLINTER3D_BIN` if set, else `build/bin/splinter3D[.exe]`. CI points it at the binary inside the release package (see below).
- Check what a user would observe: exit code, stdout/stderr, and output files (`read_stl()` + `bounding_box()` parse ASCII and binary STL).

## Batch mode

The end-to-end tests drive the application through its non-interactive mode, which needs no display:

```bash
splinter3D --batch script.json
```

```json
{
  "steps": [
    { "open": "models/cube.stl" },
    { "transform": { "target": 0, "move": [10, 0, 0], "rotate": [0, 0, 90] } },
    { "export": "out/scene.stl" },
    { "clear": true }
  ]
}
```

- `open` imports an STL file (File > Open); `transform` sets a model's absolute placement like the toolbar fields (`target`: model index or `"all"`, the default; `move` in mm, `rotate` in degrees, omitted vectors are `[0, 0, 0]`); `export` writes every model, each at its transform, to one STL file (File > Export); `clear` removes every model (File > New).
- Relative paths are resolved against the script's folder; paths are UTF-8.
- Steps run in order and stop at the first failure. One line per step goes to stdout, errors to stderr.
- Exit codes: `0` success, `1` a step failed, `2` bad command line or unreadable/invalid script.

Implemented by `application::workflow::BatchRunner` (`src/application/workflow/`) and `app/cli/batch_mode.cpp`, which `main()` calls before wxWidgets starts.

## Coverage

`scripts/build.sh --tests --coverage` writes `build/test-output/code_coverage.txt`, merging every suite *and* the app binary run by the end-to-end tests (on macOS: `llvm-profdata merge` + `llvm-cov report`; elsewhere: `gcovr`). Raw profiles from a previous run are deleted first. Test code itself (`tests/`) is excluded.

Things to know when reading the report:

- **Only code linked into a test binary or the app is listed.** `splinter3D_lib` is static, so files nothing uses (currently the `ui/widgets/*` builders and the demo panels under `ui/panels/`) never appear and are not counted.
- **`warning: N functions have mismatched data`** is expected: small `inline` header functions are compiled separately into each binary, and a binary that never calls one records a placeholder whose fingerprint does not match the profile. `llvm-cov` skips that copy only; the function's coverage still comes from the binaries that call it.
- The app writes its profile when it exits normally - which is why the GUI smoke tests stop it with SIGTERM/SIGINT (handled gracefully, see `WxApp::onSignalPoll()`) rather than killing it.
- If your shell exports `LLVM_PROFILE_FILE`, any instrumented binary you run by hand writes `.profraw` files into the current directory. `Tests.py` and the CTest e2e entry override it to point at `build/test-output/`.

Known gaps: modal dialogs (`ui/dialogs/dialogs.hpp`), fullscreen toggling, the Windows branch of `core/utils/os_compatibility.hpp`, and mouse-driven gizmo drags in the 3D viewer.

## CI

`compile-on-pr.yml` runs, for each of its six OS/arch entries: `build.sh --ci --tests` (all suites), then the release build and packaging, then `.github/workflows/scripts/e2e-package.sh` (`.tar.xz`/`.dmg`) or `e2e-package.ps1` (`.zip`), which unpacks the package and runs the end-to-end suite against the binary inside it - catching missing assets, libraries or a wrong layout. Linux runners install `xvfb`, `xauth` and `libgl1-mesa-dri` for the GUI tests (`.github/workflows/scripts/Linux/install.sh`).

GitHub's Windows runners have no GPU; if the OCCT viewer cannot get a recent enough OpenGL there, exclude the viewer-dependent integration tests on that runner with the Catch2 filter `~[viewer]~[main_window]`.

## Bugs found and fixed while adding these tests

Kept here as context for the corresponding regression tests:

| Bug | Fix | Covered by |
|---|---|---|
| A corrupt `splinter3D.cfg` value (`fontSize big`, out-of-range number) threw out of `PreferenceManager::load()`, crashing the app on the next theme/language switch | parse with `std::from_chars`, keep the default on error | `unit/platform/preference_manager_test.cpp` |
| Preference values containing spaces were truncated and the rest of the line misread as the next key | parse one `key value` pair per line (also tolerates `\r\n` and blank lines) | same |
| `LocaleManager`'s path and translations were static, shared by every instance and impossible to reset | per-instance state, public constructor; `get()` remains the application-wide instance | `unit/platform/locale_manager_test.cpp` |
| STL export only worked because displaying a model meshed it as a side effect | `SceneService::saveSTL()` meshes before writing | `unit/application/scene_service_test.cpp` |
| Export wrote only the first model, without its move/rotation | export every model, each at its transform, in one compound | same, and `end_to_end/test_batch_mode.py` (`ExportPlacement`) |
| Paths were converted with `path.string()` / `ToStdString()`, which is the ANSI code page on Windows, breaking non-ASCII file names | UTF-8 conversions (`geometry/occt/path.hpp`, `ModelViewerPanel`, batch mode) | `end_to_end/test_batch_mode.py` (`test_non_ascii_paths`) |
| Ctrl+C / SIGTERM were caught but never acted on: the GUI could only be stopped with SIGKILL | `WxApp` polls the signal flag and quits cleanly (exit code 0) | `end_to_end/test_gui_smoke.py` |
| Test/debug CMake options stuck in `build/` across builds (a release build after `--tests` or `-d` kept them) | `Configure.py` always passes them explicitly | manual check: `build/CMakeCache.txt` after switching builds |
