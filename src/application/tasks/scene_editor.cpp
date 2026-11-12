#include "scene_editor.hpp"

#include <utility>

namespace application::tasks
{
    SceneServiceEditor::SceneServiceEditor(service::SceneService& scene)
        : _scene(scene)
    { }

    void SceneServiceEditor::addModel(service::ModelId              id,
                                      const std::string&            baseName,
                                      geometry::occt::Shape         shape,
                                      platform::benchmark::Duration loadDuration)
    {
        _scene.add(baseName, std::move(shape), id, loadDuration);
    }

    void SceneServiceEditor::removeModel(service::ModelId id)
    {
        _scene.removeById(id);
    }

    void SceneServiceEditor::setTransform(service::ModelId id, const geometry::occt::Transform& transform)
    {
        _scene.setTransformById(id, transform);
    }

    void SceneServiceEditor::setBRep(service::ModelId id, geometry::occt::Shape brep)
    {
        _scene.setBRep(id, std::move(brep));
    }

    void SceneServiceEditor::clear()
    {
        _scene.clear();
    }
} // namespace application::tasks
