#include "main_window.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registry.hpp"
#include "ui/menus/menu_registry.hpp"
#include "ui/panels/panel_registry.hpp"
#include "ui/toolbars/toolbar_registry.hpp"

namespace ui::windows
{
    MainWindow::MainWindow()
        : BaseWindow(nullptr, wxID_ANY, "Splinter3D",
                     wxDefaultPosition, wxSize(1270, 720))
    {
        InitializeAll();
    }

    void MainWindow::InitMenuBar()
    {
        using ui::menus::MenuRegistry;

        auto* menuBar = new wxMenuBar();
        auto* fileMenu =
            MenuRegistry::getInstance().Create(ui::framework::wx::ids::menus::kExample);
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
        // Must run before InitToolBars()/InitPanels() so layout_ already
        // exists when those hooks want to place something into it.
        layout_ = std::unique_ptr<ui::layouts::BaseLayout>(
            ui::layouts::LayoutRegistry::getInstance()
                .Create(ui::framework::wx::ids::layouts::kMain, this));
    }

    void MainWindow::InitToolBars()
    {
        using ui::toolbars::ToolBarRegistry;

        auto* toolbar = ToolBarRegistry::getInstance().Create(
            ui::framework::wx::ids::toolbars::kExample, this);

        if (toolbar != nullptr && layout_ != nullptr)
        {
            layout_->AddTop(toolbar);
        }
    }

    void MainWindow::InitPanels()
    {
        using ui::panels::PanelRegistry;

        // MainWindow only asks the registry for the panel ids it cares about.
        // Other windows will ask for different ids from their own InitPanels().
        auto* widgets = PanelRegistry::getInstance().Create(
            ui::framework::wx::ids::panels::kWidgetsShowcase, this);

        if (widgets != nullptr && layout_ != nullptr)
        {
            layout_->AddLeft(widgets);
        }

        // Center intentionally left empty: reserved for the future
        // OCCT/OpenGL viewer. Nothing is created or placed here yet.

        Layout();
    }
} // namespace ui::windows
