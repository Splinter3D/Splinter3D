// Input/IAction.hpp
#pragma once
#include <Renderer/IRenderer.hpp>
#include <string>

namespace input::actions
{
    struct IAction
    {
        virtual ~IAction()     = default;
        virtual void execute() = 0;
    };
} // namespace input::actions
