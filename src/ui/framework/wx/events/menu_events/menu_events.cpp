#include "menu_events.hpp"

#include "ui/dialogs/dialogs.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/framework/wx/locale/locale.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        void MenuEvents::bind()
        {
            bindFileMenu();
            bindHelpMenu();
        }

        void MenuEvents::bindFileMenu()
        {
            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    if (on_new_)
                        on_new_();
                },
                ids::file::kNew);

            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    if (on_open_)
                        on_open_();
                },
                ids::file::kOpen);

            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    if (on_save_)
                        on_save_();
                },
                ids::file::kSave);

            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    if (on_exit_)
                        on_exit_();
                },
                ids::file::kExit);
        }

        void MenuEvents::bindHelpMenu()
        {
            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    onAbout();
                },
                ids::help::kAbout);
        }

        void MenuEvents::onAbout()
        {
            ui::dialogs::info(
                frame_,
                locale::tr("wxWidgets Demo\nAll major widgets in one place."),
                locale::tr("About"));
        }

    } // namespace events
} // namespace ui::framework::wx
