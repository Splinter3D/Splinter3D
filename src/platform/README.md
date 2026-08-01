# platform/

Everything that communicates directly with the OS or persists raw data. The lowest layer—it has no knowledge of the layers above it.

## Contents

- `config/` — technical settings (paths, feature flags, build parameters).
- `preference/` — persisted user settings (selected theme, selected language, etc.).
- `locale/` — loading/saving translations.
- `logging/` — writing logs.
- `file_system/` — file reading/writing, paths.
- `clipboard/` — clipboard operations.
- `threading/` — thread/asynchronous task management.
- `networking/` — network access if needed (updates, telemetry, etc.).

## Dependency Rule

`platform/` does not depend on anything else within `src/`. No headers from `ui`, `application`, `core`, `geometry`, or `rendering` should appear here.
