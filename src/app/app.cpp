#include "app.hpp"

#include "ui/windows/main_window/main_window.hpp"

namespace app
{
    App::App(std::unique_ptr<ui::windows::MainWindow> mainWindow)
        // Release ownership immediately: from here on, wxWidgets is the
        // sole owner of these objects. App only keeps a non-owning
        // pointer to call Show() on them.
        : main_window_(mainWindow.release())
    {
    }

    bool App::init()
    {
        if (!main_window_)
            return false;

        main_window_->Show(true);

        return true;
    }

    void App::shutdown()
    {
        // Hook point to release high-level services if/when needed.
        // Do NOT delete main_window_/demo_window_ here - wx already did,
        // or will, once their respective windows are closed.
    }
} // namespace app
