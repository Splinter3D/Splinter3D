#include "main_window.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registry.hpp"
#include "ui/menus/menu_registry.hpp"
#include "ui/panels/panel_registry.hpp"
#include "ui/toolbars/toolbar_registry.hpp"

#include <filesystem>
#include <vector>
#include <wx/image.h>
#include <wx/stdpaths.h>

namespace ui::windows
{
    namespace
    {
        std::filesystem::path findIcon(const char* filename)
        {
            const auto executable = std::filesystem::path(
                wxStandardPaths::Get().GetExecutablePath().ToStdString());
            const std::vector<std::filesystem::path> roots{
                std::filesystem::current_path(), executable.parent_path(),
                executable.parent_path().parent_path(),
                executable.parent_path().parent_path().parent_path(),
                executable.parent_path().parent_path().parent_path().parent_path()};

            for (const auto& root : roots)
            {
                const auto candidate = root / "assets" / "icons" / filename;
                if (std::filesystem::exists(candidate))
                    return candidate;
            }
            return {};
        }
    } // namespace

    MainWindow::MainWindow()
        : BaseWindow(nullptr, wxID_ANY, "Splinter3D",
                     wxDefaultPosition, wxSize(1270, 720))
    {
        const auto iconPath = findIcon("logo_borderless.png");
        if (!iconPath.empty())
        {
            wxImage image(iconPath.string(), wxBITMAP_TYPE_PNG);
            if (image.IsOk())
            {
                wxIcon icon;
                icon.CopyFromBitmap(wxBitmap(image));
                SetIcon(icon);
            }
            else
            {
                wxLogWarning("Unable to load application icon: %s", iconPath.string());
            }
        }
        else
            wxLogWarning("Application icon was not found in assets/icons.");

        InitializeAll();
    }

    void MainWindow::InitMenuBarEntry(wxMenuBar* menuBar, const int menuId, const wxString& title)
    {
        using ui::menus::MenuRegistry;

        auto* menu = MenuRegistry::getInstance().Create(menuId);
        if (menu != nullptr)
        {
            menuBar->Append(menu, title);
        }
    }

    void MainWindow::InitMenuBar()
    {
        using ui::menus::MenuRegistry;

        auto* menuBar = new wxMenuBar();

        InitMenuBarEntry(menuBar, ui::framework::wx::ids::menus::kFile, "&File");

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
            ui::framework::wx::ids::toolbars::kTransform, this);

        if (toolbar != nullptr && layout_ != nullptr)
        {
            layout_->AddTop(toolbar);
        }
    }

    void MainWindow::InitPanels()
    {
        using ui::panels::PanelRegistry;

        auto* modelViewer = PanelRegistry::getInstance().Create(
            ui::framework::wx::ids::panels::kModelViewer, this);

        if (modelViewer != nullptr && layout_ != nullptr)
        {
            layout_->AddCenter(modelViewer);
            model_viewer_ = dynamic_cast<ui::framework::occt::ModelViewerPanel*>(modelViewer);
        }

        Layout();
    }

    void MainWindow::BindEvents()
    {
        Bind(
            wxEVT_MENU, [this](wxCommandEvent&) {
                 if (model_viewer_ != nullptr)
                     model_viewer_->newFile(); }, ui::framework::wx::ids::file::kNew);

        Bind(
            wxEVT_MENU, [this](wxCommandEvent&) {
                 if (model_viewer_ != nullptr)
                     model_viewer_->openFile(); }, ui::framework::wx::ids::file::kOpen);

        Bind(
            wxEVT_MENU, [this](wxCommandEvent&) {
                 if (model_viewer_ != nullptr)
                     model_viewer_->saveFile(); }, ui::framework::wx::ids::file::kSave);

        Bind(
            wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, ui::framework::wx::ids::file::kExit);

        Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
            if (event.ControlDown())
            {
                const int key       = event.GetKeyCode();
                const int commandId = key == 'N'   ? ui::framework::wx::ids::file::kNew
                                      : key == 'O' ? ui::framework::wx::ids::file::kOpen
                                      : key == 'S' ? ui::framework::wx::ids::file::kSave
                                      : key == 'Q' ? ui::framework::wx::ids::file::kExit
                                                   : wxID_NONE;
                if (commandId != wxID_NONE)
                {
                    wxCommandEvent command(wxEVT_MENU, commandId);
                    ProcessWindowEvent(command);
                    return;
                }
            }

            if (event.GetKeyCode() == WXK_F11)
            {
                ShowFullScreen(!IsFullScreen());
                return;
            }

            event.Skip();
        });
    }
} // namespace ui::windows
