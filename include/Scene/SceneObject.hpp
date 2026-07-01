#pragma once
#include <Objects3D/Object3D.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/RenderObject.hpp> // needs full definition as member
#include <optional>

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

        SceneObject(const objects3D::Object3D& object, renderer::Color color = {255, 255, 255, 255})
            : obj(object)
        {
            rObj.bind(obj);
            rObj.setColor(color);
        }

        ~SceneObject() = default;

        /**
         * Batch build multiple SceneObjects from STL files.
         */
        static std::vector<SceneObject> batchBuildFromStl(const std::vector<std::string>& stlFiles, const renderer::Color& color = {255, 255, 255, 255})
        {
            std::vector<SceneObject> objects;
            objects.reserve(stlFiles.size());
            for (const auto& file : stlFiles)
                objects.emplace_back(file, color);
            return objects;
        }

        /**
         * Batch build multiple SceneObjects from meshes.
         */
        static std::vector<SceneObject> batchBuildFromMeshes(const std::vector<geometry::Mesh>& meshes, const renderer::Color& color = {255, 255, 255, 255})
        {
            std::vector<SceneObject> objects;
            objects.reserve(meshes.size());
            for (const auto& mesh : meshes)
                objects.emplace_back(SceneObject(objects3D::Object3D(new geometry::Mesh(mesh)), color));
            return objects;
        }

        void            draw(renderer::IRenderer& renderer) const;
        bool            isHit(const geometry::Ray& ray);
        void            setColor(const renderer::Color& c);
        renderer::Color getColor() const;

        objects3D::Object3D* getObject3D();
        objects3D::Transform getTransform() const;
        geometry::Mesh*      getTransformedMesh() const;
        void                 setTransform(const objects3D::Transform& transform);

      private:
        objects3D::Object3D    obj;
        renderer::RenderObject rObj;
    };
} // namespace scene
