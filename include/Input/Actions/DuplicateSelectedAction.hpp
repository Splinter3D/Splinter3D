#pragma once
#include <Input/Actions/IAction.hpp>
#include <Scene/Scene.hpp>

namespace input::actions
{
    struct DuplicateSelectedAction : IAction
    {
        void execute() override
        {
            scene::Scene::getInstance().splitSelected(0.5f, 0.5f, 0.5f);
        }
    };
} // namespace input::actions