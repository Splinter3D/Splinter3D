#include <Gui/CenteredToolbar.hpp>
#include <Gui/Utils/FileDialog.hpp>
#include <Input/Actions/DeleteSelectedAction.hpp>
#include <Input/Actions/DuplicateSelectedAction.hpp>
#include <Input/Actions/ImportAction.hpp>
#include <Input/Actions/OpenPannelAction.hpp>
#include <Input/Actions/SelectObjectAction.hpp>
#include <Input/InputBindings.hpp>
#include <Input/InputManager.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/OpenPannelEvent.hpp>
#include <fstream>
#include <optional>
#include <nlohmann/json.hpp>

namespace
{
    std::optional<renderer::MouseButton> mouseButtonFromString(const std::string& str)
    {
        if (str == "Left" || str == "MouseLeft")
            return renderer::MouseButton::Left;
        if (str == "Right" || str == "MouseRight")
            return renderer::MouseButton::Right;
        if (str == "Middle" || str == "MouseMiddle")
            return renderer::MouseButton::Middle;
        return std::nullopt;
    }
} // namespace

namespace input
{
    void registerBindings(renderer::IRenderer& renderer, gui::CenteredToolbar& toolbar)
    {
        auto& mgr = InputManager::getInstance();

        // Link actions to their handlers
        mgr.registerAction(Action::DeleteSelectedObject, std::make_unique<actions::DeleteSelectedAction>());
        mgr.registerAction(Action::DuplicateSelectedObject, std::make_unique<actions::DuplicateSelectedAction>());
        mgr.registerAction(Action::Import, std::make_unique<actions::ImportAction>());
        mgr.registerAction(Action::OpenExportPannel, std::make_unique<actions::OpenPannelAction>("export"));
        mgr.registerAction(Action::OpenPannelRotation, std::make_unique<actions::OpenPannelAction>("rotation"));
        mgr.registerAction(Action::OpenPannelTransform, std::make_unique<actions::OpenPannelAction>("transform"));
        mgr.registerAction(Action::OpenPannelScale, std::make_unique<actions::OpenPannelAction>("scale"));
        mgr.registerAction(Action::SelectObject, std::make_unique<actions::SelectObjectAction>(renderer, toolbar));

        // Load key bindings from JSON
        std::ifstream file("assets/keybindings.json");
        if (!file.is_open())
            return;

        // Link actions to their keybinds
        nlohmann::json json = nlohmann::json::parse(file);
        for (const auto& binding : json["bindings"])
        {
            const input::Action        action = actionFromString(binding["action"]);
            if (action == Action::Unknown)
                continue;
            std::vector<renderer::Key> keys;
            for (const auto& key : binding["keys"])
                keys.push_back(renderer::keyFromString(key));

            std::optional<renderer::MouseButton> mouseButton;
            if (binding.contains("mouse"))
                mouseButton = mouseButtonFromString(binding["mouse"].get<std::string>());

            mgr.bindKeys(action, keys, mouseButton);
        }
        file.close();
    }

} // namespace input
