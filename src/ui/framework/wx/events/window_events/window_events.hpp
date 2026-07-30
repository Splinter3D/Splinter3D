#pragma once

#include "../event_binder.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Handles application window related events.
         *
         * Responsible for:
         * - fullscreen management
         * - window closing
         * - window lifecycle events
         */
        class WindowEvents final : public EventBinder
        {
        public:
            using EventBinder::EventBinder;

            /**
             * @brief Registers all window event handlers.
             */
            void bind() override;

        private:
            void bindFullscreen();

            void bindClose();

            void bindResize();

            void toggleFullscreen(bool enabled);
        };

    } // namespace events
} // namespace ui::framework::wx
