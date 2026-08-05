#pragma once

#include "ui/windows/base_window/base_window.hpp"

namespace ui::windows
{
    /**
     * @brief Main application window.
     *
     * MainWindow is the primary window displayed by the application.
     * It is responsible for creating the main layout, the status bar,
     * and the initial workspace panels.
     */
    class MainWindow : public BaseWindow
    {
      public:
        /**
         * @brief Constructs the main application window.
         */
        MainWindow();

      protected:
        /**
         * @brief Creates the status bar.
         */
        void InitStatusBar() override;

        /**
         * @brief Creates the main layout.
         */
        void InitLayout() override;

        /**
         * @brief Creates the default panels.
         */
        void InitPanels() override;

      private:
        /**
         * @brief Root layout of the window.
         *
         * Once assigned with SetSizer(), ownership is transferred to
         * wxWidgets. The pointer is retained only for adding child panels.
         */
        wxBoxSizer* rootSizer_ = nullptr;
    };
} // namespace ui::windows
