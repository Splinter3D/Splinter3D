#pragma once

#include "ui/framework/occt/model_viewer_panel.hpp"
#include "ui/framework/wx/events/event_manager.hpp"
#include "ui/layouts/base_layout/base_layout.hpp"
#include "ui/toolbars/transform_toolbar/transform_toolbar.hpp"
#include "ui/windows/base_window/base_window.hpp"

#include <memory>

namespace ui::dialogs
{
    class ModelManagementDialog;
}

namespace ui::windows
{
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
        std::unique_ptr<ui::layouts::BaseLayout>                 layout_;
        ui::framework::occt::ModelViewerPanel*                   model_viewer_      = nullptr;
        ui::toolbars::TransformToolbar*                          transform_toolbar_ = nullptr;
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

        void ShowModelManagement();
    };
} // namespace ui::windows
