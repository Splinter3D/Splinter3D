/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <Geometry/Utils/MeshUtils.hpp>
#include <Gui/CenterredToolBar.hpp>
#include <Objects3D/Object3D.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Renderer/RenderObject.hpp>

#define RAYGUI_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <Renderer/RayGUI.hpp>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// namespace
// {
//     struct MeshBounds
//     {
//         geometry::Vec3 min{};
//         geometry::Vec3 max{};
//         bool           valid{false};
//     };

//     MeshBounds computeMeshBounds(const objects3D::OMesh& mesh)
//     {
//         MeshBounds bounds;
//         for (const auto& triangle : mesh.triangles)
//         {
//             for (const auto& vertex : triangle.vertices)
//             {
//                 geometry::Vec3 current{vertex.x, vertex.y, vertex.z};
//                 if (!bounds.valid)
//                 {
//                     bounds.min   = current;
//                     bounds.max   = current;
//                     bounds.valid = true;
//                 }
//                 else
//                 {
//                     bounds.min.x = std::min(bounds.min.x, current.x);
//                     bounds.min.y = std::min(bounds.min.y, current.y);
//                     bounds.min.z = std::min(bounds.min.z, current.z);

//                     bounds.max.x = std::max(bounds.max.x, current.x);
//                     bounds.max.y = std::max(bounds.max.y, current.y);
//                     bounds.max.z = std::max(bounds.max.z, current.z);
//                 }
//             }
//         }
//         return bounds;
//     }

//     geometry::Vec3 computeBoundsCenter(const MeshBounds& bounds)
//     {
//         return geometry::Vec3{
//             (bounds.min.x + bounds.max.x) * 0.5f,
//             (bounds.min.y + bounds.max.y) * 0.5f,
//             (bounds.min.z + bounds.max.z) * 0.5f};
//     }

//     float computeBoundsRadius(const MeshBounds& bounds)
//     {
//         const float dx = bounds.max.x - bounds.min.x;
//         const float dy = bounds.max.y - bounds.min.y;
//         const float dz = bounds.max.z - bounds.min.z;
//         return 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
//     }

//     struct ResizePanelState
//     {
//         bool                                show{false};
//         bool                                uniformScale{true};
//         geometry::Vec3                      baseSize{1.0f, 1.0f, 1.0f};
//         geometry::Vec3                      scalePercent{100.0f, 100.0f, 100.0f};
//         geometry::Vec3                      sizeMM{1.0f, 1.0f, 1.0f};
//         geometry::Vec3                      center{};
//         std::array<std::array<char, 16>, 3> scaleFields{};
//         std::array<std::array<char, 16>, 3> sizeFields{};
//         std::array<bool, 3>                 scaleEditing{};
//         std::array<bool, 3>                 sizeEditing{};
//     };

//     float getAxisValue(const geometry::Vec3& vec, int axis)
//     {
//         switch (axis)
//         {
//             case 0:
//                 return vec.x;
//             case 1:
//                 return vec.y;
//             default:
//                 return vec.z;
//         }
//     }

//     void setAxisValue(geometry::Vec3& vec, int axis, float value)
//     {
//         switch (axis)
//         {
//             case 0:
//                 vec.x = value;
//                 break;
//             case 1:
//                 vec.y = value;
//                 break;
//             default:
//                 vec.z = value;
//                 break;
//         }
//     }

//     void updateSizeFromScale(ResizePanelState& resizeState)
//     {
//         for (int axis = 0; axis < 3; ++axis)
//         {
//             const float base    = getAxisValue(resizeState.baseSize, axis);
//             const float percent = getAxisValue(resizeState.scalePercent, axis);
//             float       size    = (base > 0.0f) ? base * (percent / 100.0f) : 0.0f;
//             setAxisValue(resizeState.sizeMM, axis, size);
//         }
//     }

//     float clampScalePercent(float percent)
//     {
//         const float minScale = 0.01f;
//         const float maxScale = 10000.0f;
//         return std::clamp(percent, minScale, maxScale);
//     }

