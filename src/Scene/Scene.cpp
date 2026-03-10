/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** Scene
*/

#include <Geometry/Utils/Splitter/MeshSplitter.hpp>
#include <Scene/Scene.hpp>
#include <iostream>

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
                    .publish(splinter3D::events::ObjectSelectedEvent{i});
                return;
            }
        }
        _selectedObjectIndex = _lastSelectedObjectIndex; // Keeping the last selected if no new object is hit
        if (_selectedObjectIndex >= 0 && _selectedObjectIndex < (int) _objects.size())
        {
            _objects[(size_t) _selectedObjectIndex]->setColor({255, 0, 0, 255}); // Highlight the selected object
        }
        splinter3D::events::EventBus::getInstance()
            .publish(splinter3D::events::ObjectSelectedEvent{_selectedObjectIndex});
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
            .publish(splinter3D::events::ObjectSelectedEvent{-1});
    }

    void Scene::duplicateSelected()
    {
        if (_selectedObjectIndex < 0 || _selectedObjectIndex >= (int) _objects.size())
            return;

        const auto& selectedObj = _objects[(size_t) _selectedObjectIndex];
        auto        newObj      = std::make_unique<SceneObject>(*selectedObj);
        _objects.push_back(std::move(newObj));
    }

    void Scene::splitSelected(float x, float y, float z)
    {
        if (_selectedObjectIndex < 0 || _selectedObjectIndex >= (int) _objects.size())
            return;

        std::unique_ptr<SceneObject>& selectedObj = _objects[(size_t) _selectedObjectIndex];
        geometry::Mesh*               mesh        = selectedObj->getObject3D()->getTransformedMesh();

        std::vector<geometry::Mesh> slabMeshes = geometry::utils::splitter::splitByGrid(*mesh, geometry::Vec3{x, y, z});
        std::vector<SceneObject>    newObjects = SceneObject::batchBuildFromMeshes(slabMeshes, selectedObj->getColor());

        for (int i = 0; i < (int) newObjects.size(); ++i)
        {
            newObjects[(size_t) i].setColor(renderer::Color{
                static_cast<unsigned char>(rand() % 256),
                static_cast<unsigned char>(rand() % 256),
                static_cast<unsigned char>(rand() % 256),
                255}); // Assign a random color to each new object for better visibility
        }
        _objects.erase(_objects.begin() + _selectedObjectIndex);
        for (auto& obj : newObjects)
        {
            _objects.push_back(std::make_unique<SceneObject>(obj)); // Add new objects to the scene
        }
    }

} // namespace scene
