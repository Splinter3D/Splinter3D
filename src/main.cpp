/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** main
*/

#include <Objects3D/OMesh.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>
#include <raymath.h>

#define RAYGUI_IMPLEMENTATION
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#    pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#    pragma GCC diagnostic ignored "-Wshadow"
#    pragma GCC diagnostic ignored "-Wmissing-declarations"
#    pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#include <Renderer/RayGUI.hpp>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

struct ToolbarButton
{
    std::string           id;
    Texture2D             icon{};
    bool                  hasIcon{false};
    std::function<void()> onClick;
};

class CenteredToolbar
{
  public:
    CenteredToolbar(float topMargin = 16.0f, float buttonSize = 48.0f, float spacing = 12.0f)
        : topMargin_(topMargin)
        , buttonSize_(buttonSize)
        , spacing_(spacing)
    {
    }

    void draw(const std::vector<ToolbarButton>& buttons) const
    {
        if (buttons.empty())
            return;

        const size_t buttonCount = buttons.size();
        float        totalWidth  = static_cast<float>(buttonCount) * buttonSize_;
        if (buttonCount > 1)
            totalWidth += static_cast<float>(buttonCount - 1) * spacing_;

        const float startX = (static_cast<float>(GetScreenWidth()) - totalWidth) * 0.5f;
        const float y      = topMargin_;

        for (size_t i = 0; i < buttonCount; ++i)
        {
            Rectangle bounds{
                startX + static_cast<float>(i) * (buttonSize_ + spacing_),
                y,
                buttonSize_,
                buttonSize_};

            const auto& button = buttons[i];

            if (GuiButton(bounds, "") && button.onClick)
                button.onClick();

            if (button.hasIcon)
                drawIcon(bounds, button.icon);
        }
    }

  private:
    float topMargin_{16.0f};
    float buttonSize_{48.0f};
    float spacing_{12.0f};

    static void drawIcon(const Rectangle& bounds, const Texture2D& icon)
    {
        const float iconSize = bounds.width * 0.55f;
        const Rectangle src{
            0.0f,
            0.0f,
            static_cast<float>(icon.width),
            static_cast<float>(icon.height)};
        const Rectangle dest{
            bounds.x + (bounds.width - iconSize) * 0.5f,
            bounds.y + (bounds.height - iconSize) * 0.5f,
            iconSize,
            iconSize};

        DrawTexturePro(icon, src, dest, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
    }
};

namespace
{
    struct MeshBounds
    {
        renderer::RVec3 min{};
        renderer::RVec3 max{};
        bool            valid{false};
    };

    MeshBounds computeMeshBounds(const objects3D::OMesh& mesh)
    {
        MeshBounds bounds;
        for (const auto& triangle : mesh.triangles)
        {
            for (const auto& vertex : triangle.vertices)
            {
                renderer::RVec3 current{vertex.x, vertex.y, vertex.z};
                if (!bounds.valid)
                {
                    bounds.min  = current;
                    bounds.max  = current;
                    bounds.valid = true;
                }
                else
                {
                    bounds.min.x = std::min(bounds.min.x, current.x);
                    bounds.min.y = std::min(bounds.min.y, current.y);
                    bounds.min.z = std::min(bounds.min.z, current.z);

                    bounds.max.x = std::max(bounds.max.x, current.x);
                    bounds.max.y = std::max(bounds.max.y, current.y);
                    bounds.max.z = std::max(bounds.max.z, current.z);
                }
            }
        }
        return bounds;
    }

    renderer::RVec3 computeBoundsCenter(const MeshBounds& bounds)
    {
        return renderer::RVec3{
            (bounds.min.x + bounds.max.x) * 0.5f,
            (bounds.min.y + bounds.max.y) * 0.5f,
            (bounds.min.z + bounds.max.z) * 0.5f};
    }

