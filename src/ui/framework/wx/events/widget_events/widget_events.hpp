#pragma once

#include "../event_binder.hpp"

#include <functional>
#include <utility>
#include <vector>
#include <wx/spinctrl.h>

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
            WidgetEvents(wxFrame*                       frame,
                         std::vector<wxSpinCtrlDouble*> transform_controls,
                         std::function<void()>          on_transform_changed)
                : EventBinder(frame), transform_controls_(std::move(transform_controls)), on_transform_changed_(std::move(on_transform_changed))
            {
            }

            /**
             * @brief Registers widget related events.
             */
            void bind() override;

          private:
            /**
             * @brief Handles generic widget updates.
             */
            void bindWidgetUpdates();
            void bindTransformControls();

            std::vector<wxSpinCtrlDouble*> transform_controls_;
            std::function<void()>          on_transform_changed_;
        };

    } // namespace events
} // namespace ui::framework::wx
