#include "menu_events.hpp"

#include "app/locale/locale.hpp"
#include "ui/dialogs/dialogs.hpp"
#include "ui/framework/wx/ids/ids.hpp"

using app::locale::tr;

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
                    onNew();
                },
                ids::file::kNew);

            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    onOpen();
                },
                ids::file::kOpen);

            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    onSave();
                },
                ids::file::kSave);

            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    onExit();
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

        void MenuEvents::onNew()
        {
            // TODO: implement/complete the function.
            ui::dialogs::info(
                frame_,
                tr("New file created (demo)."),
                tr("New"));
        }

        void MenuEvents::onOpen()
        {
            // TODO: implement/complete the function.
            if (auto path = ui::dialogs::openFile(frame_))
            {
                ui::dialogs::info(frame_,
                                  tr("Opened: ") + *path,
                                  tr("Open"));
            }
        }

        void MenuEvents::onSave()
        {
            // TODO: implement/complete the function.
        }

        void MenuEvents::onExit()
        {
            // TODO: implement function
            frame_->Close();
        }

        void MenuEvents::onAbout()
        {
            // TODO: implement function
            ui::dialogs::info(
                frame_,
                tr("wxWidgets Demo\nAll major widgets in one place."),
                tr("About"));
        }

    } // namespace events
} // namespace ui::framework::wx
