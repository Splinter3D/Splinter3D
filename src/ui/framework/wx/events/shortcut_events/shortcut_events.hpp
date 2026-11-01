#pragma once

#include "../event_binder.hpp"

#include <functional>

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Handles application keyboard shortcuts.
         *
         * This event binder registers global keyboard shortcuts
         * such as Ctrl+S, Ctrl+O, F11, etc.
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
            /**
             * @brief Handles standard application shortcuts.
             */
            void bindCommonShortcuts();

            /**
             * @brief Handles fullscreen shortcut.
             */
            void bindFullscreenShortcut();

            std::function<void()> on_model_management_;
        };

    } // namespace events
} // namespace ui::framework::wx
