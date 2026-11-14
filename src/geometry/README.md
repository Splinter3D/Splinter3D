# geometry/

The domain layer: everything that handles the 3D data itself, regardless of how it is displayed or triggered.

## Contents

- `occt/` - the OpenCascade boundary for CAD shapes and import/export. Used to override OCCT types to avoid exposing it everywhere.
  - `shape.hpp` - `Shape`: wraps a `TopoDS_Shape`. Models are **meshes**: `fromSTL()` / `fromMesh()` give one surface-less face carrying a single-precision `Poly_Triangulation` (`mesh()`, `triangleCount()`), cheap to import, display and export. `toBRep()` builds the face-per-triangle B-Rep that booleans/analysis need (what `StlAPI_Reader` builds; slow and large on big meshes: Lucy, 28 M triangles, ~4.5 min and ~15 GB), cancellable through its progress range.
  - `stl_reader.hpp` - `readBinarySTL()`: memory-maps a single-block binary STL (`isBinarySTL()`) and reads it in parallel through an injected `ParallelFor` (geometry depends on no other layer; the import task passes the Dispatcher's). Output equals `RWStl::ReadFile`'s node for node (same-bits vertices merged, first-appearance order, degenerate triangles dropped). Lucy: 0.57 s and 1.2 GB peak vs RWStl's 3.4 s and 2.3 GB. Other files (ASCII, several solids) still go through `RWStl`.
  - `transform.hpp` - `Transform` (move in mm, rotation in degrees around each world axis, applied X then Y then Z around the model origin, then moved), `toTrsf()` to build the OCCT transformation, and the gizmo drag maths (`withTranslationDelta()`, `withRotationDelta()`).
  - `windows_prelude.hpp` - on Windows, includes `<windows.h>` (with `WIN32_LEAN_AND_MEAN`, so no Winsock 1 header clashes with wxWidgets' `<winsock2.h>`) before any OCCT header. Otherwise OCCT's `Standard_Macro.hxx` includes it with `NOGDI`/`NOUSER`/... defined, which breaks every wxWidgets/OpenGL header included later in the same file (`PIXELFORMATDESCRIPTOR` missing in `wx/msw/glcanvas.h`). Any header here that includes OCCT must include it first.
  - `path.hpp` - `toUtf8()`: the UTF-8 file name OpenCascade expects on every platform (`path.string()` is the ANSI code page on Windows).

## Dependency rule

`geometry/` depends only on itself and potentially on `core/` (for shared interfaces like `IRenderable`). It must never include `ui/`, `rendering/`, or `application/`-otherwise, it would be impossible to test a mesh split without instantiating a renderer.
