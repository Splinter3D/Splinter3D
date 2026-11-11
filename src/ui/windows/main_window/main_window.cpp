#include "main_window.hpp"

#include "core/utils/data_root.hpp"
#include "ui/dialogs/model_management_dialog.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registry.hpp"
#include "ui/menus/menu_registry.hpp"
#include "ui/panels/panel_registry.hpp"
#include "ui/panels/tasks_panel/tasks_panel.hpp"
#include "ui/toolbars/toolbar_registry.hpp"

#include <filesystem>
#include <utility>
#include <wx/iconbndl.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

namespace ui::windows
{
    namespace
    {
        // Wide strings so non-ASCII install paths survive on Windows.
        std::filesystem::path findIcon(const char* filename)
        {
            const std::filesystem::path executable(
                wxStandardPaths::Get().GetExecutablePath().ToStdWstring());
            return core::utils::findAssetFile(executable, std::filesystem::path("icons") / filename);
        }

        // Window managers pick the closest size for the title bar, taskbar and
        // alt-tab, so provide the usual ones instead of a single huge bitmap.
        wxIconBundle makeIconBundle(const wxImage& source)
        {
            wxIconBundle bundle;
            for (const int size : {16, 24, 32, 48, 64, 128, 256})
            {
                wxIcon icon;
                icon.CopyFromBitmap(wxBitmap(source.Scale(size, size, wxIMAGE_QUALITY_HIGH)));
                bundle.AddIcon(icon);
            }
            return bundle;
        }
    } // namespace

    MainWindow::MainWindow()
        : BaseWindow(nullptr, wxID_ANY, "Splinter3D",
                     wxDefaultPosition, wxSize(1270, 720))
    {
        const auto iconPath = findIcon("logo_borderless.png");
        if (!iconPath.empty())
        {
            wxImage image(wxString(iconPath.wstring()), wxBITMAP_TYPE_PNG);
            if (image.IsOk())
                SetIcons(makeIconBundle(image));
            else
                wxLogWarning("Unable to load application icon: %s", wxString(iconPath.wstring()));
        }
        else
            wxLogWarning("Application icon was not found in assets/icons.");

        InitializeAll();
    }

    MainWindow::~MainWindow() = default;

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
            transform_toolbar_ = dynamic_cast<ui::toolbars::TransformToolbar*>(toolbar);
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

        // Right of the viewer; hidden until a task shows up.
        auto* tasksPanel = PanelRegistry::getInstance().Create(ui::framework::wx::ids::panels::kTasks, this);
        tasks_panel_     = dynamic_cast<ui::panels::TasksPanel*>(tasksPanel);
        if (tasks_panel_ != nullptr && layout_ != nullptr && model_viewer_ != nullptr)
        {
            layout_->AddRight(tasks_panel_);
            tasks_panel_->attach(model_viewer_->tasks());
        }

        if (model_viewer_ != nullptr)
        {
            model_management_menu_ = std::make_unique<ui::dialogs::ModelManagementDialog>(
                this,
                [this] { return model_viewer_->GetModelNames(); },
                [this] { return model_viewer_->GetModelVisibility(); },
                [this](int index, bool visible) {
                    model_viewer_->SetModelVisibility(index, visible);
                },
                [this](int index) { model_viewer_->RemoveModel(index); },
                [this](int index) { model_viewer_->FocusModel(index); });
        }

