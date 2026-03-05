#pragma once
#include <Objects3D/Object3D.hpp>
#include <Objects3D/ObjectObserver.hpp>
#include <Renderer/Color.hpp>
#include <raylib.h>
#include <raymath.h>

namespace renderer
{
    class IRenderer;

    struct RenderObject : objects3D::ObjectObserver
    {
        const objects3D::Object3D* object = nullptr;
        Matrix                     modelMatrix;

        RenderObject(Color color = Color{255, 255, 255, 255});

        void bind(objects3D::Object3D& obj);
        void draw(renderer::IRenderer& renderer) const;
        void setColor(const Color& c);

        void            onTransformChanged() override;
        geometry::Mesh* getTransformedMesh() const;

      private:
        void  rebuildMatrix();
        Color _color;
    };
} // namespace renderer
