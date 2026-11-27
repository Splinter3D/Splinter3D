#include "history_events.hpp"

#include "ui/framework/wx/ids/ids.hpp"

#include <wx/tglbtn.h>

namespace ui::framework::wx
{
    namespace events
    {
        void HistoryEvents::bind()
        {
            bindAction(ids::history::kUndo, on_undo_, can_undo_);
            bindAction(ids::history::kRedo, on_redo_, can_redo_);
        }

        void HistoryEvents::bindAction(int id, const Action& action, const Predicate& enabled)
        {
            // A shortcut while there is nothing to do still lands here: check again.
            const auto run = [&action, &enabled](wxCommandEvent&) {
                if (action && (!enabled || enabled()))
                    action();
            };
            frame_->Bind(wxEVT_MENU, run, id);         // menu item and shortcut
            frame_->Bind(wxEVT_TOGGLEBUTTON, run, id); // toolbar button (shaped like the toggles)

            // Menu items when the menu opens, buttons when idle.
            frame_->Bind(
                wxEVT_UPDATE_UI,
                [&enabled](wxUpdateUIEvent& event) { event.Enable(enabled && enabled()); },
                id);
        }

    } // namespace events
} // namespace ui::framework::wx
