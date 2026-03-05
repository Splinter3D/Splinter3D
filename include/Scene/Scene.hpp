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
            for (size_t i = 0; i < _objects.size(); ++i)
            {
                _objects[i]->setColor({255, 255, 255, 255}); // Reset color for all objects
            } // Reset selection before checking for hits TODO Remove this
            for (size_t i = 0; i < _objects.size(); ++i)
            {
                if (_objects[i]->isHit(ray))
                {
                    _selectedObjectIndex     = (int) i;
                    _lastSelectedObjectIndex = _selectedObjectIndex;
                    std::cout << "Object " << i << " is hit!" << std::endl;
                    _objects[(size_t) _selectedObjectIndex]->setColor({255, 0, 0, 255}); // Highlight the selected object
                    return;
                }
            }
            _selectedObjectIndex = _lastSelectedObjectIndex; // Keeping the last selected if no new object is hit
            if (_selectedObjectIndex >= 0 && _selectedObjectIndex < (int) _objects.size())
            {
                _objects[(size_t) _selectedObjectIndex]->setColor({255, 0, 0, 255}); // Highlight the selected object
            }
        }

        int getSelectedIndex() const
        {
            return _selectedObjectIndex;
        }

        SceneObject* getSelected()
        {
            std::cout << "Selected index: " << _selectedObjectIndex << std::endl;
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