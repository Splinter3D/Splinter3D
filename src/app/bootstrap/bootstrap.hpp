#pragma once

#include <memory>

namespace app
{
    class App;
}

namespace app
{
    /**
     * @brief Constructs and wires together the application's dependencies.
     *
     * Bootstrap is responsible for dependency injection: it builds
     * services from platform/, core/, and application/ (logging, locale,
     * data root resolution, signal handlers, etc.) and assembles the
     * fully-formed App instance before the application starts running.
     *
     * WxApp is the only class that should ever call Bootstrap::build().
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * Bootstrap bootstrap;
     * std::unique_ptr<App> app = bootstrap.build(argc, argv);
     * @endcode
     */
    class Bootstrap
    {
      public:
        /**
         * @brief Builds a fully-initialized App instance.
         *
         * Resolves the executable path and data root, initializes
         * logging and locale, installs OS signal handlers, and creates
         * the main window before returning the assembled App.
         *
         * @param argc Argument count, as received from wxApp.
         * @param argv Argument values, as received from wxApp.
         *
         * @return A fully-constructed App, ready to be initialized.
         */
        std::unique_ptr<App> build(int argc, char** argv);
    };
} // namespace app
