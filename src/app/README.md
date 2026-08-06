# app/

Application entry point and startup. Nothing else.

## Contents

- `main.cpp` - `main()` function; the bare minimum to launch `app`.
- `app.cpp` / `app.hpp` - Application class: handles the lifecycle (init, run, shutdown) and owns high-level instances (main window, etc.).
- `bootstrap.cpp` - Dependency injection: constructs and connects services from `platform/`, `core/`, and `application/` before `app` starts.

## Dependency rule

`app/` can depend on anything (it is the top-level layer), but nothing should depend on `app/`. If another folder needs to include a header from `app/`, it indicates that something is misplaced.
