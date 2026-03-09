#include <Input/Action.hpp>
#include <Input/InputManager.hpp>

namespace
{
    std::string mouseButtonToString(renderer::MouseButton button)
    {
        switch (button)
        {
            case renderer::MouseButton::Left:
                return "MouseLeft";
            case renderer::MouseButton::Right:
                return "MouseRight";
            case renderer::MouseButton::Middle:
                return "MouseMiddle";
            default:
                return "Mouse?";
        }
    }
} // namespace

namespace input
{
    void InputManager::registerAction(Action action, std::unique_ptr<input::actions::IAction> actionHandler)
    {
        _actions[action] = std::move(actionHandler);
    }

    void InputManager::bindKeys(const Action& action, std::vector<renderer::Key> keys,
                                std::optional<renderer::MouseButton> mouseButton)
    {
        Binding binding;
        binding.keys        = std::move(keys);
        binding.mouseButton = mouseButton;
        _keyBindings[action] = std::move(binding);
    }

    void InputManager::update(const renderer::IRenderer& renderer)
    {
        for (auto& [action, binding] : _keyBindings)
        {
            if (isBindingTriggered(binding, renderer) && _actions.count(action))
                _actions[action]->execute();
        }
    }

    void InputManager::trigger(const Action& action)
    {
        if (_actions.count(action))
            _actions[action]->execute();
    }

    bool InputManager::isBindingActive(const Action& action, const renderer::IRenderer& renderer) const
    {
        auto it = _keyBindings.find(action);
        if (it == _keyBindings.end())
            return false;
        return isBindingTriggered(it->second, renderer);
    }

    std::string InputManager::getActionKeyBindings(const Action& action) const
    {
        for (const auto& entry : _keyBindings)
            if (entry.first == action)
            {
                std::string result;
                for (size_t i = 0; i < entry.second.keys.size(); ++i)
                {
                    if (i > 0)
                        result += " + ";
                    result += renderer::stringFromKey(entry.second.keys[i]);
                }
                if (entry.second.mouseButton.has_value())
                {
                    if (!result.empty())
                        result += " + ";
                    result += mouseButtonToString(*entry.second.mouseButton);
                }
                return result;
            }
        return {};
    }

    bool InputManager::isBindingTriggered(const Binding& binding, const renderer::IRenderer& renderer) const
    {
        if (binding.mouseButton.has_value())
        {
            for (auto key : binding.keys)
                if (!renderer.isKeyDown(key))
                    return false;
            return renderer.isMouseButtonPressed(static_cast<int>(*binding.mouseButton));
        }

        if (binding.keys.empty())
            return false;

        for (size_t i = 0; i + 1 < binding.keys.size(); ++i)
            if (!renderer.isKeyDown(binding.keys[i]))
                return false;
        return renderer.isKeyPressed(binding.keys.back());
    }

} // namespace input
