#pragma once

#include <wx/frame.h>

namespace ui::framework::wx
{
    class FloatingMenu : public wxFrame
    {
      public:
        FloatingMenu(wxWindow*       parent,
                     const wxString& title,
                     const wxSize&   size = wxDefaultSize);

        void ToggleVisibility();

      protected:
        void OnClose(wxCloseEvent& event);
    };
} // namespace ui::framework::wx