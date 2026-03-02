#include <Renderer/RaylibRenderer.hpp>
#include <memory>
#include <raylib.h>
#include <rlgl.h>
#include <vector>

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
        geometry::Vec3 v0 = tri.vertices[0];
        geometry::Vec3 v1 = tri.vertices[1];
        geometry::Vec3 v2 = tri.vertices[2];
        geometry::Vec3 normal = geometry::Vec3::cross(v1 - v0, v2 - v0);
        geometry::Vec3 camDir = geometry::Vec3{cameraPos.x, cameraPos.y, cameraPos.z} - v0;
        float dot    = geometry::Vec3::dotProduct(normal, camDir);
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
