# application/

The layer that translates a user action into a decision, without ever drawing anything on the screen.

## Contents

- `commands/` - a triggerable action (e.g., "Split Mesh", "Export STL"). Encapsulates undo/redo functionality if needed.
- `controller/` - receives events from `ui/` and decides which command or service to call.
- `service/` - reusable application logic called by controllers or commands (e.g., `ExportService`).
- `workflow/` - sequences of multiple commands/services for a complete scenario (e.g., import → validation → display).

## Current implementations

- `service/scene_service.hpp` - `SceneService`: the models currently open (name, `geometry::occt::Shape`, `geometry::occt::Transform`), in load order. Loading, stable `ModelId`s (never reused; `reserveId()` for a model a background import will add), unique display names (`cube.stl (2)`), per-model or all-model (`kAllModels`) placement, clearing, and STL export (`exportSTL()`: every model at its transform, with progress and cancellation, through a temporary file renamed on success). The single source of truth that `ui::framework::occt::ModelViewerPanel` only mirrors on screen.
- `workflow/batch_runner.hpp` - `BatchRunner`: runs a JSON script of `open` / `transform` / `export` / `clear` steps against a `SceneService`, through the same tasks as the GUI (run inline). Used by `splinter3D --batch` and the end-to-end tests (format: `tests/README.md`, *Batch mode*).
- `tasks/` - background tasks, so the UI never waits on a model operation:
  - `task.hpp` - `Task`: a name, the models it reads and writes, and three steps: `prepare()` (UI thread: checks its models still exist, snapshots them), `work(TaskContext&)` (on the `platform::threading::Dispatcher` pool) and `apply()` (UI thread: commits). `TaskSnapshot` is what a card shows.
  - `task_context.hpp` - `TaskContext`: progress (or an indeterminate status) and cancellation for running work; `progressRange()` hands both to OCCT algorithms.
  - `task_manager.hpp` - `TaskManager`: FIFO per model (a task waits for earlier unfinished writes to its models; reads snapshot, so they never block later writes), "waiting for workers" when the Dispatcher is full, cancel/dismiss, progress at most every 50 ms, `cancelAllAndWait()` for shutdown. With no poster it runs everything inline (batch). A task without work on free models (move, remove) applies inside `submit()`, without a card.
  - `model_tasks.hpp` - `importTask` / `exportTask` / `moveTask` / `removeTask` / `clearTask`, and `convertTask` ("Convert <model>": builds and caches `Model::brep`). Operations that need real topology use `submitWithBRep()` (queues one conversion per unconverted model ahead of them) and `requireBRep()` in their `prepare()` ("could not be converted" after a cancelled conversion). They commit through a `SceneEditor` (`scene_editor.hpp`): the viewer implements it to keep its presentations in step; `SceneServiceEditor` changes the `SceneService` only.

All of these are unit-tested without any window (`tests/unit/application/`).

## Purpose of this layer

In the old structure, logic was scattered between `Gui/states/` and the panels themselves-the UI both made decisions *and* handled display. Here, `ui/` simply calls `application/`, which in turn calls `geometry/`, `rendering/`, and `platform/` via `core/`. This allows the logic to be tested without instantiating a single window.

## Dependency rule

`application/` may depend on `core/`, `geometry/`, `rendering/`, and `platform/`. It must never include a header from `ui/`.
