#pragma once

#include <Renderer/IRenderer.hpp>
#include <memory>
#include <vector>

namespace renderer
{
    class RaylibRenderer final : public IRenderer
    {
      public:
        explicit RaylibRenderer(const Config& cfg = {});
        ~RaylibRenderer() override;

        // --------------------
        // Frame lifecycle
        // --------------------
        float beginFrame() override;
        void  endFrame() override;

        // --------------------
        // 3D rendering
        // --------------------
        void begin3D() override;
        void end3D() override;

        // --------------------
        // Drawing primitives
        // --------------------
        void drawTriangle(const geometry::Triangle& tri, Color color) override;
        void drawMesh(const geometry::Mesh& mesh, Color color) override;

        // --------------------
        // RenderObject
        // --------------------
        void drawObject(const RenderObject& obj, Color color) override;

        // --------------------
        // Helpers
        // --------------------
        void drawGrid(int slices, float spacing) override;
        void drawAxis(float size) override;

        // --------------------
        // Camera
        // --------------------
        void  updateCamera(float deltaTime);
        void  setCameraPosition(const geometry::Vec3& position);
        void  setCameraTarget(const geometry::Vec3& target);
        void  setCameraFov(float fovY);
        float getCameraFov() const;

        // --------------------
        // Window / input
        // --------------------
        bool shouldClose() const override;
        void requestClose() override;

        bool           isKeyDown(Key key) const override;
        bool           isMouseButtonDown(int button) const;
        geometry::Vec3 getMouseDelta() const;

      private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        void ensureCCW(geometry::Triangle& tri, geometry::Vec3 cameraPos);
    };
} // namespace renderer