    float computeBoundsRadius(const MeshBounds& bounds)
    {
        const float dx = bounds.max.x - bounds.min.x;
        const float dy = bounds.max.y - bounds.min.y;
        const float dz = bounds.max.z - bounds.min.z;
        return 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    struct ResizePanelState
    {
        bool            show{false};
        bool            uniformScale{true};
        renderer::RVec3 baseSize{1.0f, 1.0f, 1.0f};
        renderer::RVec3 scalePercent{100.0f, 100.0f, 100.0f};
        renderer::RVec3 sizeMM{1.0f, 1.0f, 1.0f};
        renderer::RVec3 center{};
        std::array<std::array<char, 16>, 3> scaleFields{};
        std::array<std::array<char, 16>, 3> sizeFields{};
        std::array<bool, 3>                 scaleEditing{};
        std::array<bool, 3>                 sizeEditing{};
    };

    float getAxisValue(const renderer::RVec3& vec, int axis)
    {
        switch (axis)
        {
            case 0:
                return vec.x;
            case 1:
                return vec.y;
            default:
                return vec.z;
        }
    }

    void setAxisValue(renderer::RVec3& vec, int axis, float value)
    {
        switch (axis)
        {
            case 0:
                vec.x = value;
                break;
            case 1:
                vec.y = value;
                break;
            default:
                vec.z = value;
                break;
        }
    }

    void updateSizeFromScale(ResizePanelState& resizeState)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            const float base    = getAxisValue(resizeState.baseSize, axis);
            const float percent = getAxisValue(resizeState.scalePercent, axis);
            float       size    = (base > 0.0f) ? base * (percent / 100.0f) : 0.0f;
            setAxisValue(resizeState.sizeMM, axis, size);
        }
    }

    float clampScalePercent(float percent)
    {
        const float minScale = 0.01f;
        const float maxScale = 10000.0f;
        return std::clamp(percent, minScale, maxScale);
    }

    void setScalePercent(ResizePanelState& resizeState, float percent, int axis)
    {
        percent = clampScalePercent(percent);
        if (resizeState.uniformScale)
        {
            resizeState.scalePercent = renderer::RVec3{percent, percent, percent};
        }
        else
        {
            setAxisValue(resizeState.scalePercent, axis, percent);
        }
        updateSizeFromScale(resizeState);
    }

    void setSizeMM(ResizePanelState& resizeState, float sizeValue, int axis)
    {
        sizeValue = std::max(sizeValue, 0.0f);
        const float base = getAxisValue(resizeState.baseSize, axis);
        if (base <= 0.0001f)
            return;
        const float percent = (sizeValue / base) * 100.0f;
        setScalePercent(resizeState, percent, axis);
    }

    void refreshTextFields(ResizePanelState& resizeState)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            const std::size_t idx = static_cast<std::size_t>(axis);
            if (!resizeState.scaleEditing[idx])
            {
                std::snprintf(
                    resizeState.scaleFields[idx].data(),
                    resizeState.scaleFields[idx].size(),
                    "%.2f",
                    static_cast<double>(getAxisValue(resizeState.scalePercent, axis)));
            }

