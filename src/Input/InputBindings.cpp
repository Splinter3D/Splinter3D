#include <Gui/Utils/FileDialog.hpp>
#include <Input/Actions/DeleteSelectedAction.hpp>
#include <Input/Actions/DuplicateSelectedAction.hpp>
#include <Input/Actions/ImportAction.hpp>
#include <Input/Actions/OpenPannelAction.hpp>
#include <Input/InputBindings.hpp>
#include <Input/InputManager.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/OpenPannelEvent.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

namespace input
{
    void registerBindings()
    {
        auto& mgr = InputManager::getInstance();

        // Link actions to their handlers
        mgr.registerAction(Action::DeleteSelectedObject, std::make_unique<actions::DeleteSelectedAction>());
        mgr.registerAction(Action::DuplicateSelectedObject, std::make_unique<actions::DuplicateSelectedAction>());
        mgr.registerAction(Action::Import, std::make_unique<actions::ImportAction>());
        mgr.registerAction(Action::OpenPannelExport, std::make_unique<actions::OpenPannelAction>("export"));
        mgr.registerAction(Action::OpenPannelRotation, std::make_unique<actions::OpenPannelAction>("rotation"));
        mgr.registerAction(Action::OpenPannelTransform, std::make_unique<actions::OpenPannelAction>("transform"));
        mgr.registerAction(Action::OpenPannelScale, std::make_unique<actions::OpenPannelAction>("scale"));
        mgr.registerAction(Action::OpenPannelCut, std::make_unique<actions::OpenPannelAction>("cut"));

        // Load key bindings from JSON
        std::ifstream file("assets/keybindings.json");
        if (!file.is_open())
            return;

        // Link actions to their keybinds
        nlohmann::json json = nlohmann::json::parse(file);
        for (const auto& binding : json["bindings"])
        {
            const input::Action        action = actionFromString(binding["action"]);
            std::vector<renderer::Key> keys;
            for (const auto& key : binding["keys"])
                keys.push_back(renderer::keyFromString(key));

            mgr.bindKeys(action, keys);
        }
        file.close();
    }

} // namespace input