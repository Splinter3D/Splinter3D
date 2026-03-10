#pragma once

#include <Gui/CenteredToolbar.hpp>
#include <Input/Actions/IAction.hpp>
#include <Input/InputManager.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/Scene.hpp>

namespace input::actions
{
    class SelectObjectAction : public IAction
    {
      public:
        SelectObjectAction(renderer::IRenderer& renderer, gui::CenteredToolbar& toolbar)
            : renderer_(renderer), toolbar_(toolbar)
        { }

        void execute() override
        {
            if (toolbar_.isMouseOver(renderer_))
                return;

            const bool additive = InputManager::getInstance().isBindingActive(input::Action::MultiSelection, renderer_);
            auto       ray      = renderer_.getMouseRay();
            scene::Scene::getInstance().handleClick(ray, additive);
        }

      private:
        renderer::IRenderer&  renderer_;
        gui::CenteredToolbar& toolbar_;
    };
} // namespace input::actions
