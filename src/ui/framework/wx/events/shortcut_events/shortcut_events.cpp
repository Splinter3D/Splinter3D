#include "shortcut_events.hpp"

#include "ui/framework/wx/ids/ids.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        void ShortcutEvents::bind()
        {
            bindCommonShortcuts();
            bindFullscreenShortcut();
        }

        void ShortcutEvents::bindCommonShortcuts()
        {
            frame_->Bind(
                wxEVT_CHAR_HOOK,
                [this](wxKeyEvent &event)
                {
                    const int key = event.GetKeyCode();

                    // Ctrl + S
                    if (event.ControlDown() && key == 'S')
                    {
                        wxCommandEvent saveEvent(
                            wxEVT_MENU,
                            ids::file::kSave);

                        frame_->ProcessWindowEvent(saveEvent);
                        return;
                    }

                    // Ctrl + O
                    if (event.ControlDown() && key == 'O')
                    {
                        wxCommandEvent openEvent(
                            wxEVT_MENU,
                            ids::file::kOpen);

                        frame_->ProcessWindowEvent(openEvent);
                        return;
                    }

                    // Ctrl + Q
                    if (event.ControlDown() && key == 'Q')
                    {
                        wxCommandEvent exitEvent(
                            wxEVT_MENU,
                            ids::file::kExit);

                        frame_->ProcessWindowEvent(exitEvent);
                        return;
                    }

                    event.Skip();
                });
        }

        void ShortcutEvents::bindFullscreenShortcut()
        {
            frame_->Bind(
                wxEVT_CHAR_HOOK,
                [this](wxKeyEvent &event)
                {
                    if (event.GetKeyCode() == WXK_F11)
                    {
                        wxCommandEvent fullscreenEvent(
                            wxEVT_MENU,
                            ids::view::kToggleFullscreen);

                        frame_->ProcessWindowEvent(fullscreenEvent);
                        return;
                    }

                    event.Skip();
                });
        }

    } // namespace events
} // namespace ui::framework::wx
