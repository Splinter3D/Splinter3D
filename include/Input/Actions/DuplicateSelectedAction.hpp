#pragma once
#include <Input/Actions/IAction.hpp>
#include <Scene/Scene.hpp>

namespace input::actions
{
    struct DuplicateSelectedAction : IAction
    {
        void execute() override
        {
            scene::Scene::getInstance().duplicateSelected();
        }
    };
} // namespace input::actions
