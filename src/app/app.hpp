#pragma once

#include <memory>

namespace ui::windows
{
    class MainWindow;
}

namespace app
{
    /**
     * @brief Owns and manages the application's high-level lifecycle.
     *
     * App is the top-level orchestrator of Splinter3D. It does not create
     * any business logic or draw anything itself — it simply owns the
     * main window and drives its lifecycle (initialization, execution,
     * shutdown).
     *
     * App is constructed by Bootstrap, which is responsible for wiring
     * together all lower-level services (locale, logging, preferences,
     * etc.) before the main window is created.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * Bootstrap bootstrap;
     * std::unique_ptr<App> app = bootstrap.build(argc, argv);
     *
     * if (app && app->init())
     * {
     *     // wx event loop takes over from here
     * }
     *
     * app->shutdown();
     * @endcode
     */
    class App
    {
      public:
        /**
         * @brief Constructs the application with an already-built main window.
         *
         * @param mainWindow The application's main window. App takes
         * ownership of it.
         */
        explicit App(std::unique_ptr<ui::windows::MainWindow> mainWindow);

        /**
         * @brief Destroys the application and releases owned resources.
         */
        ~App();

        /**
         * @brief Initializes the application and shows the main window.
         *
         * @return true if initialization succeeded, false otherwise
         * (e.g. if no main window was provided).
         */
        bool init();

        /**
         * @brief Releases high-level services before the application exits.
         *
         * Called once the wx event loop has ended, right before the
         * process terminates.
         */
        void shutdown();

      private:
        std::unique_ptr<ui::windows::MainWindow> mainWindow_;
    };
} // namespace app
