#pragma once

#include "ui/windows/demo_window/demo_window.hpp"
#include "ui/windows/main_window/main_window.hpp"

namespace app
{
    class App
    {
      public:
        explicit App(std::unique_ptr<ui::windows::MainWindow> mainWindow,
                     std::unique_ptr<ui::windows::DemoWindow> demoWindow = nullptr);

        ~App() = default; // nothing to delete: wx owns and destroys the windows

        App(const App&)            = delete;
        App& operator=(const App&) = delete;
        App(App&&)                 = delete;
        App& operator=(App&&)      = delete;

        bool init();
        void shutdown();

      private:
        // Non-owning: once Show()'d, wxWidgets takes ownership of every
        // top-level window and deletes it itself as soon as the user
        // closes it (Destroy(), deferred to the next idle cycle). App
        // must NOT also own/delete these pointers - doing so causes a
        // double free once the last window closes, which can silently
        // hang or corrupt the process on shutdown instead of exiting
        // cleanly (symptom: the executable stays locked even though
        // every window appears closed).
        ui::windows::MainWindow* mainWindow_ = nullptr;
        ui::windows::DemoWindow* demoWindow_ = nullptr;
    };
} // namespace app
