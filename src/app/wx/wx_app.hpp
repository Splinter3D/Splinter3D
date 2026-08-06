#pragma once

#include <memory>
#include <wx/app.h>

namespace app
{
    class App;

    /**
     * @brief Adapts wxWidgets' application entry point to Splinter3D's
     * architecture.
     *
     * WxApp is the only class in the codebase that depends directly on
     * wxApp. It contains no business logic: its sole responsibility is
     * to bootstrap the App instance and forward wx's OnInit/OnExit
     * lifecycle to it.
     *
     * WxApp is instantiated by the wxIMPLEMENT_APP macro, which also
     * generates the platform-specific entry point (WinMain on Windows,
     * main on Linux/macOS).
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * // main.cpp
     * #include "wx/wx_app.hpp"
     *
     * wxIMPLEMENT_APP(app::WxApp);
     * @endcode
     */
    class WxApp final : public wxApp
    {
      public:
        /**
         * @brief Constructs the wx application adapter.
         */
        WxApp();

        /**
         * @brief Destroys the wx application adapter.
         *
         * Declared out-of-line so that App only needs to be a complete
         * type in wx_app.cpp, not wherever WxApp is used (e.g. main.cpp).
         */
        ~WxApp() override;

        /**
         * @brief wxWidgets entry point, called once the wx runtime is ready.
         *
         * Builds the App instance via Bootstrap and initializes it.
         *
         * @return true if the application was successfully initialized
         * and should proceed to run its event loop, false to abort startup.
         */
        bool OnInit() override;

        /**
         * @brief wxWidgets exit point, called after the event loop ends.
         *
         * Shuts down the App instance before the process terminates.
         *
         * @return The result of wxApp::OnExit().
         */
        int OnExit() override;

      private:
        std::unique_ptr<App> application_;
    };
} // namespace app
