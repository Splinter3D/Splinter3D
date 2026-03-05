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

        void draw(renderer::IRenderer& renderer) const; // declare only

      private:
        objects3D::Object3D    obj;
        renderer::RenderObject rObj;
    };
} // namespace scene