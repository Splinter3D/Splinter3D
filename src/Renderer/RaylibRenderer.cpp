#include <Renderer/Palette.hpp>
#include <Renderer/RayGUI.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <Splinter3D/Utils/Logger.hpp>
#include <Splinter3D/Utils/OSCompatibility.hpp>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <raylib.h>
#include <rlgl.h>
#include <vector>

#pragma region Logger callback

static void RaylibToLogger([[maybe_unused]] int         logLevel,
                           [[maybe_unused]] const char* text, [[maybe_unused]] va_list args)
{
#if !defined(SPLINTER3D_DEBUG)
    return;
#endif

    char* buf = NULL;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

    int ret = vasprintf(&buf, text, args);

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    if (ret < 0)
    {
        return;
    }

    const char* lvl = "INFO";
    switch (logLevel)
    {
        case LOG_TRACE:
            lvl = "TRACE";
            break;
        case LOG_DEBUG:
            lvl = "DEBUG";
            break;
        case LOG_INFO:
            lvl = "INFO";
            break;
        case LOG_WARNING:
            lvl = "WARN";
            break;
        case LOG_ERROR:
            lvl = "ERROR";
            break;
        case LOG_FATAL:
            lvl = "FATAL";
            break;
        default:
            break;
    }

    splinter3D::utils::clog("[raylib ", lvl, "] ", buf);
    std::free(buf);
}

#pragma endregion
#pragma region CTOR / DTOR

namespace renderer
{
    struct RaylibRenderer::Impl
    {
        Config   cfg;
        Camera3D camera{};

        float yaw      = 0.0f;
        float pitch    = 0.0f;
        float distance = 5.0f;

        bool exitRequested{false};
    };

    RaylibRenderer::RaylibRenderer(const Config& cfg)
        : impl_(std::make_unique<Impl>())
    {
        impl_->cfg = cfg;

        SetTraceLogCallback(RaylibToLogger);
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
        InitWindow(cfg.width, cfg.height, cfg.title.c_str());
        SetWindowMinSize(640, 360);
        SetTargetFPS(cfg.target_fps);
        SetExitKey(0);

        impl_->camera = {
            .position   = {5.0f, 5.0f, 5.0f},
            .target     = {0.0f, 0.0f, 0.0f},
            .up         = {0.0f, 1.0f, 0.0f},
            .fovy       = 45.0f,
            .projection = CAMERA_PERSPECTIVE};
    }

    RaylibRenderer::~RaylibRenderer()
    {
        CloseWindow();
    }

#pragma region HELPERS

    inline ::Color toRaylibColor(const renderer::Color& c)
    {
        return ::Color{c.r, c.g, c.b, c.a};
    }

    inline std::vector<::Vector3> toRaylibVec3(const std::vector<geometry::Vec3>& vecs)
    {
        std::vector<::Vector3> result;
        result.reserve(vecs.size());
        for (auto& v : vecs)
            result.push_back(v.toRaylib());
        return result;
    }

#pragma endregion
#pragma region Lifecycle

    // ------------------------
    // FRAME LIFECYCLE
    // ------------------------

    float RaylibRenderer::beginFrame()
    {
        BeginDrawing();
        ClearBackground(::DARKGRAY);
        return GetFrameTime();
    }

    void RaylibRenderer::begin3D()
    {
        BeginMode3D(impl_->camera);
    }

    void RaylibRenderer::end3D()
    {
        for (auto& cmd : drawQueue_[Layer::World])
        {
            std::visit([](auto&& c) {
                using T = std::decay_t<decltype(c)>;
                if constexpr (std::is_same_v<T, TriangleCmd>)
                {
                    rlPushMatrix();
                    rlMultMatrixf(MatrixToFloat(c.modelMatrix));
                    rlDisableBackfaceCulling();
                    DrawTriangle3D(
                        c.tri.vertices[0].toRaylib(),
                        c.tri.vertices[1].toRaylib(),
                        c.tri.vertices[2].toRaylib(),
                        toRaylibColor(c.color));
                    rlEnableBackfaceCulling();
                    rlPopMatrix();
                }
                else if constexpr (std::is_same_v<T, LineCmd>)
                    DrawLine3D(c.start.toRaylib(), c.end.toRaylib(), toRaylibColor(c.color));
                else if constexpr (std::is_same_v<T, GridCmd>)
                    DrawGrid(c.slices, c.spacing);
            },
                       cmd);
        }
        drawQueue_[Layer::World].clear();
        EndMode3D();
    }

