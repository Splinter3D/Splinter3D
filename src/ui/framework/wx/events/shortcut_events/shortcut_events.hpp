#pragma once

#include "../event_binder.hpp"

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
            using EventBinder::EventBinder;

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
        };

    } // namespace events
} // namespace ui::framework::wx
