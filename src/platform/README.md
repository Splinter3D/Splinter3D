# platform/

Everything that communicates directly with the OS or persists raw data. The lowest layer-it has no knowledge of the layers above it.

## Contents

- `config/` - technical settings (paths, feature flags, build parameters).
- `preference/` - persisted user settings (selected theme, selected language, etc.).
- `locale/` - loading/saving translations.
- `logging/` - writing logs.
- `benchmark/` - `measure()`: runs a call and fills a `Report` with the requested `Metrics` (wall and CPU time, cores used, resident memory change, longest UI frame), even if the call throws; `operator<<`/`formatReport()` print it. `FrameClock` is the UI heartbeat (ticked by `WxApp`) behind the longest frame, so a frozen UI shows up as a frame as long as the freeze. Used to log model import (`SceneService::load()`) and display (`ModelViewerPanel`) costs.
- `file_system/` - file reading/writing, paths.
- `clipboard/` - clipboard operations.
- `threading/` - `Dispatcher` (singleton): shares the CPU between expensive operations. `run(name, fn)` gives an operation a worker budget from one global `WorkerPool`, sized from its `ScalabilityProfile` (serial fraction estimated from past `measure()` reports, diminishing returns past +0.1x speedup per worker) and capped by what concurrent operations hold; code inside reads it with `NPROC` and splits loops with `parallelFor()`. Nested runs share the outer budget through a thread-local `ExecutionContext`. Runs shorter than `kMinLearnedDuration` (20 ms) are not learned from (overhead, not scaling); every 8th *recorded* run explores ±1 worker, so exploration carries over between sessions. Profiles persist in `$SPLINTER3D_PROFILES` if set (the end-to-end tests point it at a temporary file), else `<data root>/assets/splinter3D.profiles.json` (git-ignored; loaded by `Bootstrap` and batch mode, saved on exit, only once a run was recorded).
- `networking/` - network access if needed (updates, telemetry, etc.).

## Dependency Rule

`platform/` does not depend on anything else within `src/`. No headers from `ui`, `application`, `core`, `geometry`, or `rendering` should appear here.