    void RaylibRenderer::endFrame()
    {
        for (Layer layer : {Layer::UI, Layer::Overlay, Layer::Debug})
        {
            for (auto& cmd : drawQueue_[layer])
            {
                std::visit([](auto&& c) {
                    using T = std::decay_t<decltype(c)>;
                    if constexpr (std::is_same_v<T, RectCmd>)
                        DrawRectangleRec({c.x, c.y, c.w, c.h}, toRaylibColor(c.color));
                    else if constexpr (std::is_same_v<T, RectLinesCmd>)
                        DrawRectangleLinesEx({c.x, c.y, c.w, c.h}, 1.0f, toRaylibColor(c.color));
                    else if constexpr (std::is_same_v<T, TextureCmd>)
                    {
                        const auto* rt = static_cast<const RaylibTexture*>(c.texture);
                        if (!rt)
                            return;
                        DrawTexturePro(rt->tex, c.src, c.dest, {0, 0}, 0.0f, ::WHITE);
                    }
                    else if constexpr (std::is_same_v<T, ValueBoxCmd>)
                    {
                        int val = c.value;
                        if (GuiValueBox({c.x, c.y, c.w, c.h}, c.label.c_str(), &val, c.min, c.max, c.editMode))
                            if (c.outEdit)
                                *c.outEdit = !c.editMode;
                        if (c.outValue)
                            *c.outValue = val;
                    }
                    else if constexpr (std::is_same_v<T, ValueBoxFloatCmd>)
                    {
                        float val  = c.value;
                        bool  edit = c.editMode; // local copy
                        if (GuiFloatValueBox({c.x, c.y, c.w, c.h}, c.label.c_str(),
                                             &val, c.min, c.max, edit))
                        {
                            if (c.outEdit)
                                *c.outEdit = !edit; // toggle using local, not c.editMode
                        }
                        else
                        {
                            if (c.outEdit)
                                *c.outEdit = edit; // write back any internal state changes
                        }
                        if (c.outValue)
                            *c.outValue = val;
                    }
                    else if constexpr (std::is_same_v<T, CheckboxCmd>)
                    {
                        bool checked = c.checked;
                        GuiCheckBox({c.x, c.y, c.size, c.size}, c.label.c_str(), &checked);
                        if (c.outChecked)
                            *c.outChecked = checked;
                    }
                    else if constexpr (std::is_same_v<T, TextCmd>)
                        DrawText(c.text.c_str(), c.x, c.y, c.fontSize, toRaylibColor(c.color));
                },
                           cmd);
            }
            drawQueue_[layer].clear();
        }
        EndDrawing();
    }

#pragma endregion
#pragma region WINDOW / INPUT

    bool RaylibRenderer::shouldClose() const
    {
        return WindowShouldClose() || impl_->exitRequested;
    }

    void RaylibRenderer::requestClose()
    {
        impl_->exitRequested = true;
    }

    bool RaylibRenderer::isKeyDown(Key key) const
    {
        return IsKeyDown(static_cast<int>(key));
    }

    bool RaylibRenderer::isKeyPressed(Key key) const
    {
        return IsKeyPressed(static_cast<int>(key));
    }

    bool RaylibRenderer::isMouseButtonDown(int button) const
    {
        return IsMouseButtonDown(button);
    }

    bool RaylibRenderer::isMouseButtonPressed(int button) const
    {
        return IsMouseButtonPressed(button);
    }

    geometry::Vec3 RaylibRenderer::getMousePosition() const
    {
        Vector2 position = GetMousePosition();
        return geometry::Vec3(position.x, position.y, 0.0f);
    }

    geometry::Vec3 RaylibRenderer::getMouseDelta() const
    {
        Vector2 delta = GetMouseDelta();
        return geometry::Vec3(delta.x, delta.y, 0.0f);
    }

