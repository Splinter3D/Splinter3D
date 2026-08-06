#include "app.hpp"

#include "ui/windows/demo_window/demo_window.hpp"
#include "ui/windows/main_window/main_window.hpp"

namespace app
{
    App::App(std::unique_ptr<ui::windows::MainWindow> mainWindow,
             std::unique_ptr<ui::windows::DemoWindow> demoWindow)
        : mainWindow_(std::move(mainWindow))
        , demoWindow_(std::move(demoWindow))
    {
    }

    App::~App() = default;

    bool App::init()
    {
        if (!mainWindow_)
            return false;

        mainWindow_->Show(true);
        // Optional second window, purely to demonstrate that the same
        // menu/toolbar/panel factories can be reassembled with a different
        // layout with zero duplicated logic. Safe to remove once the point
        // is made.
        if (demoWindow_)
        {
            demoWindow_->Show(true);
        }

        return true;
    }

    void App::shutdown()
    {
        // Hook point to release high-level services if/when needed.
    }
} // namespace app
