#include "base_window.hpp"

namespace ui::windows
{
    BaseWindow::BaseWindow(wxWindow*       parent,
                           wxWindowID      id,
                           const wxString& title,
                           const wxPoint&  pos,
                           const wxSize&   size,
                           long            style)
        : wxFrame(parent, id, title, pos, size, style)
    {
        // Intentionally empty: see the note on InitializeAll() in the header.
        // Hooks are NOT called here.
    }

    void BaseWindow::InitializeAll()
    {
        InitMenuBar();
        InitStatusBar();
        InitLayout();
        InitToolBars();
        InitPanels();
        BindEvents();
    }
} // namespace ui::windows
