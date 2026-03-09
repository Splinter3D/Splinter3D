#pragma once
#include <Gui/Utils/FileDialog.hpp>
#include <Input/Actions/IAction.hpp>
#include <Scene/Scene.hpp>

namespace input::actions
{
    struct ImportAction : IAction
    {
        void execute() override
        {
            auto path = gui::utils::pickSTLFile();
            if (path.has_value())
                scene::Scene::getInstance().addObject(*path);
        }
    };
} // namespace input::actions