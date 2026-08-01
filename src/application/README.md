# application/

The layer that translates a user action into a decision, without ever drawing anything on the screen.

## Contents

- `commands/` — a triggerable action (e.g., "Split Mesh", "Export STL"). Encapsulates undo/redo functionality if needed.
- `controller/` — receives events from `ui/` and decides which command or service to call.
- `service/` — reusable application logic called by controllers or commands (e.g., `ExportService`).
- `workflow/` — sequences of multiple commands/services for a complete scenario (e.g., import → validation → display).

## Purpose of this layer

In the old structure, logic was scattered between `Gui/states/` and the panels themselves—the UI both made decisions *and* handled display. Here, `ui/` simply calls `application/`, which in turn calls `geometry/`, `rendering/`, and `platform/` via `core/`. This allows the logic to be tested without instantiating a single window.

## Dependency rule

`application/` may depend on `core/`, `geometry/`, `rendering/`, and `platform/`. It must never include a header from `ui/`.