    int RaylibRenderer::getScreenWidth() const
    {
        return GetScreenWidth();
    }

    int RaylibRenderer::getScreenHeight() const
    {
        return GetScreenHeight();
    }

    float RaylibRenderer::getDeltaTime() const
    {
        return GetFrameTime();
    }

    geometry::Ray RaylibRenderer::getMouseRay() const
    {
        Ray ray = GetMouseRay(GetMousePosition(), impl_->camera);
        return geometry::Ray{
            {ray.position.x, ray.position.y, ray.position.z},
            {ray.direction.x, ray.direction.y, ray.direction.z}};
    }

#pragma endregion
#pragma region CAMERA

    void RaylibRenderer::updateCamera(float dt)
    {
        (void) dt;

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Vector2 delta = GetMouseDelta();
            impl_->yaw -= delta.x * 0.01f;
            impl_->pitch -= delta.y * 0.01f;
        }

        impl_->distance -= GetMouseWheelMove();
        impl_->distance = std::max(impl_->distance, 1.0f);

        impl_->camera.position = {
            impl_->camera.target.x + impl_->distance * cosf(impl_->pitch) * sinf(impl_->yaw),
            impl_->camera.target.y + impl_->distance * sinf(impl_->pitch),
            impl_->camera.target.z + impl_->distance * cosf(impl_->pitch) * cosf(impl_->yaw)};
        UpdateCamera(&impl_->camera, CAMERA_CUSTOM);
    }

    void RaylibRenderer::setCameraPosition(const geometry::Vec3& position)
    {
        impl_->camera.position = position.toRaylib();
    }

    void RaylibRenderer::setCameraTarget(const geometry::Vec3& target)
    {
        impl_->camera.target = target.toRaylib();
    }

    void RaylibRenderer::setCameraFov(float fovY)
    {
        impl_->camera.fovy = fovY;
    }

    float RaylibRenderer::getCameraFov() const
    {
        return impl_->camera.fovy;
    }

    void RaylibRenderer::setOrbitDistance(float d)
    {
        impl_->distance = d;
    }

    void RaylibRenderer::setOrbitAngles(float yaw, float pitch)
    {
        impl_->yaw   = yaw;
        impl_->pitch = pitch;
    }

#pragma endregion
#pragma region GUI DRAWING

    void RaylibRenderer::drawGuiComponent(const gui::IGuiComponent& component) const
    {
        component.draw(*this);
    }

    ITexture* RaylibRenderer::createIcon(int width, int height, const std::function<void(void*)>& painter)
    {
        RaylibTexture* icon  = new RaylibTexture();
        Image          image = GenImageColor(width, height, {0, 0, 0, 0});

        painter(&image);
        icon->tex = LoadTextureFromImage(image);

        UnloadImage(image);
        return icon;
    }

    void RaylibRenderer::drawTexture(float x, float y, float width, float height, const ITexture* texture, Layer layer) const
    {
        const RaylibTexture* rt = static_cast<const RaylibTexture*>(texture);
        if (!rt)
            return;

        Rectangle src  = {0.0f, 0.0f, static_cast<float>(rt->tex.width), static_cast<float>(rt->tex.height)};
        Rectangle dest = {x, y, width, height};
        drawQueue_[layer].push_back(TextureCmd{texture, src, dest});
    }

    void RaylibRenderer::drawButton(float x, float y, float width, float height,
                                    const renderer::ITexture*    icon,
                                    const std::function<void()>& onClick, Layer layer) const
    {
        Rectangle rect{x, y, width, height};

        if (GuiButton(rect, "")) // Using raygui button detection
        {
            if (onClick)
                onClick();
        }

        if (icon)
        {
            float iconSize = width * 0.55f;
            this->drawTexture(x + (width - iconSize) * 0.5f, y + (height - iconSize) * 0.5f, iconSize, iconSize, icon, layer);
        }
    }

    void RaylibRenderer::drawPanel(float x, float y, float width, float height, Layer layer) const
    {
        drawQueue_[layer].push_back(RectCmd{x, y, width, height, Palette::Background});
    }

    void RaylibRenderer::drawText(float x, float y, const char* text, int fontSize, Layer layer) const
    {
        drawQueue_[layer].push_back(TextCmd{text, (int) x, (int) y, fontSize, Palette::Secondary});
    }

    void RaylibRenderer::drawRectangle(float x, float y, float width, float height, Color color, Layer layer) const
    {
        drawQueue_[layer].push_back(RectCmd{x, y, width, height, color});
    }

    void RaylibRenderer::drawRectangleLines(float x, float y, float width, float height, Color color, Layer layer) const
    {
        drawQueue_[layer].push_back(RectLinesCmd{x, y, width, height, color});
    }

    void* RaylibRenderer::getCanvas() const
    {
        return nullptr; // Not needed for raylib since we draw directly on textures
    }

    void RaylibRenderer::drawValueBox(float x, float y, float w, float h, const char* label, int& value, int min, int max, bool& editMode, Layer layer) const
    {
        drawQueue_[layer].push_back(ValueBoxCmd{x, y, w, h, label, value, min, max, editMode, &value, &editMode});
    }

    void RaylibRenderer::drawFloatValueBox(float x, float y, float w, float h, const char* label, float& value, float min, float max, bool& editMode, Layer layer) const
    {
        drawQueue_[layer].push_back(ValueBoxFloatCmd{x, y, w, h, label, value, min, max, editMode, &value, &editMode});
    }

    void RaylibRenderer::drawCheckbox(float x, float y, float size, const char* label, bool& checked, Layer layer) const
    {
        drawQueue_[layer].push_back(CheckboxCmd{x, y, size, label, checked, &checked});
    }

