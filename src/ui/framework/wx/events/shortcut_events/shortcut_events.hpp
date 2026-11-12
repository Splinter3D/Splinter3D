#pragma once

#include "../event_binder.hpp"
#include "platform/settings/settings_registry.hpp"

#include <functional>
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
         */
        class ShortcutEvents final : public EventBinder
        {
          public:
            ShortcutEvents(wxFrame* frame, std::function<void()> onModelManagement)
                : EventBinder(frame), on_model_management_(std::move(onModelManagement))
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

            std::function<void()>                         on_model_management_;
            std::vector<platform::settings::Subscription> subscriptions_;
        };

    } // namespace events
} // namespace ui::framework::wx
