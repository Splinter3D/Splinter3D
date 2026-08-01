# rendering/

Handles display, not the decision of what to display. Answers the question "how do we draw," never "why do we draw this."

## Contents

- `renderer/` — rendering engine (currently raylib: `RaylibRenderer`, `RenderObject`).
- `camera/` — position, projection, and camera controls.
- `scene/` — scene graph (`Scene`, `SceneObject`): what is present and where, not how it is manipulated by the user.
- `lights/` — lighting management.
- `occt/` — C++ library used for CAD via OpenGL (not to be confused with `ui/framework/occt/`, which deals with integrating OCCT widgets into the UI).

## Dependency Rule

`rendering/` depends on `geometry/` (to know what to draw) and `core/` (interfaces), but never on `ui/` or `application/`.
