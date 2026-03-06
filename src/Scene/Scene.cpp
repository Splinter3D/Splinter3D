/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** Scene
*/

#include <Scene/Scene.hpp>

namespace scene
{

    void Scene::addObject(const std::string& stlPath, renderer::Color color)
    {
        _objects.emplace_back(std::make_unique<SceneObject>(stlPath, color));
    }

    void Scene::draw(renderer::IRenderer& renderer) const
    {
        for (const auto& obj : _objects)
        {
            obj->draw(renderer);
        }
    }

    void Scene::handleClick(const geometry::Ray& ray)
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

    int Scene::getSelectedIndex() const
    {
        return _selectedObjectIndex;
    }

    SceneObject* Scene::getSelected()
    {
        if (_selectedObjectIndex < 0)
            return nullptr;
        return _objects[(size_t) _selectedObjectIndex].get();
    }

    void Scene::removeSelected()
    {
        if (_selectedObjectIndex < 0 || _selectedObjectIndex >= (int) _objects.size())
            return;

        _objects.erase(_objects.begin() + _selectedObjectIndex);
        _selectedObjectIndex     = -1;
        _lastSelectedObjectIndex = -1;

        splinter3D::events::EventBus::getInstance()
            .publish(scene::events::ObjectSelectedEvent{-1});
    }

    void Scene::duplicateSelected()
    {
        if (_selectedObjectIndex < 0 || _selectedObjectIndex >= (int) _objects.size())
            return;

        const auto& selectedObj = _objects[(size_t) _selectedObjectIndex];
        auto        newObj      = std::make_unique<SceneObject>(*selectedObj);
        _objects.push_back(std::move(newObj));
    }

} // namespace scene
