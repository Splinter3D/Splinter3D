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

        void drawTriangle(RTriangle& tri) override;
        void drawTriangles(std::vector<RTriangle>& tris) override;

        void drawGrid(int slices, float spacing) override;
        void drawAxis(float size) override;

        // --------------------
        // Camera
        // --------------------
        void updateCamera(float deltaTime);

        // --------------------
        // Window / input
        // --------------------
        bool shouldClose() const override;
        void requestClose() override;

        bool isKeyDown(Key key) const override;
        bool isMouseButtonDown(int button) const;
        RVec3 getMouseDelta() const;

      private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
