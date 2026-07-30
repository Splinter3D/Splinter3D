#pragma once

#include "../event_binder.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Handles application theme related events.
         *
         * This binder registers menu events related to theme management:
         * - Toggle dark mode
         * - Force light mode
         * - Force dark mode
         *
         * Theme state persistence and application are delegated
         * to the theme framework.
         */
        class ThemeEvents final : public EventBinder
        {
        public:
            using EventBinder::EventBinder;

            /**
             * @brief Registers all theme event handlers.
             */
            void bind() override;

        private:
            /**
             * @brief Registers the dark mode toggle event.
             */
            void bindToggleDarkMode();

            /**
             * @brief Registers explicit light/dark mode events.
             */
            void bindThemeModes();

            /**
             * @brief Applies the current theme state.
             *
             * @param dark True to enable dark mode, false for light mode.
             */
            void applyTheme(bool dark);
        };

    } // namespace events
} // namespace ui::framework::wx
