# geometry/

The domain layer: everything that handles the 3D data itself, regardless of how it is displayed or triggered.

## Contents

- `occt/` - the OpenCascade boundary for CAD shapes and import/export. Used to override OCCT types to avoid exposing it everywhere.
  - `shape.hpp` - `Shape`: wraps a `TopoDS_Shape`; `Shape::fromSTL()` imports a file.
  - `transform.hpp` - `Transform` (move in mm, rotation in degrees around each world axis, applied X then Y then Z around the model origin, then moved), `toTrsf()` to build the OCCT transformation, and the gizmo drag maths (`withTranslationDelta()`, `withRotationDelta()`).
  - `windows_prelude.hpp` - on Windows, includes `<windows.h>` (with `WIN32_LEAN_AND_MEAN`, so no Winsock 1 header clashes with wxWidgets' `<winsock2.h>`) before any OCCT header. Otherwise OCCT's `Standard_Macro.hxx` includes it with `NOGDI`/`NOUSER`/... defined, which breaks every wxWidgets/OpenGL header included later in the same file (`PIXELFORMATDESCRIPTOR` missing in `wx/msw/glcanvas.h`). Any header here that includes OCCT must include it first.
  - `path.hpp` - `toUtf8()`: the UTF-8 file name OpenCascade expects on every platform (`path.string()` is the ANSI code page on Windows).

## Dependency rule

`geometry/` depends only on itself and potentially on `core/` (for shared interfaces like `IRenderable`). It must never include `ui/`, `rendering/`, or `application/`-otherwise, it would be impossible to test a mesh split without instantiating a renderer.