        Layout();
    }

    void MainWindow::BindEvents()
    {
        if (model_viewer_ != nullptr && transform_toolbar_ != nullptr)
        {
            model_viewer_->SetOnModelStateChanged([this](bool hasModel) {
                wxArrayString names;
                for (const auto& name : model_viewer_->GetModelNames())
                    names.Add(name);
                transform_toolbar_->SetTargets(names);
                transform_toolbar_->SetEnabled(hasModel);
                SyncTransformFields();
                UpdateGizmoState();
                if (model_management_menu_ != nullptr)
                    model_management_menu_->RefreshModelRows();
            });

            transform_toolbar_->SetOnTargetChanged([this](int) {
                SyncTransformFields();
                UpdateGizmoState();
            });

            transform_toolbar_->SetOnActiveToolChanged(
                [this](ui::toolbars::TransformToolbar::ActiveTool) { UpdateGizmoState(); });

            model_viewer_->SetOnGizmoChanged([this] { SyncTransformFields(); });
        }

        ui::framework::wx::events::EventManager::Callbacks callbacks;
        callbacks.onNew = [this] {
            if (model_viewer_ != nullptr)
                model_viewer_->newFile();
        };
        callbacks.onOpen = [this] {
            if (model_viewer_ != nullptr)
                model_viewer_->openFile();
        };
        callbacks.onSave = [this] {
            if (model_viewer_ != nullptr)
                model_viewer_->saveFile();
        };
        callbacks.onExit             = [this] { Close(); };
        callbacks.onModelManagement  = [this] { ShowModelManagement(); };
        callbacks.onTransformChanged = [this] { ApplyTransform(); };

        if (transform_toolbar_ != nullptr)
        {
            callbacks.transformControls = {
                transform_toolbar_->GetMoveX(),
                transform_toolbar_->GetMoveY(),
                transform_toolbar_->GetMoveZ(),
                transform_toolbar_->GetRotateX(),
                transform_toolbar_->GetRotateY(),
                transform_toolbar_->GetRotateZ()};
        }

        event_manager_ = std::make_unique<ui::framework::wx::events::EventManager>(
            this,
            std::move(callbacks));
        event_manager_->bindAll();

        Bind(wxEVT_CLOSE_WINDOW, &MainWindow::OnClose, this);
    }

    void MainWindow::OnClose(wxCloseEvent& event)
    {
        if (model_viewer_ != nullptr)
        {
            auto&             tasks   = model_viewer_->tasks();
            const std::size_t pending = tasks.pending();
            // A forced close (e.g. a signal) can't be refused: cancel without asking.
            if (pending > 0 && event.CanVeto())
            {
                bool quit = false;
                if (confirm_quit_)
                    quit = confirm_quit_(pending);
                else
                {
                    wxMessageDialog dialog(
                        this,
                        wxString::Format("%zu task(s) still running or queued. Cancel them and quit?", pending),
                        "Quit Splinter3D", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
                    dialog.SetYesNoLabels("Quit", "Keep working");
                    quit = dialog.ShowModal() == wxID_YES;
                }
                if (!quit)
                {
                    event.Veto();
                    return;
                }
            }
            tasks.cancelAllAndWait();
        }
        event.Skip();
    }

#if defined(SPLINTER3D_TESTING)
    void MainWindow::SetConfirmQuit(std::function<bool(std::size_t pending)> confirm)
    {
        confirm_quit_ = std::move(confirm);
    }
#endif

    void MainWindow::ShowModelManagement()
    {
        if (model_management_menu_ == nullptr)
            return;
        model_management_menu_->ToggleVisibility();
    }

    void MainWindow::ApplyTransform()
    {
        if (model_viewer_ == nullptr || transform_toolbar_ == nullptr)
            return;

        model_viewer_->setTransform(
            transform_toolbar_->GetTargetIndex(),
            transform_toolbar_->GetMoveX()->GetValue(),
            transform_toolbar_->GetMoveY()->GetValue(),
            transform_toolbar_->GetMoveZ()->GetValue(),
            transform_toolbar_->GetRotateX()->GetValue(),
            transform_toolbar_->GetRotateY()->GetValue(),
            transform_toolbar_->GetRotateZ()->GetValue());
    }

    void MainWindow::SyncTransformFields()
    {
        if (model_viewer_ == nullptr || transform_toolbar_ == nullptr)
            return;

        const auto transform = model_viewer_->GetTransform(transform_toolbar_->GetTargetIndex());
        transform_toolbar_->GetMoveX()->SetValue(transform.moveX);
        transform_toolbar_->GetMoveY()->SetValue(transform.moveY);
        transform_toolbar_->GetMoveZ()->SetValue(transform.moveZ);
        transform_toolbar_->GetRotateX()->SetValue(transform.rotateX);
        transform_toolbar_->GetRotateY()->SetValue(transform.rotateY);
        transform_toolbar_->GetRotateZ()->SetValue(transform.rotateZ);
    }

    void MainWindow::UpdateGizmoState()
    {
        if (model_viewer_ == nullptr || transform_toolbar_ == nullptr)
            return;

        using ui::framework::occt::ModelViewerPanel;
        using ui::toolbars::TransformToolbar;

        ModelViewerPanel::GizmoTool tool = ModelViewerPanel::GizmoTool::None;
        switch (transform_toolbar_->GetActiveTool())
        {
            case TransformToolbar::ActiveTool::Move:
                tool = ModelViewerPanel::GizmoTool::Move;
                break;
            case TransformToolbar::ActiveTool::Rotate:
                tool = ModelViewerPanel::GizmoTool::Rotate;
                break;
            case TransformToolbar::ActiveTool::None:
            default:
                break;
        }

        model_viewer_->SetGizmoState(transform_toolbar_->GetTargetIndex(), tool);
    }
} // namespace ui::windows
