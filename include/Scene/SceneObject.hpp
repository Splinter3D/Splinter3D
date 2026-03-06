#pragma once
#include <Objects3D/Object3D.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/RenderObject.hpp> // needs full definition as member

namespace renderer
{
    class IRenderer;
} // namespace renderer

namespace scene
{
    struct SceneObject
    {
        SceneObject(const std::string& stlPath, renderer::Color color = {255, 255, 255, 255})
            : obj(objects3D::Object3D::fromSTL(stlPath))
        {
            rObj.bind(obj);
            rObj.setColor(color);
        }

        SceneObject(const SceneObject& other)
            : obj(other.obj)
        {
            rObj.bind(obj);
            rObj.setColor(other.rObj.getColor());
        }

        ~SceneObject() = default;

        void draw(renderer::IRenderer& renderer) const;
        bool isHit(const geometry::Ray& ray);
        void setColor(const renderer::Color& c);

        objects3D::Transform getTransform() const;
        geometry::Mesh*      getTransformedMesh() const;
        void                 setTransform(const objects3D::Transform& transform);

      private:
        objects3D::Object3D    obj;
        renderer::RenderObject rObj;
    };
} // namespace scene
