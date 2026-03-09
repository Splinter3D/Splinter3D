/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** Scene
*/

#pragma once

#include <Renderer/Color.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/SceneObject.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>
#include <Splinter3D/Utils/Singleton.hpp>
#include <memory>
#include <set>
#include <vector>

namespace scene
{
    class Scene : public splinter3D::utils::Singleton<Scene>
    {

      public:
        void addObject(const std::string& stlPath, renderer::Color color = {255, 255, 255, 255});

        void draw(renderer::IRenderer& renderer) const;

        void handleClick(const geometry::Ray& ray, bool additiveSelection = false);

        int getSelectedIndex() const;

        SceneObject*                   getSelected();
        std::vector<SceneObject*>      getSelectedObjects();
        std::vector<const SceneObject*> getSelectedObjects() const;

        void removeSelected();
        void duplicateSelected();

        std::unique_ptr<geometry::Mesh> getSelectedMesh(bool applyTransform = true);

        std::vector<objects3D::Object3D> getAllObjects3D() const
        {
            std::vector<objects3D::Object3D> result;
            for (const auto& obj : _objects)
            {
                result.push_back(*obj->getObject3D());
            }
            return result;
        }

      protected:
        Scene()           = default;
        ~Scene() noexcept = default;

      private:
        void updateSelectionVisuals();
        void notifySelectionChanged() const;

        std::vector<std::unique_ptr<SceneObject>> _objects;
        std::set<int>                              _selectedObjectIndices;
        int                                       _selectedObjectIndex{-1};

        friend class Singleton<Scene>;
    };
} // namespace scene
