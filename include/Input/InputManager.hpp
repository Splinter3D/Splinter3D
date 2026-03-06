#pragma once
#include <Input/Action.hpp>
#include <Renderer/IRenderer.hpp>
#include <Splinter3D/Utils/Singleton.hpp>
#include <functional>
#include <unordered_map>
#include <vector>

namespace input
{
    class InputManager : public splinter3D::utils::Singleton<InputManager>
    {
      public:
        using ActionFn = std::function<void()>;

        /**
         * Binds an action to a set of keys and a callback function. The callback will be called when the keys are pressed.
         */
        void bind(Action action, std::vector<renderer::Key> keys, ActionFn fn);

        /**
         * Updates the input manager and checks for key presses.
         */
        void update(const renderer::IRenderer& renderer);

        /**
         * Manually triggers an action.
         */
        void trigger(Action action);

        /**
         * Gets a string representation of the key bindings for a given action.
         */
        std::string getActionKeyBindings(Action action) const;

      protected:
        InputManager()  = default;
        ~InputManager() = default;

      private:
        struct Binding
        {
            Action                     action;
            std::vector<renderer::Key> keys;
            ActionFn                   fn;
        };

        std::vector<Binding> _bindings;

        friend class splinter3D::utils::Singleton<InputManager>;
    };
} // namespace input