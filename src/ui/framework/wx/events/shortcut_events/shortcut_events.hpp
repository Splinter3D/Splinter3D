#pragma once

#include "../event_binder.hpp"
#include "platform/settings/settings_registry.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Handles application keyboard shortcuts.
         *
         * Each shortcut is a KeyBind setting ("keys.save", default Ctrl+S, ...;
         * see kShortcuts) the user can change in the settings panel. The File
         * menu's items show the current ones.
         *
         * Shortcuts should trigger application actions rather than
         * containing business logic directly.
         *
         * While a text field has the focus, a key without Ctrl/Command or Alt
         * (other than F1-F24 and Escape) is left to it: Backspace there edits
         * the text.
         */
        class ShortcutEvents final : public EventBinder
        {
          public:
            /**
             * @param actions What the shortcuts that are not menu commands do,
             * by setting key ("keys.model_management", "keys.tasks_panel",
             * "keys.settings_panel", "keys.delete_model", "keys.hide_selection",
             * "keys.duplicate", "keys.deselect");
             * false when there was nothing to do: the key then goes on to the
             * focused control.
             */
            ShortcutEvents(wxFrame* frame, std::map<std::string, std::function<bool()>> actions)
                : EventBinder(frame), actions_(std::move(actions))
            {
            }

            /**
             * @brief Registers all keyboard shortcuts.
             */
            void bind() override;

          private:
            void onKey(wxKeyEvent& event);

            /** @brief Shows the current shortcuts on the menu items they trigger. */
            void syncMenus();

            std::map<std::string, std::function<bool()>>  actions_;
            std::vector<platform::settings::Subscription> subscriptions_;
        };

    } // namespace events
} // namespace ui::framework::wx
