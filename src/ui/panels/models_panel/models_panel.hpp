#pragma once

#include <functional>
#include <vector>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/panel.h>

namespace ui::panels
{
    /**
     * @brief Lists the models, one row each: name, visibility (eye) and delete
     * (trash) icons; double-clicking a row focuses that model. A side dock page.
     *
     * The rows show the viewer's selection, and clicking a name changes it:
     * a click selects just that model, Command/Ctrl-click adds or removes it,
     * Shift-click selects every shown row from the last one clicked (hidden
     * models are left out unless "Include hidden" is ticked, though a click
     * selects one); a click below the rows clears it. Below the list, Hide,
     * Delete, Show only and Duplicate act on the selected models (disabled
     * while none is); Show all shows every model.
     *
     * Rows are read through the actions, again on each RefreshModelRows().
     */
    class ModelsPanel final : public wxPanel
    {
      public:
        struct Actions
        {
            std::function<std::vector<wxString>()> names;
            std::function<std::vector<bool>()>     visibility;
            std::function<std::vector<int>()>      selected; // indices, as names()
            std::function<void(int, bool)>         setVisibility;
            std::function<void(int)>               remove;
            std::function<void(int)>               focus;
            std::function<void(std::vector<int>)>  select; // the selection becomes these
            std::function<void(int)>               toggleSelected;
            std::function<void(std::vector<int>)>  hide;       // these models
            std::function<void(std::vector<int>)>  removeMany; // these models
            std::function<void(std::vector<int>)>  showOnly;   // these models, hiding the others
            std::function<void(std::vector<int>)>  duplicate;  // these models
            std::function<void()>                  showAll;
        };

        ModelsPanel(wxWindow* parent, Actions actions);

        /** @brief Re-reads the models (after one is added, removed, hidden...). */
        void RefreshModelRows();
        /** @brief Highlights the selected rows (after the selection changed). */
        void ShowSelection();

      private:
        void ResizeColumns();
        void OnItemActivated(wxListEvent& event);
        void OnListLeftDown(wxMouseEvent& event);
        void OnListDoubleClick(wxMouseEvent& event);
        // The name clicked on row (-1: below the rows), as the modifiers say.
        void clickName(int row, const wxMouseEvent& event);

        wxListCtrl* list_ = nullptr;
        // Act on the selection, enabled while there is one.
        std::vector<wxButton*> selection_buttons_;
        Actions                actions_;
        int                    anchor_         = -1;    // where a Shift-click range starts: the last row clicked
        bool                   include_hidden_ = false; // in Shift-click ranges
    };
} // namespace ui::panels
