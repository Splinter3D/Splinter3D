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
