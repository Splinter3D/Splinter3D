# geometry/

The domain layer: everything that handles the 3D data itself, regardless of how it is displayed or triggered.

## Contents

- `occt/` - the OpenCascade boundary for CAD shapes and import/export. Used to override OCCT types to aovid exposing it everywhere

## Dependency rule

`geometry/` depends only on itself and potentially on `core/` (for shared interfaces like `IRenderable`). It must never include `ui/`, `rendering/`, or `application/`-otherwise, it would be impossible to test a mesh split without instantiating a renderer.

