#pragma once

#include <wx/frame.h>

namespace ui::framework::wx
{
    namespace events
    {
        /**
         * @brief Base class for all event binders.
         *
         * An EventBinder is responsible for registering a coherent set of events
         * on a wxFrame.
         *
         * Examples include menu events, theme events, keyboard shortcuts,
         * language switching and window management.
         */
        class EventBinder
        {
          public:
            explicit EventBinder(wxFrame* frame)
                : frame_(frame)
            {
            }

            virtual ~EventBinder() = default;

            /**
             * @brief Registers all events handled by this binder.
             */
            virtual void bind() = 0;

          protected:
            wxFrame* frame_;
        };

    } // namespace events
} // namespace ui::framework::wx
