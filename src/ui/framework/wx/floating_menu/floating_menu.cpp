#include "floating_menu.hpp"

namespace ui::framework::wx
{
    FloatingMenu::FloatingMenu(wxWindow* parent, const wxString& title, const wxSize& size)
        : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, size,
                  wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER | wxFRAME_FLOAT_ON_PARENT |
                      wxFRAME_TOOL_WINDOW)
    {
        Bind(wxEVT_CLOSE_WINDOW, &FloatingMenu::OnClose, this);
    }

    void FloatingMenu::ToggleVisibility()
    {
        if (IsShown())
            Hide();
        else
        {
            Show();
            Raise();
        }
    }

    void FloatingMenu::OnClose(wxCloseEvent& event)
    {
        Hide();
        event.Skip(false);
    }
} // namespace ui::framework::wx