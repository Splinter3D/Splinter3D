#include "main_window.hpp"

namespace ui::windows
{
    MainWindow::MainWindow()
        : wxFrame(nullptr, wxID_ANY, "Splinter3D",
                  wxDefaultPosition, wxSize(1270, 720))
    {
        // Empty window for now.
        // Menus / toolbars / panels will be added here as they are built.
        // StatusBar may be removed or changed to a copyright note
        CreateStatusBar();
        SetStatusText("Splinter3D — ready");
    }
} // namespace ui::windows
