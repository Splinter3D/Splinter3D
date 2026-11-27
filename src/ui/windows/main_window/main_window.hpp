#pragma once

#include "platform/settings/settings_registry.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"
#include "ui/framework/wx/events/event_manager.hpp"
#include "ui/layouts/base_layout/base_layout.hpp"
#include "ui/toolbars/transform_toolbar/transform_toolbar.hpp"
#include "ui/windows/base_window/base_window.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ui::panels
{
    class ModelsPanel;
    class SettingsPanel;
    class SideDock;
    class TasksPanel;
} // namespace ui::panels

namespace ui::windows
{
    /// Whether the tasks panel opens by itself when a task starts ("1"/"0").
    inline constexpr const char* kTasksAutoOpenSetting = "tasks.auto_open";
    /// Whether deleting models asks first ("1"/"0").
    inline constexpr const char* kConfirmDeleteSetting = "models.confirm_delete";

    /**
     * @brief Main application window.
     *
     * MainWindow is the primary window displayed by the application.
     * It is responsible for creating the menu bar, the toolbars, the main
     * layout, the status bar, and the initial workspace panels.
     */
    class MainWindow : public BaseWindow
    {
      public:
        MainWindow();

        /**
         * @brief Removes these models (indices as the viewer's), after asking
         * if kConfirmDeleteSetting is on; then nothing is selected. Does
         * nothing for an empty list.
         */
        void DeleteModels(std::vector<int> indices);
        /** @brief Hides these models (they leave the selection). */
        void HideModels(const std::vector<int>& indices);
        /** @brief Shows these models and hides every other. */
        void ShowOnlyModels(const std::vector<int>& indices);
        /** @brief Shows every model. */
        void ShowAllModels();
        /**
         * @brief Asks how many copies to make, then copies these models in
         * place that many times; the copies become the selection.
         */
        void DuplicateModels(const std::vector<int>& indices);

#if defined(SPLINTER3D_TESTING)
        /** @brief Test-only: replaces the "quit with tasks pending?" dialog (true = quit). */
        void SetConfirmQuit(std::function<bool(std::size_t pending)> confirm);
        /** @brief Test-only: replaces the "delete?" dialog (true = delete). */
        void SetConfirmDelete(std::function<bool(const wxString& title, const wxString& message)> confirm);
        /**
         * @brief Test-only: replaces the "reload for the new language?" dialog
         * (true = reload). With tasks pending it is only told (the answer is ignored).
         */
        void SetConfirmReload(std::function<bool(std::size_t pending)> confirm);
        /** @brief Test-only: replaces the "how many copies?" dialog (nullopt = cancelled). */
        void SetAskCopies(std::function<std::optional<int>(std::size_t models)> ask);
#endif

      protected:
        void InitMenuBar() override;
        void InitStatusBar() override;
        void InitLayout() override;
        void InitToolBars() override;
        void InitPanels() override;
        void BindEvents() override;

      private:
        // Owns the spatial organization of the window. MainWindow never
        // touches a wxSizer directly - it only asks layout_ where to put
        // each widget it creates.
        std::unique_ptr<ui::layouts::BaseLayout> layout_;
        ui::framework::occt::ModelViewerPanel*   model_viewer_      = nullptr;
        ui::toolbars::TransformToolbar*          transform_toolbar_ = nullptr;
        ui::panels::SideDock*                    side_dock_         = nullptr;
        ui::panels::ModelsPanel*                 models_panel_      = nullptr;
        ui::panels::TasksPanel*                  tasks_panel_       = nullptr;
        ui::panels::SettingsPanel*               settings_panel_    = nullptr;
        // Asked before closing with tasks pending; true quits. Empty: a dialog asks.
        std::function<bool(std::size_t pending)> confirm_quit_;
        // Asked before deleting models; true deletes. Empty: a dialog asks.
        std::function<bool(const wxString& title, const wxString& message)> confirm_delete_;
        // Asked when the language changes; true reloads. Empty: a dialog asks.
        std::function<bool(std::size_t pending)> confirm_reload_;
        // Asked how many copies to make; nullopt cancels. Empty: a dialog asks.
        std::function<std::optional<int>(std::size_t models)> ask_copies_;
        platform::settings::Subscription                      language_subscription_;
        // The tool to open once the models are in (a reload's, see Reload()).
        ui::toolbars::TransformToolbar::ActiveTool               reopen_tool_ = ui::toolbars::TransformToolbar::ActiveTool::None;
        std::unique_ptr<ui::framework::wx::events::EventManager> event_manager_;

        // Helper function to initialize a menu bar entry and add it to the menu bar.
        void InitMenuBarEntry(wxMenuBar* menuBar, int menuId, const wxString& title);

        // Reads the toolbar's current fields and applies them to the model viewer.
        void ApplyTransform();

        // Refreshes the toolbar's spin fields to reflect the transform
        // currently applied to its selected target.
        void SyncTransformFields();

        // Shows/hides/repositions the viewport's gizmo to match the
        // toolbar's current target and active tool.
        void UpdateGizmoState();

        // Puts the toolbar's target dropdown on the viewer's selection.
        void SyncTargetToSelection();

        // Opens page in the side dock, or closes it when it is the open one.
        void ToggleDockPage(wxWindow* page);

        // Asks before quitting with tasks pending (vetoes on "Keep working"),
        // then cancels them and waits until none runs.
        void OnClose(wxCloseEvent& event);

        // The language setting changed: strings are read when the window is
        // built, so offers to Reload() now (unless tasks are pending), or
        // leaves it for the next start.
        void OfferReload();

        // Switches to the language setting's language and replaces this
        // window with a new one in its place, holding the same models,
        // camera, selection, open tool and side dock page.
        void Reload();
    };
} // namespace ui::windows