            if (!resizeState.sizeEditing[idx])
            {
                std::snprintf(
                    resizeState.sizeFields[idx].data(),
                    resizeState.sizeFields[idx].size(),
                    "%.2f",
                    static_cast<double>(getAxisValue(resizeState.sizeMM, axis)));
            }
        }
    }

    bool parseFloatValue(const char* text, float& value)
    {
        if (text == nullptr || *text == '\0')
            return false;
        char* endPtr = nullptr;
        value        = std::strtof(text, &endPtr);
        if (endPtr == text)
            return false;
        if (!std::isfinite(value))
            return false;
        return true;
    }

    bool applyScaleInput(ResizePanelState& resizeState, int axis)
    {
        const std::size_t idx = static_cast<std::size_t>(axis);
        float parsed = 0.0f;
        if (!parseFloatValue(resizeState.scaleFields[idx].data(), parsed))
            return false;
        parsed = clampScalePercent(parsed);
        const float current = getAxisValue(resizeState.scalePercent, axis);
        if (std::fabs(current - parsed) < 0.005f)
            return false;
        setScalePercent(resizeState, parsed, axis);
        return true;
    }

    bool applySizeInput(ResizePanelState& resizeState, int axis)
    {
        const std::size_t idx = static_cast<std::size_t>(axis);
        float parsed = 0.0f;
        if (!parseFloatValue(resizeState.sizeFields[idx].data(), parsed))
            return false;
        const float current = getAxisValue(resizeState.sizeMM, axis);
        if (std::fabs(current - parsed) < 0.005f)
            return false;
        setSizeMM(resizeState, parsed, axis);
        return true;
    }

    ResizePanelState initResizePanelState(const MeshBounds& bounds)
    {
        ResizePanelState state;
        state.baseSize = renderer::RVec3{
            std::max(bounds.max.x - bounds.min.x, 0.0f),
            std::max(bounds.max.y - bounds.min.y, 0.0f),
            std::max(bounds.max.z - bounds.min.z, 0.0f)};
        updateSizeFromScale(state);
        state.center = computeBoundsCenter(bounds);
        refreshTextFields(state);
        return state;
    }

    void frameCameraOnMesh(renderer::RaylibRenderer& renderer, const MeshBounds& bounds)
    {
        if (!bounds.valid)
            return;

        const renderer::RVec3 center = computeBoundsCenter(bounds);
        renderer.setCameraTarget(center);

        const float radius = std::max(computeBoundsRadius(bounds), 0.5f);
        const float screenWidth  = static_cast<float>(GetScreenWidth());
        const float screenHeight = static_cast<float>(GetScreenHeight());
        const float aspect       = (screenHeight > 0.0f) ? (screenWidth / screenHeight) : 1.0f;

        const float fovYRad     = renderer.getCameraFov() * DEG2RAD;
        const float tanHalfFovY = std::max(std::tan(fovYRad * 0.5f), 0.001f);
        float       distanceY   = radius / tanHalfFovY;

        float distanceX = distanceY;
        if (aspect > 0.0f)
        {
            const float fovXRad     = 2.0f * std::atan(std::tan(fovYRad * 0.5f) * aspect);
            const float tanHalfFovX = std::max(std::tan(fovXRad * 0.5f), 0.001f);
            distanceX               = radius / tanHalfFovX;
        }

        const float distance = std::max(distanceX, distanceY) * 1.2f;
        const Vector3 direction = Vector3Normalize(Vector3{1.4f, 1.0f, 1.3f});

        renderer::RVec3 position{
            center.x + direction.x * distance,
            center.y + direction.y * distance,
            center.z + direction.z * distance};
        renderer.setCameraPosition(position);
    }

    renderer::RVec3 scaleVertex(
        const renderer::RVec3& vertex,
        const renderer::RVec3& center,
        const renderer::RVec3& scaleFactors)
    {
        renderer::RVec3 relative{
            vertex.x - center.x,
            vertex.y - center.y,
            vertex.z - center.z};

        return renderer::RVec3{
            center.x + relative.x * scaleFactors.x,
            center.y + relative.y * scaleFactors.y,
            center.z + relative.z * scaleFactors.z};
    }

    void applyScaleToMesh(
        renderer::RMesh&              destination,
        const renderer::RMesh&        source,
        const renderer::RVec3&        center,
        const renderer::RVec3&        scalePercent)
    {
        destination = source;
        renderer::RVec3 factors{
            scalePercent.x / 100.0f,
            scalePercent.y / 100.0f,
            scalePercent.z / 100.0f};

        for (auto& tri : destination.triangles)
        {
            tri.v0 = scaleVertex(tri.v0, center, factors);
            tri.v1 = scaleVertex(tri.v1, center, factors);
            tri.v2 = scaleVertex(tri.v2, center, factors);
        }
    }

    bool drawResizePanel(ResizePanelState& resizeState)
    {
        if (!resizeState.show)
            return false;

        refreshTextFields(resizeState);

        const float panelWidth  = 460.0f;
        const float panelHeight = 210.0f;
        Rectangle   panelBounds{
            (static_cast<float>(GetScreenWidth()) - panelWidth) * 0.5f,
            88.0f,
            panelWidth,
            panelHeight};

        GuiPanel(panelBounds, nullptr);
        GuiLabel({panelBounds.x + 20.0f, panelBounds.y + 10.0f, 220.0f, 20.0f}, "Resize / Scale");

        Rectangle checkboxBounds{panelBounds.x + 20.0f, panelBounds.y + 36.0f, 20.0f, 20.0f};
        bool      checkboxChanged = GuiCheckBox(checkboxBounds, "Uniform scale", &resizeState.uniformScale);
        bool      valuesChanged   = false;
        if (checkboxChanged && resizeState.uniformScale)
        {
            setScalePercent(resizeState, getAxisValue(resizeState.scalePercent, 0), 0);
            valuesChanged = true;
        }

        const float labelWidth  = 90.0f;
        const float margin      = 20.0f;
        const float usableWidth = panelWidth - margin * 2.0f - labelWidth;
        const float columnWidth = usableWidth / 3.0f;
        const float headerY     = panelBounds.y + 74.0f;
        const float scaleRowY   = headerY + 20.0f;
        const float sizeRowY    = scaleRowY + 52.0f;

        GuiLabel({panelBounds.x + margin, scaleRowY + 4.0f, labelWidth, 24.0f}, "Scale (%)");
        GuiLabel({panelBounds.x + margin, sizeRowY + 4.0f, labelWidth, 24.0f}, "Size (mm)");

        static const char* axisLabels[3] = {"X", "Y", "Z"};
        static const Color axisColors[3] = {RED, GREEN, BLUE};

        for (int axis = 0; axis < 3; ++axis)
        {
            const std::size_t idx    = static_cast<std::size_t>(axis);
            float             columnX = panelBounds.x + margin + labelWidth + static_cast<float>(axis) * columnWidth;

            int   textWidth  = MeasureText(axisLabels[axis], 16);
            float textWidthF = static_cast<float>(textWidth);
            float textX      = columnX + (columnWidth - textWidthF) * 0.5f;
            DrawText(
                axisLabels[axis],
                static_cast<int>(textX),
                static_cast<int>(headerY - 12.0f),
                16,
                axisColors[axis]);

            Rectangle scaleBox{columnX, scaleRowY, columnWidth - 12.0f, 28.0f};
            if (GuiTextBox(
                    scaleBox,
                    resizeState.scaleFields[idx].data(),
                    static_cast<int>(resizeState.scaleFields[idx].size()),
                    resizeState.scaleEditing[idx]))
                resizeState.scaleEditing[idx] = !resizeState.scaleEditing[idx];

            if (!resizeState.scaleEditing[idx])
                valuesChanged |= applyScaleInput(resizeState, axis);

            Rectangle sizeBox{columnX, sizeRowY, columnWidth - 12.0f, 28.0f};
            if (GuiTextBox(
                    sizeBox,
                    resizeState.sizeFields[idx].data(),
                    static_cast<int>(resizeState.sizeFields[idx].size()),
                    resizeState.sizeEditing[idx]))
                resizeState.sizeEditing[idx] = !resizeState.sizeEditing[idx];

            if (!resizeState.sizeEditing[idx])
                valuesChanged |= applySizeInput(resizeState, axis);
        }

        return valuesChanged;
    }

    struct WindowState
    {
        int windowedWidth;
        int windowedHeight;
    };

    void rememberWindowedSize(WindowState& state)
    {
        state.windowedWidth  = GetScreenWidth();
        state.windowedHeight = GetScreenHeight();
    }

    void toggleFullscreen(WindowState& state)
    {
        if (IsWindowFullscreen())
        {
            ToggleFullscreen();
            SetWindowSize(state.windowedWidth, state.windowedHeight);
        }
        else
        {
            rememberWindowedSize(state);
            ToggleFullscreen();
        }
    }

    Texture2D createToolbarIcon(const std::function<void(Image&)>& painter)
    {
        constexpr int iconSize = 64;
        Image         canvas   = GenImageColor(iconSize, iconSize, Color{0, 0, 0, 0});
        painter(canvas);
        Texture2D texture = LoadTextureFromImage(canvas);
        UnloadImage(canvas);
        return texture;
    }

    ToolbarButton makeToolbarButton(
        const std::string&                 id,
        std::function<void()>              onClick,
        const std::function<void(Image&)>& iconPainter)
    {
        ToolbarButton button;
        button.id      = id;
        button.onClick = std::move(onClick);
        if (iconPainter)
        {
            button.icon    = createToolbarIcon(iconPainter);
            button.hasIcon = true;
        }
        return button;
    }
} // namespace

