#include "demo_window.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registry.hpp"
#include "ui/menus/menu_registry.hpp"
#include "ui/panels/panel_registry.hpp"
#include "ui/toolbars/toolbar_registry.hpp"

namespace ui::windows
{
    DemoWindow::DemoWindow()
        : BaseWindow(nullptr, wxID_ANY, "Splinter3D - Widgets Demo",
                     wxDefaultPosition, wxSize(900, 600))
    {
        InitializeAll();
    }

    void DemoWindow::InitMenuBar()
    {
        using ui::menus::MenuRegistry;

        // Same registered menu as MainWindow - reused as-is.
        auto* menuBar  = new wxMenuBar();
        auto* fileMenu = MenuRegistry::getInstance().Create(ui::framework::wx::ids::menus::kExample);
        if (fileMenu != nullptr)
        {
            menuBar->Append(fileMenu, "&File");
        }
        SetMenuBar(menuBar);
    }

    void DemoWindow::InitLayout()
    {
        // Different layout id than MainWindow - this is the whole point:
        // swapping layouts costs one line, nothing else changes.
        layout_ = std::unique_ptr<ui::layouts::BaseLayout>(
            ui::layouts::LayoutRegistry::getInstance()
                .Create(ui::framework::wx::ids::layouts::kTwoColumn, this));
    }

    void DemoWindow::InitToolBars()
    {
        using ui::toolbars::ToolBarRegistry;

        // Same registered toolbar as MainWindow - reused as-is.
        auto* toolbar = ToolBarRegistry::getInstance().Create(
            ui::framework::wx::ids::toolbars::kExample, this);

        // TwoColumnLayout doesn't override AddTop(), so this call safely
        // resolves to BaseLayout's no-op - this layout simply has no top slot.
        if (toolbar != nullptr && layout_ != nullptr)
        {
            layout_->AddTop(toolbar);
        }
    }

    void DemoWindow::InitPanels()
    {
        using ui::panels::PanelRegistry;

        // Same registered panel as MainWindow's left slot - reused as-is.
        auto* widgets = PanelRegistry::getInstance().Create(
            ui::framework::wx::ids::panels::kWidgetsShowcase, this);

        if (widgets != nullptr && layout_ != nullptr)
        {
            layout_->AddLeft(widgets);
        }

        // Right column left empty too, for symmetry with MainWindow's center.

        Layout();
    }
} // namespace ui::windows