//     void setScalePercent(ResizePanelState& resizeState, float percent, int axis)
//     {
//         percent = clampScalePercent(percent);
//         if (resizeState.uniformScale)
//         {
//             resizeState.scalePercent = geometry::Vec3{percent, percent, percent};
//         }
//         else
//         {
//             setAxisValue(resizeState.scalePercent, axis, percent);
//         }
//         updateSizeFromScale(resizeState);
//     }

//     void setSizeMM(ResizePanelState& resizeState, float sizeValue, int axis)
//     {
//         sizeValue        = std::max(sizeValue, 0.0f);
//         const float base = getAxisValue(resizeState.baseSize, axis);
//         if (base <= 0.0001f)
//             return;
//         const float percent = (sizeValue / base) * 100.0f;
//         setScalePercent(resizeState, percent, axis);
//     }

//     void refreshTextFields(ResizePanelState& resizeState)
//     {
//         for (int axis = 0; axis < 3; ++axis)
//         {
//             const std::size_t idx = static_cast<std::size_t>(axis);
//             if (!resizeState.scaleEditing[idx])
//             {
//                 std::snprintf(
//                     resizeState.scaleFields[idx].data(),
//                     resizeState.scaleFields[idx].size(),
//                     "%.2f",
//                     static_cast<double>(getAxisValue(resizeState.scalePercent, axis)));
//             }

//             if (!resizeState.sizeEditing[idx])
//             {
//                 std::snprintf(
//                     resizeState.sizeFields[idx].data(),
//                     resizeState.sizeFields[idx].size(),
//                     "%.2f",
//                     static_cast<double>(getAxisValue(resizeState.sizeMM, axis)));
//             }
//         }
//     }

//     bool parseFloatValue(const char* text, float& value)
//     {
//         if (text == nullptr || *text == '\0')
//             return false;
//         char* endPtr = nullptr;
//         value        = std::strtof(text, &endPtr);
//         if (endPtr == text)
//             return false;
//         if (!std::isfinite(value))
//             return false;
//         return true;
//     }

//     bool applyScaleInput(ResizePanelState& resizeState, int axis)
//     {
//         const std::size_t idx    = static_cast<std::size_t>(axis);
//         float             parsed = 0.0f;
//         if (!parseFloatValue(resizeState.scaleFields[idx].data(), parsed))
//             return false;
//         parsed              = clampScalePercent(parsed);
//         const float current = getAxisValue(resizeState.scalePercent, axis);
//         if (std::fabs(current - parsed) < 0.005f)
//             return false;
//         setScalePercent(resizeState, parsed, axis);
//         return true;
//     }

//     bool applySizeInput(ResizePanelState& resizeState, int axis)
//     {
//         const std::size_t idx    = static_cast<std::size_t>(axis);
//         float             parsed = 0.0f;
//         if (!parseFloatValue(resizeState.sizeFields[idx].data(), parsed))
//             return false;
//         const float current = getAxisValue(resizeState.sizeMM, axis);
//         if (std::fabs(current - parsed) < 0.005f)
//             return false;
//         setSizeMM(resizeState, parsed, axis);
//         return true;
//     }

//     ResizePanelState initResizePanelState(const MeshBounds& bounds)
//     {
//         ResizePanelState state;
//         state.baseSize = geometry::Vec3{
//             std::max(bounds.max.x - bounds.min.x, 0.0f),
//             std::max(bounds.max.y - bounds.min.y, 0.0f),
//             std::max(bounds.max.z - bounds.min.z, 0.0f)};
//         updateSizeFromScale(state);
//         state.center = computeBoundsCenter(bounds);
//         refreshTextFields(state);
//         return state;
//     }

//     void frameCameraOnMesh(renderer::RaylibRenderer& renderer, const MeshBounds& bounds)
//     {
//         if (!bounds.valid)
//             return;

//         const geometry::Vec3 center = computeBoundsCenter(bounds);
//         renderer.setCameraTarget(center);

//         const float radius       = std::max(computeBoundsRadius(bounds), 0.5f);
//         const float screenWidth  = static_cast<float>(GetScreenWidth());
//         const float screenHeight = static_cast<float>(GetScreenHeight());
//         const float aspect       = (screenHeight > 0.0f) ? (screenWidth / screenHeight) : 1.0f;

