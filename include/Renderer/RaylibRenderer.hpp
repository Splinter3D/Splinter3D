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
        void  begin3D() override;
        void  end3D() override;

        // --------------------
        // Drawing Gui
        // --------------------
        void drawIconCentered(const Rectangle& bounds, const Texture2D& icon, float scale = 0.55f) const;
        void drawGuiComponent(const gui::IGuiComponent& component) const override;

        // --------------------
        // Drawing 3D
        // --------------------
        void drawTriangle(const geometry::Triangle& tri, Color color) override;
        void drawMesh(const geometry::Mesh& mesh, Color color) override;
        void drawObject(const RenderObject& obj, Color color) override;
        void drawGrid(int slices, float spacing) override;
        void drawAxis(float size) override;

        // --------------------
        // Camera
        // --------------------
        void  updateCamera(float deltaTime) override;
        void  setCameraPosition(const geometry::Vec3& position) override;
        void  setCameraTarget(const geometry::Vec3& target) override;
        void  setCameraFov(float fovY) override;
        float getCameraFov() const override;
        void  setOrbitDistance(float d) override;
        void  setOrbitAngles(float yaw, float pitch) override;

        // --------------------
        // Window / input
        // --------------------
        bool           shouldClose() const override;
        void           requestClose() override;
        bool           isKeyDown(Key key) const override;
        bool           isMouseButtonDown(int button) const override;
        geometry::Vec3 getMouseDelta() const override;

      private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        void ensureCCW(geometry::Triangle& tri, geometry::Vec3 cameraPos);
    };
} // namespace renderer
