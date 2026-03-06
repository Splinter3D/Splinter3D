#include <Input/Actions.hpp>
#include <Input/InputManager.hpp>

namespace input
{
    void InputManager::bind(Action action, std::vector<renderer::Key> keys, ActionFn fn)
    {
        _bindings.push_back({action, std::move(keys), std::move(fn)});
    }

    void InputManager::update(const renderer::IRenderer& renderer)
    {
        for (auto& b : _bindings)
        {
            bool allDown = true;
            for (size_t i = 0; i < b.keys.size() - 1; ++i)
                if (!renderer.isKeyDown(b.keys[i]))
                    allDown = false;

            if (allDown && renderer.isKeyPressed(b.keys.back()))
                b.fn();
        }
    }

    void InputManager::trigger(Action action)
    {
        for (auto& b : _bindings)
            if (b.action == action)
            {
                b.fn();
                return;
            }
    }

    std::string InputManager::getActionKeyBindings(Action action) const
    {
        for (const auto& b : _bindings)
            if (b.action == action)
            {
                std::string result;
                for (size_t i = 0; i < b.keys.size(); ++i)
                {
                    if (i > 0)
                        result += " + ";
                    result += renderer::to_string(b.keys[i]);
                }
                return result;
            }
        return {};
    }

} // namespace input