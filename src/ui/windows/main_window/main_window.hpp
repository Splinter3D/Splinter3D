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
#include <vector>

namespace ui::dialogs
{
    class ModelManagementDialog;
}

namespace ui::panels
{
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
        // Defined in main_window.cpp, where ModelManagementDialog is a complete
        // type: model_management_menu_'s unique_ptr can only be destroyed there.
        ~MainWindow() override;

        /**
         * @brief Removes these models (indices as the viewer's), after asking
         * if kConfirmDeleteSetting is on; then nothing is selected. Does
         * nothing for an empty list.
         */
        void DeleteModels(std::vector<int> indices);

#if defined(SPLINTER3D_TESTING)
        /** @brief Test-only: replaces the "quit with tasks pending?" dialog (true = quit). */
        void SetConfirmQuit(std::function<bool(std::size_t pending)> confirm);
        /** @brief Test-only: replaces the "delete?" dialog (true = delete). */
        void SetConfirmDelete(std::function<bool(const wxString& message)> confirm);
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
        ui::panels::TasksPanel*                  tasks_panel_       = nullptr;
        ui::panels::SettingsPanel*               settings_panel_    = nullptr;
        // Asked before closing with tasks pending; true quits. Empty: a dialog asks.
        std::function<bool(std::size_t pending)> confirm_quit_;
        // Asked before deleting models; true deletes. Empty: a dialog asks.
        std::function<bool(const wxString& message)>             confirm_delete_;
        std::unique_ptr<ui::framework::wx::events::EventManager> event_manager_;
        std::unique_ptr<ui::dialogs::ModelManagementDialog>      model_management_menu_;

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

        void ShowModelManagement();

        // Asks before quitting with tasks pending (vetoes on "Keep working"),
        // then cancels them and waits until none runs.
        void OnClose(wxCloseEvent& event);
    };
} // namespace ui::windows
