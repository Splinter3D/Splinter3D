#include "widget_events.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        void WidgetEvents::bind()
        {
            bindWidgetUpdates();
            bindTransformControls();
        }

        void WidgetEvents::bindTransformControls()
        {
            for (auto* control : transform_controls_)
            {
                if (control == nullptr)
                    continue;

                control->Bind(
                    wxEVT_SPINCTRLDOUBLE,
                    [this](wxSpinDoubleEvent&) {
                        if (on_transform_changed_)
                            on_transform_changed_();
                    });
            }
        }

        void WidgetEvents::bindWidgetUpdates()
        {
            frame_->Bind(
                wxEVT_UPDATE_UI,
                [](wxUpdateUIEvent& event) {
                    /*
                     * Global UI update handling.
                     *
                     * Example:
                     * - enable/disable menu entries
                     * - update toolbar buttons
                     * - synchronize widgets state
                     */

                    event.Skip();
                });
        }

    } // namespace events
} // namespace ui::framework::wx
