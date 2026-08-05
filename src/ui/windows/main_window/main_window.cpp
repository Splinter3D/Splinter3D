#include "main_window.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/menus/menu_registry.hpp"
#include "ui/panels/panel_registry.hpp"
#include "ui/toolbars/toolbar_registry.hpp"

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

    void MainWindow::InitMenuBar()
    {
        using ui::menus::MenuRegistry;

        auto* menuBar  = new wxMenuBar();
        auto* fileMenu = MenuRegistry::getInstance().Create(ui::framework::wx::ids::menus::kExample);
        if (fileMenu != nullptr)
        {
            menuBar->Append(fileMenu, "&File");
        }
        SetMenuBar(menuBar);
    }

    void MainWindow::InitStatusBar()
    {
        CreateStatusBar();
        SetStatusText("Splinter3D - ready");
    }

    void MainWindow::InitLayout()
    {
        // Runs before InitToolBars()/InitPanels() (see BaseWindow's
        // guaranteed order), so the sizer exists and is ready to receive
        // whatever gets created next.
        rootSizer_ = new wxBoxSizer(wxVERTICAL);
        SetSizer(rootSizer_);
    }

    void MainWindow::InitToolBars()
    {
        using ui::toolbars::ToolBarRegistry;

        // MainWindow only asks for the toolbar ids it cares about - same
        // "pick what you need" logic as InitPanels() below.
        auto* toolbar = ToolBarRegistry::getInstance().Create(
            ui::framework::wx::ids::toolbars::kExample, this);

        if (toolbar != nullptr)
        {
            rootSizer_->Add(toolbar, 0, wxEXPAND);
        }
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
