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
                locale::tr("New file created (demo)."),
                locale::tr("New"));
        }

        void MenuEvents::onOpen()
        {
            // TODO: implement/complete the function.
            if (auto path = ui::dialogs::openFile(frame_))
            {
                ui::dialogs::info(frame_,
                                  locale::tr("Opened: ") + *path,
                                  locale::tr("Open"));
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
                locale::tr("wxWidgets Demo\nAll major widgets in one place."),
                locale::tr("About"));
        }

    } // namespace events
} // namespace ui::framework::wx
