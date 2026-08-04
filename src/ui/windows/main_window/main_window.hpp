#pragma once

#include <wx/frame.h>

namespace ui::windows
{
    /**
     * @brief The application's main window.
     *
     * MainWindow is the root wxFrame of Splinter3D. It contains no
     * business logic — its sole responsibility is to host the visual
     * structure of the application (menus, toolbars, panels, status bar).
     *
     * It is created by Bootstrap and owned by App for the lifetime of
     * the application.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto mainWindow = std::make_unique<ui::windows::MainWindow>();
     * mainWindow->Show(true);
     * @endcode
     */
    class MainWindow : public wxFrame
    {
      public:
        /**
         * @brief Constructs the main window.
         *
         * Creates an empty top-level frame with a status bar. Menus,
         * toolbars, and panels are added incrementally as the
         * application grows.
         */
        MainWindow();

        /**
         * @brief Destroys the main window.
         */
        ~MainWindow() override = default;
    };
} // namespace ui::windows