#pragma region GUI Utils

    float RaylibRenderer::measureTextWidth(const char* text, int fontSize) const
    {
        return static_cast<float>(MeasureText(text, fontSize));
    }

#pragma endregion
#pragma region ICON

    void RaylibRenderer::drawImportIcon(void* canvas)
    {
        Image*        img = static_cast<Image*>(canvas);
        const ::Color accent{0, 190, 255, 255};
        ImageDrawRectangle(img, 30, 10, 4, 26, accent);
        ImageDrawTriangle(img, {18, 32}, {46, 32}, {32, 50}, accent);
    }

    void RaylibRenderer::drawExportIcon(void* canvas)
    {
        Image*        img = static_cast<Image*>(canvas);
        const ::Color accent{67, 176, 65, 255};
        ImageDrawRectangle(img, 30, 20, 4, 26, accent);
        ImageDrawTriangle(img, {32, 14}, {18, 32}, {46, 32}, accent);
    }

    void RaylibRenderer::drawSliceIcon(void* canvas)
    {
        Image*        img = static_cast<Image*>(canvas);
        const ::Color accent{243, 156, 18, 255};
        ImageDrawRectangle(img, 16, 14, 32, 6, accent);
        ImageDrawRectangle(img, 16, 26, 32, 6, accent);
        ImageDrawRectangle(img, 16, 38, 32, 6, accent);
    }

    void RaylibRenderer::drawPreviewIcon(void* canvas)
    {
        Image*        img = static_cast<Image*>(canvas);
        const ::Color outline{236, 240, 241, 255};
        const ::Color iris{0, 151, 230, 255};
        ImageDrawCircleLines(img, 32, 32, 18, outline);
        ImageDrawCircle(img, 32, 32, 10, iris);
        ImageDrawCircle(img, 32, 32, 4, {255, 255, 255, 255});
    }

    void RaylibRenderer::drawRotationIcon(void* canvas)
    {
        Image*        img = static_cast<Image*>(canvas);
        const ::Color c{100, 180, 255, 255};

        // Circular arrow (simplified)
        ImageDrawCircleLines(img, 32, 32, 18, c);
        ImageDrawTriangle(img, {32, 10}, {28, 16}, {36, 16}, c);
    }

    void RaylibRenderer::drawScaleIcon(void* canvas)
    {
        Image*        img = static_cast<Image*>(canvas);
        const ::Color c{243, 156, 18, 255};

        // Central box
        ImageDrawRectangleLines(img, {20, 20, 24, 24}, 2, c);

        // Corner arrows (diagonal scale feel)
        // Top-left
        ImageDrawLine(img, 20, 20, 8, 8, c);
        ImageDrawTriangle(img, {4, 4}, {14, 8}, {8, 14}, c);

        // Top-right
        ImageDrawLine(img, 44, 20, 56, 8, c);
        ImageDrawTriangle(img, {60, 4}, {50, 8}, {56, 14}, c);

        // Bottom-left
        ImageDrawLine(img, 20, 44, 8, 56, c);
        ImageDrawTriangle(img, {4, 60}, {8, 50}, {14, 56}, c);

        // Bottom-right
        ImageDrawLine(img, 44, 44, 56, 56, c);
        ImageDrawTriangle(img, {60, 60}, {50, 56}, {56, 50}, c);
    }

    void RaylibRenderer::drawTransformIcon(void* canvas)
    {
        Image*        img = static_cast<Image*>(canvas);
        const ::Color c{100, 180, 255, 255};

        const int cx = 32, cy = 32;
        const int len = 14, w = 5;

        // Cross lines
        ImageDrawLine(img, cx, cy - len, cx, cy + len, c); // vertical
        ImageDrawLine(img, cx - len, cy, cx + len, cy, c); // horizontal

        // Up arrow
        ImageDrawTriangle(img, {cx, cy - len - 8}, {cx - w, cy - len}, {cx + w, cy - len}, c);

        // Down arrow
        ImageDrawTriangle(img, {cx, cy + len + 8}, {cx - w, cy + len}, {cx + w, cy + len}, c);

        // Left arrow
        ImageDrawTriangle(img, {cx - len - 8, cy}, {cx - len, cy - w}, {cx - len, cy + w}, c);

        // Right arrow
        ImageDrawTriangle(img, {cx + len + 8, cy}, {cx + len, cy - w}, {cx + len, cy + w}, c);
    }

