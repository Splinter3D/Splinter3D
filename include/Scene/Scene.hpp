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
#include <iostream>
#include <memory>

namespace scene
{
    class Scene : public splinter3D::utils::Singleton<Scene>
    {

      public:
        void addObject(const std::string& stlPath, renderer::Color color = {255, 255, 255, 255});

        void draw(renderer::IRenderer& renderer) const;

        void handleClick(const geometry::Ray& ray);

        int getSelectedIndex() const;

        SceneObject* getSelected();

        void removeSelected();
        void duplicateSelected();

        std::unique_ptr<geometry::Mesh> getSelectedMesh(bool applyTransform = true)
        {
            SceneObject* selected = getSelected();
            if (!selected)
                return nullptr;
            if (applyTransform)
                return std::make_unique<geometry::Mesh>(*selected->getTransformedMesh());
            return std::make_unique<geometry::Mesh>(*selected->getObject3D()->getMesh());
        }

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
        std::vector<std::unique_ptr<SceneObject>> _objects;
        int                                       _selectedObjectIndex{-1};
        int                                       _lastSelectedObjectIndex{-1};

        friend class Singleton<Scene>;
    };
} // namespace scene
