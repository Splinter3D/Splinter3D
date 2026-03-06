#include <Input/InputBindings.hpp>
#include <Input/InputManager.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/Scene.hpp>

namespace input
{
    void registerBindings() // TODO Does this need a class ??
    {
        auto& mgr = InputManager::getInstance();

        mgr.bind(
            Action::DeleteSelectedObject,
            {renderer::Key::Suppr},
            []() {
                scene::Scene::getInstance().removeSelected();
            });

        mgr.bind(
            Action::DuplicateSelectedObject,
            {renderer::Key::Ctrl, renderer::Key::D},
            []() {
                scene::Scene::getInstance().duplicateSelected();
            });
    }
} // namespace input