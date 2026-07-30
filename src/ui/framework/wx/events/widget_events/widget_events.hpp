#pragma once

#include "../event_binder.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Handles global widget related events.
         *
         * This binder manages events coming from UI controls
         * that are not handled directly by individual widgets.
         */
        class WidgetEvents final : public EventBinder
        {
        public:
            using EventBinder::EventBinder;

            /**
             * @brief Registers widget related events.
             */
            void bind() override;

        private:
            /**
             * @brief Handles generic widget updates.
             */
            void bindWidgetUpdates();
        };

    } // namespace events
} // namespace ui::framework::wx
