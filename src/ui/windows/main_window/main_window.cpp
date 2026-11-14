#include "main_window.hpp"

#include "core/utils/data_root.hpp"
#include "core/utils/version.hpp"
#include "platform/locale/locale_manager.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/framework/wx/locale/locale.hpp"
#include "ui/icons/Icons.hpp"
#include "ui/layouts/layout_registry.hpp"
#include "ui/menus/menu_registry.hpp"
#include "ui/panels/models_panel/models_panel.hpp"
#include "ui/panels/panel_registry.hpp"
#include "ui/panels/settings_panel/settings_panel.hpp"
#include "ui/panels/side_dock/side_dock.hpp"
#include "ui/panels/tasks_panel/tasks_panel.hpp"
#include "ui/toolbars/toolbar_registry.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <wx/app.h>
#include <wx/iconbndl.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>
#include <wx/stdpaths.h>

using ui::framework::wx::locale::tr;

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

        InitMenuBarEntry(menuBar, ui::framework::wx::ids::menus::kFile, tr("menu.file.title"));

        SetMenuBar(menuBar);
    }

    namespace
    {
        // The most copies one duplication makes (each is a model to draw).
        constexpr int kMaxCopies = 100;

        wxString Version()
        {
            wxString version = wxString::FromUTF8(std::string(core::utils::kVersion));
            if constexpr (!core::utils::kReleaseBuild)
                version += " " + tr("window.status.dev_build");
            return version;
        }
    } // namespace

    void MainWindow::InitStatusBar()
    {
        CreateStatusBar();
        SetStatusText(tr("window.status.ready", {{"version", Version()}}));
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

        // Right of the viewer: a strip of buttons opening side pages.
        side_dock_ = dynamic_cast<ui::panels::SideDock*>(
            PanelRegistry::getInstance().Create(ui::framework::wx::ids::panels::kSideDock, this));
        if (side_dock_ != nullptr && layout_ != nullptr)
            layout_->AddRight(side_dock_);

        if (side_dock_ != nullptr && model_viewer_ != nullptr)
        {
            ui::panels::ModelsPanel::Actions actions;
            actions.names          = [this] { return model_viewer_->GetModelNames(); };
            actions.visibility     = [this] { return model_viewer_->GetModelVisibility(); };
            actions.selected       = [this] { return model_viewer_->GetSelectedIndices(); };
            actions.setVisibility  = [this](int index, bool visible) { model_viewer_->SetModelVisibility(index, visible); };
            actions.remove         = [this](int index) { DeleteModels({index}); };
            actions.focus          = [this](int index) { model_viewer_->FocusModel(index); };
            actions.select         = [this](std::vector<int> indices) { model_viewer_->Select(indices); };
            actions.toggleSelected = [this](int index) { model_viewer_->ToggleSelected(index); };
            actions.hide           = [this](std::vector<int> indices) { HideModels(indices); };
            actions.removeMany     = [this](std::vector<int> indices) { DeleteModels(std::move(indices)); };
            actions.showOnly       = [this](std::vector<int> indices) { ShowOnlyModels(indices); };
            actions.duplicate      = [this](std::vector<int> indices) { DuplicateModels(indices); };
            actions.showAll        = [this] { ShowAllModels(); };
            models_panel_          = new ui::panels::ModelsPanel(side_dock_, std::move(actions));
            side_dock_->AddPage(models_panel_, ui::icons::MakeIcon(ui::icons::modelsSvg), tr("models.title"));
        }

        // Opens by itself while tasks run (unless turned off in the settings),
        // unless already opened by hand.
        platform::settings::SettingsRegistry::getInstance().add(
            {kTasksAutoOpenSetting, "settings.section.general", "settings.tasks.auto_open", "", platform::settings::Kind::Toggle, "1", {}});
        platform::settings::SettingsRegistry::getInstance().add(
            {kConfirmDeleteSetting, "settings.section.general", "settings.models.confirm_delete", "", platform::settings::Kind::Toggle, "1", {}});
        if (side_dock_ != nullptr && model_viewer_ != nullptr)
        {
            auto* tasksPanel = PanelRegistry::getInstance().Create(ui::framework::wx::ids::panels::kTasks, side_dock_);
            tasks_panel_     = dynamic_cast<ui::panels::TasksPanel*>(tasksPanel);
            if (tasks_panel_ != nullptr)
            {
                side_dock_->AddPage(tasks_panel_, ui::icons::MakeIcon(ui::icons::tasksSvg), tr("tasks.title"));
                tasks_panel_->SetOnBusyChanged([this](bool busy) {
                    if (busy && platform::settings::SettingsRegistry::getInstance().isOn(kTasksAutoOpenSetting))
                        side_dock_->RequestOpen(tasks_panel_);
                    else
                        side_dock_->ReleaseOpen(tasks_panel_);
                });
                tasks_panel_->attach(model_viewer_->tasks());
            }
        }

        if (side_dock_ != nullptr)
        {
            settings_panel_ = dynamic_cast<ui::panels::SettingsPanel*>(
                PanelRegistry::getInstance().Create(ui::framework::wx::ids::panels::kSettings, side_dock_));
            if (settings_panel_ != nullptr)
                side_dock_->AddPage(settings_panel_, ui::icons::MakeIcon(ui::icons::settingsSvg), tr("settings.title"));
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
                SyncTargetToSelection();
                transform_toolbar_->SetEnabled(hasModel);
                if (hasModel && reopen_tool_ != ui::toolbars::TransformToolbar::ActiveTool::None)
                    transform_toolbar_->OpenTool(std::exchange(reopen_tool_, ui::toolbars::TransformToolbar::ActiveTool::None));
                SyncTransformFields();
                UpdateGizmoState();
                if (models_panel_ != nullptr)
                    models_panel_->RefreshModelRows();
            });

            // The dropdown and the viewport selection are one and the same.
            transform_toolbar_->SetOnTargetChanged([this](int index) {
                if (index >= 0)
                {
                    model_viewer_->SelectOnly(index);
                    SyncTargetToSelection();
                }
                else if (index == application::service::SceneService::kAllModels)
                    model_viewer_->ClearSelection();
                SyncTransformFields();
                UpdateGizmoState();
            });
            model_viewer_->SetOnSelectionChanged([this] {
                if (models_panel_ != nullptr)
                    models_panel_->ShowSelection();
                SyncTargetToSelection();
                SyncTransformFields();
                UpdateGizmoState();
            });
            model_viewer_->SetOnBackgroundClicked([this] { transform_toolbar_->CloseActiveTool(); });

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
        callbacks.onExit            = [this] { Close(); };
        callbacks.onModelsPanel     = [this] { ToggleDockPage(models_panel_); };
        callbacks.onTasksPanel      = [this] { ToggleDockPage(tasks_panel_); };
        callbacks.onSettingsPanel   = [this] { ToggleDockPage(settings_panel_); };
        callbacks.onDeleteSelection = [this] {
            if (model_viewer_ == nullptr || model_viewer_->GetSelectedIndices().empty())
                return false;
            DeleteModels(model_viewer_->GetSelectedIndices());
            return true;
        };
        callbacks.onHideSelection = [this] {
            if (model_viewer_ == nullptr || model_viewer_->GetSelectedIndices().empty())
                return false;
            HideModels(model_viewer_->GetSelectedIndices());
            return true;
        };
        callbacks.onDuplicateSelection = [this] {
            if (model_viewer_ == nullptr || model_viewer_->GetSelectedIndices().empty())
                return false;
            DuplicateModels(model_viewer_->GetSelectedIndices());
            return true;
        };
        callbacks.onDeselect = [this] {
            const bool toolOpen = transform_toolbar_ != nullptr &&
                                  transform_toolbar_->GetActiveTool() != ui::toolbars::TransformToolbar::ActiveTool::None;
            const bool selected = model_viewer_ != nullptr && !model_viewer_->GetSelectedIndices().empty();
            if (toolOpen)
                transform_toolbar_->CloseActiveTool();
            if (selected)
                model_viewer_->ClearSelection();
            return toolOpen || selected;
        };
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

        language_subscription_ = platform::settings::SettingsRegistry::getInstance().subscribe(
            platform::locale::kLanguageSetting, [this](const std::string&) { CallAfter([this] { OfferReload(); }); });
    }

    void MainWindow::OfferReload()
    {
        using platform::locale::LocaleManager;
        const std::string language =
            LocaleManager::resolve(platform::settings::SettingsRegistry::getInstance().get(platform::locale::kLanguageSetting));
        if (language == LocaleManager::get().getLanguage())
            return;

        const std::size_t pending = model_viewer_ != nullptr ? model_viewer_->tasks().pending() : 0;
        if (confirm_reload_)
        {
            if (confirm_reload_(pending) && pending == 0)
                Reload();
            return;
        }
        if (pending > 0)
        {
            wxMessageDialog dialog(this, tr("window.language.busy", {{"count", wxString::Format("%zu", pending)}}),
                                   tr("window.language.title"), wxOK | wxICON_INFORMATION);
            dialog.ShowModal();
            return;
        }
        wxMessageDialog dialog(this, tr("window.language.message"), tr("window.language.title"),
                               wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);
        dialog.SetYesNoLabels(tr("window.language.reload"), tr("window.language.later"));
        if (dialog.ShowModal() == wxID_YES)
            Reload();
    }

    void MainWindow::Reload()
    {
        using platform::locale::LocaleManager;
        LocaleManager::get().setLanguage(
            LocaleManager::resolve(platform::settings::SettingsRegistry::getInstance().get(platform::locale::kLanguageSetting)));

        auto* reloaded = new MainWindow();
        reloaded->SetSize(GetRect());
        if (IsMaximized())
            reloaded->Maximize();
        if (transform_toolbar_ != nullptr)
            reloaded->reopen_tool_ = transform_toolbar_->GetActiveTool();
        if (model_viewer_ != nullptr && reloaded->model_viewer_ != nullptr)
            reloaded->model_viewer_->AdoptModels(model_viewer_->SnapshotModels());
        reloaded->Show();
        if (IsFullScreen())
            reloaded->ShowFullScreen(true);

        if (side_dock_ != nullptr && reloaded->side_dock_ != nullptr)
        {
            if (side_dock_->IsClosedByHand())
                reloaded->side_dock_->Close();
            wxWindow* const page = side_dock_->CurrentPage();
            if (page != nullptr && page == settings_panel_ && reloaded->settings_panel_ != nullptr)
            {
                reloaded->side_dock_->Open(reloaded->settings_panel_);
                reloaded->CallAfter([reloaded, start = settings_panel_->GetViewStart()] {
                    reloaded->settings_panel_->Scroll(start);
                });
            }
            else if (page != nullptr && page == tasks_panel_ && side_dock_->IsOpenedManually() && reloaded->tasks_panel_ != nullptr)
                reloaded->side_dock_->Open(reloaded->tasks_panel_);
            else if (page != nullptr && page == models_panel_ && reloaded->models_panel_ != nullptr)
                reloaded->side_dock_->Open(reloaded->models_panel_);
        }
        wxTheApp->SetTopWindow(reloaded);
        Destroy();
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
                        this, tr("window.quit.message", {{"count", wxString::Format("%zu", pending)}}),
                        tr("window.quit.title"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
                    dialog.SetYesNoLabels(tr("window.quit.confirm"), tr("window.quit.keep"));
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

    void MainWindow::SetConfirmDelete(std::function<bool(const wxString& title, const wxString& message)> confirm)
    {
        confirm_delete_ = std::move(confirm);
    }

    void MainWindow::SetConfirmReload(std::function<bool(std::size_t pending)> confirm)
    {
        confirm_reload_ = std::move(confirm);
    }

    void MainWindow::SetAskCopies(std::function<std::optional<int>(std::size_t models)> ask)
    {
        ask_copies_ = std::move(ask);
    }
#endif

    void MainWindow::DeleteModels(std::vector<int> indices)
    {
        if (model_viewer_ == nullptr)
            return;
        const auto names = model_viewer_->GetModelNames();
        std::erase_if(indices, [&](int index) { return index < 0 || static_cast<std::size_t>(index) >= names.size(); });
        std::sort(indices.begin(), indices.end());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
        if (indices.empty())
            return;

        // By id: the dialog runs events, and a removal landing meanwhile
        // shifts the indices.
        std::vector<application::service::ModelId> ids;
        const auto                                 allIds = model_viewer_->GetModelIds();
        for (const int index : indices)
            ids.push_back(allIds[static_cast<std::size_t>(index)]);

        if (platform::settings::SettingsRegistry::getInstance().isOn(kConfirmDeleteSetting))
        {
            const wxString message =
                indices.size() == 1
                    ? tr("dialogs.delete_model.one", {{"name", names[static_cast<std::size_t>(indices.front())]}})
                    : tr("dialogs.delete_model.many", {{"count", wxString::Format("%zu", indices.size())}});
            const wxString title     = tr(indices.size() == 1 ? "dialogs.delete_model.title_one" : "dialogs.delete_model.title_many");
            bool           confirmed = false;
            if (confirm_delete_)
                confirmed = confirm_delete_(title, message);
            else
            {
                wxMessageDialog dialog(this, message, title, wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
                dialog.SetYesNoLabels(tr("dialogs.delete_model.confirm"), tr("dialogs.delete_model.cancel"));
                confirmed = dialog.ShowModal() == wxID_YES;
            }
            if (!confirmed)
                return;
        }

        // Last first: a removal can apply at once and shift the models after it.
        std::vector<int> current;
        const auto       nowIds = model_viewer_->GetModelIds();
        for (const auto id : ids)
            if (const auto found = std::find(nowIds.begin(), nowIds.end(), id); found != nowIds.end())
                current.push_back(static_cast<int>(found - nowIds.begin()));
        std::sort(current.begin(), current.end());
        for (auto index = current.rbegin(); index != current.rend(); ++index)
            model_viewer_->RemoveModel(*index);
        model_viewer_->ClearSelection();
    }

    void MainWindow::HideModels(const std::vector<int>& indices)
    {
        if (model_viewer_ == nullptr)
            return;
        for (const int index : indices)
            model_viewer_->SetModelVisibility(index, false);
        if (models_panel_ != nullptr)
            models_panel_->RefreshModelRows();
    }

    void MainWindow::DuplicateModels(const std::vector<int>& indices)
    {
        if (model_viewer_ == nullptr || indices.empty())
            return;
        std::optional<int> copies;
        if (ask_copies_)
            copies = ask_copies_(indices.size());
        else
        {
            wxNumberEntryDialog dialog(this, tr("dialogs.duplicate.message"), tr("dialogs.duplicate.prompt"),
                                       tr(indices.size() == 1 ? "dialogs.duplicate.title_one" : "dialogs.duplicate.title_many"),
                                       1, 1, kMaxCopies);
            if (dialog.ShowModal() == wxID_OK)
                copies = static_cast<int>(dialog.GetValue());
        }
        if (!copies || *copies < 1)
            return;
        model_viewer_->Select(model_viewer_->DuplicateModels(indices, std::min(*copies, kMaxCopies)));
    }

    void MainWindow::ShowAllModels()
    {
        if (model_viewer_ == nullptr)
            return;
        const auto visibility = model_viewer_->GetModelVisibility();
        for (std::size_t index = 0; index < visibility.size(); ++index)
            if (!visibility[index])
                model_viewer_->SetModelVisibility(static_cast<int>(index), true);
        if (models_panel_ != nullptr)
            models_panel_->RefreshModelRows();
    }

    void MainWindow::ShowOnlyModels(const std::vector<int>& indices)
    {
        if (model_viewer_ == nullptr)
            return;
        const std::size_t count = model_viewer_->GetModelNames().size();
        for (std::size_t index = 0; index < count; ++index)
        {
            const int i = static_cast<int>(index);
            model_viewer_->SetModelVisibility(i, std::find(indices.begin(), indices.end(), i) != indices.end());
        }
        if (models_panel_ != nullptr)
            models_panel_->RefreshModelRows();
    }

    void MainWindow::SyncTargetToSelection()
    {
        if (model_viewer_ == nullptr || transform_toolbar_ == nullptr)
            return;
        const auto selected = model_viewer_->GetSelectedIndices();
        transform_toolbar_->SetSelectionCount(selected.size());
        if (selected.empty())
            transform_toolbar_->SetTargetIndex(application::service::SceneService::kAllModels);
        else if (selected.size() == 1)
            transform_toolbar_->SetTargetIndex(selected.front());
        else
            transform_toolbar_->SetTargetIndex(ui::framework::occt::ModelViewerPanel::kSelectionTarget);
    }

    void MainWindow::ToggleDockPage(wxWindow* page)
    {
        if (side_dock_ != nullptr && page != nullptr)
            side_dock_->Toggle(page);
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