int main()
{
    renderer::Config         cfg{1280, 720, "Prototype 3D Slicer", 60};
    renderer::RaylibRenderer renderer(cfg);

    objects3D::OMesh mesh       = objects3D::OMesh::fromSTL("assets/stl/binary/Farfetchd.stl");
    const MeshBounds meshBounds = computeMeshBounds(mesh);
    renderer::RMesh  baseMesh   = renderer::RMesh::fromOMesh(mesh, {255, 0, 0, 255});
    renderer::RMesh  renderMesh = baseMesh;

    ResizePanelState resizePanel = initResizePanelState(meshBounds);
    WindowState      windowState{GetScreenWidth(), GetScreenHeight()};

    applyScaleToMesh(renderMesh, baseMesh, resizePanel.center, resizePanel.scalePercent);
    frameCameraOnMesh(renderer, meshBounds);

    CenteredToolbar toolbar(18.0f, 52.0f, 14.0f);

    std::vector<ToolbarButton> toolbarButtons;
    toolbarButtons.reserve(5);
    toolbarButtons.emplace_back(makeToolbarButton(
        "import",
        []() { std::cout << "[Toolbar] Import bouton\n"; },
        [](Image& image) {
            const Color accent{0, 190, 255, 255};
            ImageDrawRectangle(&image, 30, 10, 4, 26, accent);
            ImageDrawTriangle(
                &image,
                Vector2{18.0f, 32.0f},
                Vector2{46.0f, 32.0f},
                Vector2{32.0f, 50.0f},
                accent);
        }));
    toolbarButtons.emplace_back(makeToolbarButton(
        "export",
        []() { std::cout << "[Toolbar] Export bouton\n"; },
        [](Image& image) {
            const Color accent{67, 176, 65, 255};
            ImageDrawRectangle(&image, 30, 20, 4, 26, accent);
            ImageDrawTriangle(
                &image,
                Vector2{32.0f, 14.0f},
                Vector2{18.0f, 32.0f},
                Vector2{46.0f, 32.0f},
                accent);
        }));
    toolbarButtons.emplace_back(makeToolbarButton(
        "slice",
        []() { std::cout << "[Toolbar] Slice bouton\n"; },
        [](Image& image) {
            const Color accent{243, 156, 18, 255};
            ImageDrawRectangle(&image, 16, 14, 32, 6, accent);
            ImageDrawRectangle(&image, 16, 26, 32, 6, accent);
            ImageDrawRectangle(&image, 16, 38, 32, 6, accent);
        }));
    toolbarButtons.emplace_back(makeToolbarButton(
        "preview",
        []() { std::cout << "[Toolbar] Preview bouton\n"; },
        [](Image& image) {
            const Color outline{236, 240, 241, 255};
            const Color iris{0, 151, 230, 255};
            ImageDrawCircleLines(&image, 32, 32, 18, outline);
            ImageDrawCircle(&image, 32, 32, 10, iris);
            ImageDrawCircle(&image, 32, 32, 4, Color{255, 255, 255, 255});
        }));
    toolbarButtons.emplace_back(makeToolbarButton(
        "settings",
        [&resizePanel]() { resizePanel.show = !resizePanel.show; },
        [](Image& image) {
            const Color accent{155, 89, 182, 255};
            ImageDrawRectangle(&image, 12, 29, 40, 6, accent);
            ImageDrawRectangle(&image, 29, 12, 6, 40, accent);
            ImageDrawCircle(&image, 32, 32, 8, accent);
        }));

    while (!renderer.shouldClose())
    {
        renderer.beginFrame();

        // Start 3D mode
        renderer.begin3D();

        renderer.drawGrid(10, 1.0f);
        renderer.drawAxis(2.0f);
        renderer.drawMesh(renderMesh);

        renderer.end3D();
        toolbar.draw(toolbarButtons);
        if (drawResizePanel(resizePanel))
            applyScaleToMesh(renderMesh, baseMesh, resizePanel.center, resizePanel.scalePercent);
        renderer.endFrame();

        if (IsKeyPressed(KEY_F11))
            toggleFullscreen(windowState);

        if (IsWindowResized() && !IsWindowFullscreen())
            rememberWindowedSize(windowState);
    }

    for (ToolbarButton& button : toolbarButtons)
    {
        if (button.hasIcon)
            UnloadTexture(button.icon);
    }

    return 0;
}
