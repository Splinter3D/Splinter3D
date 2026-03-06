#pragma once
#include <Input/Actions/IAction.hpp>
#include <Scene/Scene.hpp>

namespace input::actions
{
    struct DeleteSelectedAction : IAction
    {
        void execute() override
        {
            scene::Scene::getInstance().removeSelected();
        }
    };
} // namespace input::actions