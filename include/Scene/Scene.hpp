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
#include <Splinter3D/Utils/Singleton.hpp>

namespace scene
{
    struct Scene : public splinter3D::utils::Singleton<Scene>
    {
        Scene() = default;

        void addObject(const std::string& stlPath, renderer::Color color = {255, 255, 255, 255})
        {
            _objects.emplace_back(std::make_unique<SceneObject>(stlPath, color));
        }

        void draw(renderer::IRenderer& renderer) const
        {
            for (const auto& obj : _objects)
            {
                obj->draw(renderer);
            }
        }

      private:
        std::vector<std::unique_ptr<SceneObject>> _objects;
        int                                       _selectedObjectIndex{-1};
    };
} // namespace scene