#pragma endregion
#pragma region 3D DRAWING

    void RaylibRenderer::drawGrid(int slices, float spacing, Layer layer)
    {
        drawQueue_[layer].push_back(GridCmd{slices, spacing});
    }

    void RaylibRenderer::drawAxis(float size, Layer layer)
    {
        drawQueue_[layer].push_back(LineCmd{{0, 0, 0}, {size, 0, 0}, RED});
        drawQueue_[layer].push_back(LineCmd{{0, 0, 0}, {0, size, 0}, GREEN});
        drawQueue_[layer].push_back(LineCmd{{0, 0, 0}, {0, 0, size}, BLUE});
    }

    void RaylibRenderer::ensureCCW(geometry::Triangle& tri, geometry::Vec3 cameraPos)
    {
        geometry::Vec3 v0     = tri.vertices[0];
        geometry::Vec3 v1     = tri.vertices[1];
        geometry::Vec3 v2     = tri.vertices[2];
        geometry::Vec3 normal = geometry::Vec3::cross(v1 - v0, v2 - v0);
        geometry::Vec3 camDir = geometry::Vec3{cameraPos.x, cameraPos.y, cameraPos.z} - v0;
        float          dot    = geometry::Vec3::dotProduct(normal, camDir);
        if (dot < 0)
        {
            // swap two vertices to make CCW
            std::swap(v1, v2);
        }
        tri.vertices[1] = v1;
        tri.vertices[2] = v2;
    }

    void RaylibRenderer::drawTriangle(const geometry::Triangle& tri, Color color, Layer layer)
    {
        drawQueue_[layer].push_back(TriangleCmd{tri, color, Matrix()});
    }

    void RaylibRenderer::drawMesh(const geometry::Mesh& mesh, Color color, Layer layer)
    {
        for (auto& tri : mesh.triangles)
        {
            drawTriangle(tri, color, layer);
        }
    }

    void RaylibRenderer::drawObject(const RenderObject& obj, Color color, Layer layer)
    {
        for (auto& tri : obj.object->getMesh()->triangles)
        {
            TriangleCmd cmd{tri, color, obj.modelMatrix};
            drawQueue_[layer].push_back(cmd);
        }
    }
} // namespace renderer
