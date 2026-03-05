#include <Renderer/RaylibRenderer.hpp>
#include <Splinter3D/Utils/Logger.hpp>
#include <Splinter3D/Utils/OSCompatibility.hpp>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <raylib.h>
#include <rlgl.h>
#include <vector>

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

    // --------------------------
    // Helpers: convert to Raylib
    // --------------------------
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
        EndMode3D();
    }

    void RaylibRenderer::endFrame()
    {
        EndDrawing();
    }

    // ------------------------
    // WINDOW / INPUT
    // ------------------------

    bool RaylibRenderer::shouldClose() const
    {
        return WindowShouldClose() || impl_->exitRequested;
    }

    void RaylibRenderer::requestClose()
    {
        impl_->exitRequested = true;
    }

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

    bool RaylibRenderer::isKeyDown(Key key) const
    {
        return IsKeyDown(static_cast<int>(key));
    }

    bool RaylibRenderer::isMouseButtonDown(int button) const
    {
        return IsMouseButtonDown(button);
    }

    geometry::Vec3 RaylibRenderer::getMouseDelta() const
    {
        Vector2 delta = GetMouseDelta();
        return geometry::Vec3(delta.x, delta.y, 0.0f);
    }

    // ------------------------
    // 3D DRAWING
    // ------------------------

    void RaylibRenderer::drawGrid(int slices, float spacing)
    {
        DrawGrid(slices, spacing);
    }

    void RaylibRenderer::drawAxis(float size)
    {
        DrawLine3D({0, 0, 0}, {size, 0, 0}, ::RED);
        DrawLine3D({0, 0, 0}, {0, size, 0}, ::GREEN);
        DrawLine3D({0, 0, 0}, {0, 0, size}, ::BLUE);
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

    void RaylibRenderer::drawTriangle(const geometry::Triangle& tri, Color color)
    {
        rlDisableBackfaceCulling();
        // geometry::Vec3 cameraPos = {impl_->camera.position.x, impl_->camera.position.y, impl_->camera.position.z};
        // ensureCCW(tri, cameraPos);

        DrawTriangle3D(
            tri.vertices[0].toRaylib(),
            tri.vertices[1].toRaylib(),
            tri.vertices[2].toRaylib(),
            toRaylibColor(color));

        rlEnableBackfaceCulling();
    }

    void RaylibRenderer::drawMesh(const geometry::Mesh& mesh, Color color)
    {
        for (auto& tri : mesh.triangles)
        {
            drawTriangle(tri, color);
        }
    }

    void RaylibRenderer::drawObject(const RenderObject& obj, Color color)
    {
        rlPushMatrix();
        rlMultMatrixf(MatrixToFloat(obj.modelMatrix));
        drawMesh(*obj.object->mesh, color);
        rlPopMatrix();
    }
} // namespace renderer
