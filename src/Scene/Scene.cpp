/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** Scene
*/

#include <Geometry/Utils/Splitter/MeshSplitter.hpp>
#include <Scene/Scene.hpp>
#include <algorithm>
#include <iostream>
#include <vector>

namespace
{
    constexpr renderer::Color kDefaultColor{255, 255, 255, 255};
    constexpr renderer::Color kSelectedColor{255, 0, 0, 255};
} // namespace

namespace scene
{

    void Scene::addObject(const std::string& stlPath, renderer::Color color)
    {
        _objects.emplace_back(std::make_unique<SceneObject>(stlPath, color));
    }

    void Scene::draw(renderer::IRenderer& renderer) const
    {
        for (const auto& obj : _objects)
            obj->draw(renderer);
    }

    void Scene::handleClick(const geometry::Ray& ray, bool additiveSelection)
    {
        int hitIndex = -1;
        for (int i = 0; i < (int) _objects.size(); ++i)
        {
            if (_objects[(size_t) i]->isHit(ray))
            {
                hitIndex = i;
                break;
            }
        }

        if (hitIndex >= 0)
        {
            if (!additiveSelection)
                _selectedObjectIndices.clear();

            const bool alreadySelected = _selectedObjectIndices.count(hitIndex) > 0;

            if (additiveSelection && alreadySelected)
            {
                _selectedObjectIndices.erase(hitIndex);
                _selectedObjectIndex = _selectedObjectIndices.empty() ? -1 : *_selectedObjectIndices.rbegin();
            }
            else
            {
                _selectedObjectIndices.insert(hitIndex);
                _selectedObjectIndex = hitIndex;
            }
        }
        else if (!additiveSelection)
        {
            _selectedObjectIndices.clear();
            _selectedObjectIndex = -1;
        }

        updateSelectionVisuals();
        notifySelectionChanged();
    }

    int Scene::getSelectedIndex() const
    {
        return _selectedObjectIndex;
    }

    SceneObject* Scene::getSelected()
    {
        if (_selectedObjectIndex < 0 || _selectedObjectIndex >= (int) _objects.size())
            return nullptr;
        return _objects[(size_t) _selectedObjectIndex].get();
    }

    std::vector<SceneObject*> Scene::getSelectedObjects()
    {
        std::vector<SceneObject*> result;
        result.reserve(_selectedObjectIndices.size());
        for (int idx : _selectedObjectIndices)
        {
            if (idx >= 0 && idx < (int) _objects.size())
                result.push_back(_objects[(size_t) idx].get());
        }
        return result;
    }

    std::vector<const SceneObject*> Scene::getSelectedObjects() const
    {
        std::vector<const SceneObject*> result;
        result.reserve(_selectedObjectIndices.size());
        for (int idx : _selectedObjectIndices)
        {
            if (idx >= 0 && idx < (int) _objects.size())
                result.push_back(_objects[(size_t) idx].get());
        }
        return result;
    }

    void Scene::removeSelected()
    {
        if (_selectedObjectIndices.empty())
            return;

        std::vector<int> toRemove(_selectedObjectIndices.begin(), _selectedObjectIndices.end());
        std::sort(toRemove.begin(), toRemove.end(), [](int lhs, int rhs) { return lhs > rhs; });

        for (int idx : toRemove)
        {
            if (idx >= 0 && idx < (int) _objects.size())
                _objects.erase(_objects.begin() + idx);
        }

        _selectedObjectIndices.clear();
        _selectedObjectIndex = -1;

        updateSelectionVisuals();
        notifySelectionChanged();
    }

    void Scene::duplicateSelected()
    {
        if (_selectedObjectIndex < 0 || _selectedObjectIndex >= (int) _objects.size())
            return;

        const auto& selectedObj = _objects[(size_t) _selectedObjectIndex];
        auto        newObj      = std::make_unique<SceneObject>(*selectedObj);
        _objects.push_back(std::move(newObj));
    }

    std::unique_ptr<geometry::Mesh> Scene::getSelectedMesh(bool applyTransform)
    {
        if (_selectedObjectIndices.empty())
            return nullptr;

        auto combined = std::make_unique<geometry::Mesh>();
        for (int idx : _selectedObjectIndices)
        {
            if (idx < 0 || idx >= (int) _objects.size())
                continue;
            auto* sceneObj = _objects[(size_t) idx].get();
            if (!sceneObj)
                continue;

            if (applyTransform)
            {
                std::unique_ptr<geometry::Mesh> mesh(sceneObj->getTransformedMesh());
                if (!mesh)
                    continue;
                combined->triangles.insert(combined->triangles.end(), mesh->triangles.begin(), mesh->triangles.end());
            }
            else
            {
                geometry::Mesh* mesh = sceneObj->getObject3D()->getMesh();
                if (!mesh)
                    continue;
                combined->triangles.insert(combined->triangles.end(), mesh->triangles.begin(), mesh->triangles.end());
            }
        }

        if (combined->triangles.empty())
            return nullptr;

        return combined;
    }

    void Scene::updateSelectionVisuals()
    {
        for (int i = 0; i < (int) _objects.size(); ++i)
        {
            const bool selected = _selectedObjectIndices.count(i) > 0;
            _objects[(size_t) i]->setColor(selected ? kSelectedColor : kDefaultColor);
        }
    }

    void Scene::notifySelectionChanged() const
    {
        splinter3D::events::ObjectSelectedEvent evt{};
        evt.index = _selectedObjectIndex;
        evt.indices.assign(_selectedObjectIndices.begin(), _selectedObjectIndices.end());

        splinter3D::events::EventBus::getInstance().publish(evt);
    }

    void Scene::splitSelected(float x, float y, float z)
    {
        for (int idx : _selectedObjectIndices)
        {
            std::cout << "Splitting object at index " << idx << " with grid size (" << x << ", " << y << ", " << z << ")\n";
            if (idx < 0 || idx >= (int) _objects.size())
            {
                std::cout << "Invalid index in splitSelected: " << idx << std::endl;
                continue;
            }
            std::unique_ptr<SceneObject>& selectedObj = _objects[(size_t) idx];
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
            _objects.erase(_objects.begin() + idx);
            for (auto& obj : newObjects)
            {
                _objects.push_back(std::make_unique<SceneObject>(obj)); // Add new objects to the scene
            }
        }
    }

} // namespace scene
