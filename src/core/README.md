# core/

The bridge between `ui/` and the rest of the system. It is the only layer known to both the UI and the logic.

## Contents

- `events/` - application-internal event definitions (distinct from wx events, which remain in `ui/framework/wx/events/`).
- `event_bus/` - publish/subscribe mechanism for propagating events between layers without direct dependencies.
- `interface/` - interfaces/contracts shared across layers (e.g., `ISelectable`, `IRenderable`).
- `models/` - data structures shared between UI and logic (e.g., selection state, current session). Do not include purely geometric data here (`Mesh` remains in `geometry/`).
- `state/` - global application state (current session, open document, etc.).
- `utils/` - generic utilities with no business-specific dependencies.

## Purpose of this folder

Without it, `ui/` would end up directly importing `geometry/` or `application/` (or vice versa), resulting in a loss of layered separation. `core/` acts as a neutral intermediary: `ui/` subscribes to its events/state, while `application/` publishes them.

## Concrete example

When the user changes the theme or language: `platform/theme/` (or `platform/locale/`) loads/saves the raw data to disk, but `core/event_bus/` notifies `ui/` so it can refresh in real-time. `platform/` never has a direct dependency on `ui/`.

## Dependency rule

`core/` depends on nothing but itself (and potentially `platform/` for low-level utilities). Neither `ui/`, `application/`, nor `geometry/` should have a reverse dependency on it.
