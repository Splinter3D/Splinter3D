#pragma once
#include <Input/Actions.hpp>
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

        void bind(Action action, std::vector<renderer::Key> keys, ActionFn fn);

        void update(const renderer::IRenderer& renderer);

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