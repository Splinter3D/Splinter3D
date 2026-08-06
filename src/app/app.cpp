#include "app.hpp"

#include "ui/windows/demo_window/demo_window.hpp"
#include "ui/windows/main_window/main_window.hpp"

namespace app
{
    App::App(std::unique_ptr<ui::windows::MainWindow> mainWindow,
             std::unique_ptr<ui::windows::DemoWindow> demoWindow)
        // Release ownership immediately: from here on, wxWidgets is the
        // sole owner of these objects. App only keeps a non-owning
        // pointer to call Show() on them.
        : mainWindow_(mainWindow.release())
        , demoWindow_(demoWindow.release())
    {
    }

    bool App::init()
    {
        if (!mainWindow_)
            return false;

        mainWindow_->Show(true);

        if (demoWindow_)
        {
            demoWindow_->Show(true);
        }

        return true;
    }

    void App::shutdown()
    {
        // Hook point to release high-level services if/when needed.
        // Do NOT delete mainWindow_/demoWindow_ here - wx already did,
        // or will, once their respective windows are closed.
    }
} // namespace app
