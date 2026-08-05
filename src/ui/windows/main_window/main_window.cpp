#include "main_window.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/panels/panel_registry.hpp"

namespace ui::windows
{
    MainWindow::MainWindow()
        : BaseWindow(nullptr, wxID_ANY, "Splinter3D",
                     wxDefaultPosition, wxSize(1270, 720))
    {
        // MUST be the last line of the constructor: runs InitMenuBar(),
        // InitToolBars(), InitStatusBar(), InitLayout(), InitPanels(),
        // BindEvents() in that fixed order, dispatching to MainWindow's own
        // overrides.
        InitializeAll();
    }

    void MainWindow::InitStatusBar()
    {
        CreateStatusBar();
        SetStatusText("Splinter3D - ready");
    }

    void MainWindow::InitLayout()
    {
        // Runs before InitPanels() (see BaseWindow's guaranteed order), so the
        // sizer exists and is ready to receive whatever panels get created next.
        rootSizer_ = new wxBoxSizer(wxVERTICAL);
        SetSizer(rootSizer_);
    }

    void MainWindow::InitPanels()
    {
        using ui::panels::PanelRegistry;

        // MainWindow only asks the registry for the panel ids it cares about.
        // Other windows will ask for different ids from their own InitPanels().
        auto* example =
            PanelRegistry::getInstance().Create(ui::framework::wx::ids::panels::kExample, this);

        if (example != nullptr)
        {
            rootSizer_->Add(example, 1, wxEXPAND);
        }

        Layout();
    }
} // namespace ui::windows
