#pragma once

#include "ui/layouts/base_layout/base_layout.hpp"
#include "ui/windows/base_window/base_window.hpp"

#include <memory>

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

      private:
        // Owns the spatial organization of the window. MainWindow never
        // touches a wxSizer directly - it only asks layout_ where to put
        // each widget it creates.
        std::unique_ptr<ui::layouts::BaseLayout> layout_;
    };
} // namespace ui::windows