//         const float fovYRad     = renderer.getCameraFov() * DEG2RAD;
//         const float tanHalfFovY = std::max(std::tan(fovYRad * 0.5f), 0.001f);
//         float       distanceY   = radius / tanHalfFovY;

//         float distanceX = distanceY;
//         if (aspect > 0.0f)
//         {
//             const float fovXRad     = 2.0f * std::atan(std::tan(fovYRad * 0.5f) * aspect);
//             const float tanHalfFovX = std::max(std::tan(fovXRad * 0.5f), 0.001f);
//             distanceX               = radius / tanHalfFovX;
//         }

//         const float   distance  = std::max(distanceX, distanceY) * 1.2f;
//         const Vector3 direction = Vector3Normalize(Vector3{1.4f, 1.0f, 1.3f});

//         geometry::Vec3 position{
//             center.x + direction.x * distance,
//             center.y + direction.y * distance,
//             center.z + direction.z * distance};
//         renderer.setCameraPosition(position);
//     }

//     geometry::Vec3 scaleVertex(
//         const geometry::Vec3& vertex,
//         const geometry::Vec3& center,
//         const geometry::Vec3& scaleFactors)
//     {
//         geometry::Vec3 relative{
//             vertex.x - center.x,
//             vertex.y - center.y,
//             vertex.z - center.z};

//         return geometry::Vec3{
//             center.x + relative.x * scaleFactors.x,
//             center.y + relative.y * scaleFactors.y,
//             center.z + relative.z * scaleFactors.z};
//     }

//     void applyScaleToMesh(
//         geometry::Mesh&       destination,
//         const geometry::Mesh& source,
//         const geometry::Vec3& center,
//         const geometry::Vec3& scalePercent)
//     {
//         destination = source;
//         geometry::Vec3 factors{
//             scalePercent.x / 100.0f,
//             scalePercent.y / 100.0f,
//             scalePercent.z / 100.0f};

//         for (auto& tri : destination.triangles)
//         {
//             for (int i = 0; i < 3; ++i)
//                 tri.vertices[i] = scaleVertex(tri.vertices[i], center, factors);
//         }
//     }

//     bool drawResizePanel(ResizePanelState& resizeState)
//     {
//         if (!resizeState.show)
//             return false;

//         refreshTextFields(resizeState);

//         const float panelWidth  = 460.0f;
//         const float panelHeight = 210.0f;
//         Rectangle   panelBounds{
//             (static_cast<float>(GetScreenWidth()) - panelWidth) * 0.5f,
//             88.0f,
//             panelWidth,
//             panelHeight};

//         GuiPanel(panelBounds, nullptr);
//         GuiLabel({panelBounds.x + 20.0f, panelBounds.y + 10.0f, 220.0f, 20.0f}, "Resize / Scale");

//         Rectangle checkboxBounds{panelBounds.x + 20.0f, panelBounds.y + 36.0f, 20.0f, 20.0f};
//         bool      checkboxChanged = GuiCheckBox(checkboxBounds, "Uniform scale", &resizeState.uniformScale);
//         bool      valuesChanged   = false;
//         if (checkboxChanged && resizeState.uniformScale)
//         {
//             setScalePercent(resizeState, getAxisValue(resizeState.scalePercent, 0), 0);
//             valuesChanged = true;
//         }

//         const float labelWidth  = 90.0f;
//         const float margin      = 20.0f;
//         const float usableWidth = panelWidth - margin * 2.0f - labelWidth;
//         const float columnWidth = usableWidth / 3.0f;
//         const float headerY     = panelBounds.y + 74.0f;
//         const float scaleRowY   = headerY + 20.0f;
//         const float sizeRowY    = scaleRowY + 52.0f;

//         GuiLabel({panelBounds.x + margin, scaleRowY + 4.0f, labelWidth, 24.0f}, "Scale (%)");
//         GuiLabel({panelBounds.x + margin, sizeRowY + 4.0f, labelWidth, 24.0f}, "Size (mm)");

//         static const char* axisLabels[3] = {"X", "Y", "Z"};
//         static const Color axisColors[3] = {RED, GREEN, BLUE};

