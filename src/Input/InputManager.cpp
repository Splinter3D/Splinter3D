#include <Input/Action.hpp>
#include <Input/InputManager.hpp>

namespace input
{
    void InputManager::registerAction(Action action, std::unique_ptr<input::actions::IAction> actionHandler)
    {
        _actions[action] = std::move(actionHandler);
    }

    void InputManager::bindKeys(const Action& action, std::vector<renderer::Key> keys)
    {
        _keyBindings[action] = std::move(keys);
    }

    void InputManager::update(const renderer::IRenderer& renderer)
    {
        for (auto& [action, keys] : _keyBindings)
        {
            bool allDown = true;
            for (size_t i = 0; i < keys.size() - 1; ++i)
                if (!renderer.isKeyDown(keys[i]))
                    allDown = false;

            if (allDown && renderer.isKeyPressed(keys.back()))
                if (_actions.count(action))
                {
                    _actions[action]->execute();
                }
        }
    }

    void InputManager::trigger(const Action& action)
    {
        if (_actions.count(action))
            _actions[action]->execute();
    }

    std::string InputManager::getActionKeyBindings(const Action& action) const
    {
        for (const auto& b : _keyBindings)
            if (b.first == action)
            {
                std::string result;
                for (size_t i = 0; i < b.second.size(); ++i)
                {
                    if (i > 0)
                        result += " + ";
                    result += renderer::stringFromKey(b.second[i]);
                }
                return result;
            }
        return {};
    }

} // namespace input