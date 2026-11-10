#pragma once

#include <wx/wx.h>

namespace ui::panels
{
    // Minimal panel used purely to validate the panel pipeline end-to-end:
    // PanelRegistry -> MainWindow::InitPanels() -> visible on screen.
    //
    // Safe to delete (and remove from panel_registration.cpp) once a first
    // real panel - ExplorerPanel, ViewerPanel, etc. - takes its place.
    class ExamplePanel : public wxPanel
    {
      public:
        explicit ExamplePanel(wxWindow* parent);
    };
} // namespace ui::panels
