# rendering/

Handles display, not the decision of what to display. Answers the question "how do we draw," never "why do we draw this."

## Contents

- `renderer/` - rendering engine and renderer-independent display values such as `Color` and `Palette`.
- `camera/` - position, projection, and camera controls.
- `scene/` - scene graph (`Scene`, `SceneObject`): what is present and where, not how it is manipulated by the user.
- `lights/` - lighting management.
- `occt/` - C++ library used for CAD via OpenGL (not to be confused with `ui/framework/occt/`, which deals with integrating OCCT widgets into the UI).

The first OpenCascade integration lives in `geometry/occt`, where CAD shapes
are owned and imported. Rendering can consume that boundary later through
`rendering/occt`; the renderer must not make STL parsing or CAD decisions.

## Dependency Rule

`rendering/` depends on `geometry/` (to know what to draw) and `core/` (interfaces), but never on `ui/` or `application/`.
