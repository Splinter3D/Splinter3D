# app/

Application entry point and startup. Nothing else.

## Contents

- `main.cpp` - `main()` (`WinMain()` on Windows); the bare minimum to launch `app`. It first hands the command line to batch mode, then starts the GUI through `wxEntry()` exactly as `wxIMPLEMENT_APP` would (`wxIMPLEMENT_APP_NO_MAIN` + our own entry point).
- `cli/batch_mode.cpp` / `cli/batch_mode.hpp` - `runBatchIfRequested()`: handles `splinter3D --batch <script.json>` before wxWidgets is initialized, so it needs no display. Runs the script with `application::workflow::BatchRunner` (format: see `tests/README.md`, *Batch mode*) and returns the exit code (`0` ok, `1` a step failed, `2` usage/script error).
- `wx/wx_app.cpp` / `wx/wx_app.hpp` - `WxApp`, the only `wxApp`: splash screen, then `Bootstrap` + `App::init()`. Also polls the Ctrl+C / SIGTERM flag set by `core::utils::InstallSignalHandlers()` and quits cleanly when it is raised (signal handlers cannot touch wx themselves).
- `app.cpp` / `app.hpp` - Application class: handles the lifecycle (init, run, shutdown) and owns high-level instances (main window, etc.).
- `bootstrap.cpp` - Dependency injection: constructs and connects services from `platform/`, `core/`, and `application/` before `app` starts.

## Dependency rule

`app/` can depend on anything (it is the top-level layer), but nothing should depend on `app/`. If another folder needs to include a header from `app/`, it indicates that something is misplaced.
