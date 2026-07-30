#include "widget_events.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        void WidgetEvents::bind()
        {
            bindWidgetUpdates();
        }

        void WidgetEvents::bindWidgetUpdates()
        {
            frame_->Bind(
                wxEVT_UPDATE_UI,
                [this](wxUpdateUIEvent &event)
                {
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
