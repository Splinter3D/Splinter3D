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

        void handleClick(const geometry::Ray& ray)
        {
            for (auto& obj : _objects)
            {
                obj->setColor({255, 255, 255, 255}); // Reset color of all objects
            }
            for (int i = 0; i < (int) _objects.size(); ++i)
            {
                if (_objects[(size_t) i]->isHit(ray))
                {
                    _selectedObjectIndex     = i;
                    _lastSelectedObjectIndex = i;
                    _objects[(size_t) _selectedObjectIndex]->setColor({255, 0, 0, 255}); // Highlight the selected object

                    splinter3D::events::EventBus::getInstance()
                        .publish(scene::events::ObjectSelectedEvent{i});
                    return;
                }
            }
            _selectedObjectIndex = _lastSelectedObjectIndex; // Keeping the last selected if no new object is hit
            if (_selectedObjectIndex >= 0 && _selectedObjectIndex < (int) _objects.size())
            {
                _objects[(size_t) _selectedObjectIndex]->setColor({255, 0, 0, 255}); // Highlight the selected object
            }
            splinter3D::events::EventBus::getInstance()
                .publish(scene::events::ObjectSelectedEvent{_selectedObjectIndex});
        }

        int getSelectedIndex() const
        {
            return _selectedObjectIndex;
        }

        SceneObject* getSelected()
        {
            if (_selectedObjectIndex < 0)
                return nullptr;
            return _objects[(size_t) _selectedObjectIndex].get();
        }

      private:
        std::vector<std::unique_ptr<SceneObject>> _objects;
        int                                       _selectedObjectIndex{-1};
        int                                       _lastSelectedObjectIndex{-1};
    };
} // namespace scene