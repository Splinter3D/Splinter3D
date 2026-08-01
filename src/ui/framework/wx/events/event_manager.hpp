#pragma once

#include "event_binder.hpp"

#include <memory>
#include <vector>

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Central manager responsible for registering application events.
         *
         * The EventManager owns all event binders and executes their registration.
         *
         * Each binder handles one specific category of events:
         *
         * - MenuEvents       : File/Edit/Help menus
         * - ThemeEvents      : Theme switching
         * - LocaleEvents     : Language switching
         * - ShortcutEvents   : Keyboard shortcuts
         * - WindowEvents     : Window lifecycle actions
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * wx::events::EventManager events(frame);
         * events.bindAll();
         * @endcode
         */
        class EventManager
        {
          public:
            /**
             * @brief Creates an event manager for a frame.
             *
             * @param frame Window receiving the wxWidgets events.
             */
            explicit EventManager(wxFrame* frame);

            /**
             * @brief Registers all application events.
             */
            void bindAll();

          private:
            std::vector<std::unique_ptr<EventBinder>> binders_;
        };

    } // namespace events
} // namespace ui::framework::wx
