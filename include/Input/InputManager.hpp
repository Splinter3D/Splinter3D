#pragma once
#include <Input/Action.hpp>
#include <Input/Actions/IAction.hpp>
#include <Renderer/IRenderer.hpp>
#include <Splinter3D/Utils/Singleton.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace input
{
    class InputManager : public splinter3D::utils::Singleton<InputManager>
    {
      public:
        void registerAction(Action action, std::unique_ptr<input::actions::IAction> actionHandler);

        void bindKeys(const Action& action, std::vector<renderer::Key> keys,
                      std::optional<renderer::MouseButton> mouseButton = std::nullopt);
        void update(const renderer::IRenderer& renderer);
        void trigger(const Action& action);
        bool isBindingActive(const Action& action, const renderer::IRenderer& renderer) const;

        /**
         * Gets a string representation of the key bindings for a given action.
         */
        std::string getActionKeyBindings(const Action& action) const;

      protected:
        InputManager()  = default;
        ~InputManager() = default;

      private:
        struct Binding
        {
            std::vector<renderer::Key>            keys;
            std::optional<renderer::MouseButton>  mouseButton;
        };

        bool isBindingTriggered(const Binding& binding, const renderer::IRenderer& renderer) const;

        std::unordered_map<Action, std::unique_ptr<input::actions::IAction>> _actions;
        std::unordered_map<Action, Binding>                                  _keyBindings;

        friend class splinter3D::utils::Singleton<InputManager>;
    };
} // namespace input
