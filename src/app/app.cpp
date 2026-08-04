#include "app.hpp"

#include "ui/windows/main_window/main_window.hpp"

namespace app
{
    App::App(std::unique_ptr<ui::windows::MainWindow> mainWindow)
        : mainWindow_(std::move(mainWindow))
    {
    }

    App::~App() = default;

    bool App::init()
    {
        if (!mainWindow_)
            return false;

        mainWindow_->Show(true);
        return true;
    }

    void App::shutdown()
    {
        // Hook point to release high-level services if/when needed.
    }
} // namespace app
