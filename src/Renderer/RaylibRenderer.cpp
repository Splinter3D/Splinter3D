#include <Renderer/RaylibRenderer.hpp>
#include <Splinter3D/Utils/Logger.hpp>
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
        bool     exitRequested{false};
    };

    RaylibRenderer::RaylibRenderer(const Config& cfg)
        : impl_(std::make_unique<Impl>())
    {
        impl_->cfg = cfg;

        SetTraceLogCallback(RaylibToLogger);
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
        InitWindow(cfg.width, cfg.height, cfg.title.c_str());
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

    inline ::Vector3 toRaylibVec3(const renderer::RVec3& v)
    {
        return ::Vector3{v.x, v.y, v.z};
    }

    inline std::vector<::Vector3> toRaylibVec3(const std::vector<RVec3>& vecs)
    {
        std::vector<::Vector3> result;
        result.reserve(vecs.size());
        for (auto& v : vecs)
            result.push_back(toRaylibVec3(v));
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
        UpdateCamera(&impl_->camera, CAMERA_ORBITAL);
        (void) dt;
    }

    bool RaylibRenderer::isKeyDown(Key key) const
    {
        return IsKeyDown(static_cast<int>(key));
    }

    bool RaylibRenderer::isMouseButtonDown(int button) const
    {
        return IsMouseButtonDown(button);
    }

    RVec3 RaylibRenderer::getMouseDelta() const
    {
        Vector2 delta = GetMouseDelta();
        return RVec3(delta.x, delta.y, 0.0f);
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

    void RaylibRenderer::ensureCCW(RVec3& v0, RVec3& v1, RVec3& v2, RVec3& cameraPos)
    {
        RVec3 normal = renderer::RVec3::cross(v1 - v0, v2 - v0);
        RVec3 camDir = renderer::RVec3{cameraPos.x, cameraPos.y, cameraPos.z} - v0;
        float dot    = renderer::RVec3::dotProduct(normal, camDir);
        if (dot < 0)
        {
            // swap two vertices to make CCW
            std::swap(v1, v2);
        }
    }

    void RaylibRenderer::drawTriangle(RTriangle& tri)
    {
        rlDisableBackfaceCulling();
        RVec3 cameraPos = {impl_->camera.position.x, impl_->camera.position.y, impl_->camera.position.z};
        ensureCCW(tri.v0, tri.v1, tri.v2, cameraPos);

        DrawTriangle3D(
            toRaylibVec3(tri.v0),
            toRaylibVec3(tri.v1),
            toRaylibVec3(tri.v2),
            toRaylibColor(tri.color));

        rlEnableBackfaceCulling();
    }

    void RaylibRenderer::drawMesh(RMesh& mesh)
    {
        for (auto& tri : mesh.triangles)
        {
            drawTriangle(tri);
        }
    }
} // namespace renderer
