#include <Gui/Utils/FileDialog.hpp>
#include <Input/InputBindings.hpp>
#include <Input/InputManager.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/OpenPannelEvent.hpp>

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

        mgr.bind(
            Action::Import,
            {renderer::Key::Ctrl, renderer::Key::I},
            []() {
                auto path = gui::utils::pickSTLFile();
                if (path.has_value())
                    scene::Scene::getInstance().addObject(*path);
            });

        mgr.bind(Action::Rotation,
                 {renderer::Key::Ctrl, renderer::Key::R},
                 []() {
                     splinter3D::events::EventBus::getInstance()
                         .publish(splinter3D::events::OpenPannelEvent{"rotation"});
                 });

        mgr.bind(Action::Transform,
                 {renderer::Key::Ctrl, renderer::Key::T},
                 []() {
                     splinter3D::events::EventBus::getInstance()
                         .publish(splinter3D::events::OpenPannelEvent{"transform"});
                 });

        mgr.bind(Action::Scale,
                 {renderer::Key::Ctrl, renderer::Key::S},
                 []() {
                     splinter3D::events::EventBus::getInstance()
                         .publish(splinter3D::events::OpenPannelEvent{"scale"});
                 });
    }
} // namespace input