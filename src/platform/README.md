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
- `threading/` - thread/asynchronous task management.
- `networking/` - network access if needed (updates, telemetry, etc.).

## Dependency Rule

`platform/` does not depend on anything else within `src/`. No headers from `ui`, `application`, `core`, `geometry`, or `rendering` should appear here.