//         for (int axis = 0; axis < 3; ++axis)
//         {
//             const std::size_t idx     = static_cast<std::size_t>(axis);
//             float             columnX = panelBounds.x + margin + labelWidth + static_cast<float>(axis) * columnWidth;

//             int   textWidth  = MeasureText(axisLabels[axis], 16);
//             float textWidthF = static_cast<float>(textWidth);
//             float textX      = columnX + (columnWidth - textWidthF) * 0.5f;
//             DrawText(
//                 axisLabels[axis],
//                 static_cast<int>(textX),
//                 static_cast<int>(headerY - 12.0f),
//                 16,
//                 axisColors[axis]);

//             Rectangle scaleBox{columnX, scaleRowY, columnWidth - 12.0f, 28.0f};
//             if (GuiTextBox(
//                     scaleBox,
//                     resizeState.scaleFields[idx].data(),
//                     static_cast<int>(resizeState.scaleFields[idx].size()),
//                     resizeState.scaleEditing[idx]))
//                 resizeState.scaleEditing[idx] = !resizeState.scaleEditing[idx];

//             if (!resizeState.scaleEditing[idx])
//                 valuesChanged |= applyScaleInput(resizeState, axis);

//             Rectangle sizeBox{columnX, sizeRowY, columnWidth - 12.0f, 28.0f};
//             if (GuiTextBox(
//                     sizeBox,
//                     resizeState.sizeFields[idx].data(),
//                     static_cast<int>(resizeState.sizeFields[idx].size()),
//                     resizeState.sizeEditing[idx]))
//                 resizeState.sizeEditing[idx] = !resizeState.sizeEditing[idx];

//             if (!resizeState.sizeEditing[idx])
//                 valuesChanged |= applySizeInput(resizeState, axis);
//         }

//         return valuesChanged;
//     }

//     struct WindowState
//     {
//         int windowedWidth;
//         int windowedHeight;
//     };

//     void rememberWindowedSize(WindowState& state)
//     {
//         state.windowedWidth  = GetScreenWidth();
//         state.windowedHeight = GetScreenHeight();
//     }

//     void toggleFullscreen(WindowState& state)
//     {
//         if (IsWindowFullscreen())
//         {
//             ToggleFullscreen();
//             SetWindowSize(state.windowedWidth, state.windowedHeight);
//         }
//         else
//         {
//             rememberWindowedSize(state);
//             ToggleFullscreen();
//         }
//     }
// } // namespace

int main()
{
    renderer::Config         cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    objects3D::Object3D    obj = objects3D::Object3D::fromSTL("assets/stl/binary/cube.stl");
    renderer::RenderObject rObj;
    rObj.bind(obj);

    auto meshBounds = geometry::meshutils::computeMeshBounds(*obj.mesh);
    // geometry::Vec3 meshCenter = geometry::meshutils::computeBoundsCenter(meshBounds);

    // Frame camera on mesh
    geometry::meshutils::frameCameraOnMesh(renderer, meshBounds);

    gui::CenteredToolbar toolbar(18.0f, 52.0f, 14.0f);

    while (!renderer.shouldClose())
    {
        renderer.beginFrame();

        // Start 3D mode
        renderer.begin3D();

        if (obj.transform.scale.x < 2.0f)
            obj.transform.scale = obj.transform.scale + geometry::Vec3(0.01f, 0.01f, 0.01f);
        else
            obj.transform.scale = geometry::Vec3(1.0f, 1.0f, 1.0f);
        obj.notifyTransform();

        renderer.drawGrid(10, 1.0f);
        renderer.drawAxis(2.0f);
        renderer.drawObject(rObj, {255, 0, 0, 255});

        renderer.end3D();
        toolbar.draw();
        // if (drawResizePanel(resizePanel))
        //     applyScaleToMesh(*obj.mesh, *obj.mesh, resizePanel.center, resizePanel.scalePercent);
        renderer.endFrame();

        // if (IsKeyPressed(KEY_F11))
        //     toggleFullscreen(windowState);

        // if (IsWindowResized() && !IsWindowFullscreen())
        //     rememberWindowedSize(windowState);
    }
    return 0;
}
