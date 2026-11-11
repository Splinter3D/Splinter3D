#include "window_events.hpp"

#include "ui/framework/wx/ids/ids.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        void WindowEvents::bind()
        {
            bindFullscreen();
            bindClose();
            bindResize();
        }

        void WindowEvents::bindFullscreen()
        {
            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent& event) {
                    toggleFullscreen(event.IsChecked());
                },
                ids::view::kToggleFullscreen);
        }

        void WindowEvents::toggleFullscreen(bool enabled)
        {
            frame_->ShowFullScreen(
                enabled,
                wxFULLSCREEN_ALL);

            if (auto* menu = frame_->GetMenuBar())
            {
                menu->Check(
                    ids::view::kToggleFullscreen,
                    enabled);
            }
        }

        void WindowEvents::bindClose()
        {
            frame_->Bind(
                wxEVT_CLOSE_WINDOW,
                // Skip rather than Destroy(): the frame's default handler
                // destroys it once every close handler had its say, so one
                // bound earlier (e.g. MainWindow asking about running tasks)
                // can still veto, whatever the binding order.
                [](wxCloseEvent& event) { event.Skip(); });
        }

        void WindowEvents::bindResize()
        {
            frame_->Bind(
                wxEVT_SIZE,
                [](wxSizeEvent& event) {
                    event.Skip();
                });
        }

    } // namespace events
} // namespace ui::framework::wx
