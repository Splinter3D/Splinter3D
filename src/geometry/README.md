# geometry/

The domain layer: everything that handles the 3D data itself, regardless of how it is displayed or triggered.

## Contents

- `math/` — mathematical primitives (vectors, matrices, quaternions, etc.).
- `mesh/` — mesh data structure and its direct utilities.
- `algorithms/` — generic geometric algorithms.
  - `splitter/` — `MeshDissector`, `MeshSplitter`: the core logic of Splinter3D, isolated in its own subfolder rather than being buried in a general `algorithms/` folder.
- `boolean/` — Boolean operations (union, intersection, difference).
- `solver/` — geometric constraint solving.
- `topology/` — mesh topology structure and validity.
- `transform/` — translations, rotations, and scaling applied to objects.

## Dependency rule

`geometry/` depends only on itself and potentially on `core/` (for shared interfaces like `IRenderable`). It must never include `ui/`, `rendering/`, or `application/`—otherwise, it would be impossible to test a mesh split without instantiating a renderer.
