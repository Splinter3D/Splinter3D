# UI/

Everything related to visual presentation. Based on wxWidgets.

## Content

- `windows/` — full windows (`MainWindow`, `SettingsWindow`, `AboutWindow`). One window = one clear, self-contained UI flow.
- `panels/` — workspaces within a window (`ExplorerPanel`, `ViewerPanel`, `PropertyPanel`, `ConsolePanel`).
- `layouts/` — spatial organization of widgets (`MainLayout`, `ViewerLayout`...).
- `menus/` — drop-down menus used when a feature is too small to warrant its own window.
- `toolbars/` — button/icon bars (New, Open, Save, Undo...).
- `dialogs/` — transient pop-ups (`OpenDialog`, `SaveDialog`, `ExportDialog`, `SettingsDialog`).
- `widgets/` — reusable basic components (`button`, `combo_box`, `slider`...), containing no business logic.
- `framework/`
  - `wx/` — everything wrapping wxWidgets (`Events/`, `Helpers/`, `IDs/`, `Builders/`).
  - `occt/` — widgets for integrating OCCT into the UI (not to be confused with `rendering/occt/`, which is the rendering engine).

## Dependency Rule

`ui/` never directly references `geometry/` or `rendering/`. To trigger a business action, it goes through `application/` (e.g., `application/commands/`). To update itself (state, selection, notifications), it observes `core/`.

## Pending Decision

`ui/framework/occt/` and `rendering/occt/` share the same name despite having different roles—if you keep both, rename one of them (e.g., `occt_widgets/`) to avoid confusion when reviewing the code later.
