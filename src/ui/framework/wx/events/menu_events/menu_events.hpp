#pragma once

#include "../event_binder.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        class MenuEvents final : public EventBinder
        {
          public:
            using EventBinder::EventBinder;

            void bind() override;

          private:
            void bindFileMenu();
            void bindHelpMenu();

            void onNew();
            void onOpen();
            void onSave();
            void onExit();

            void onAbout();
        };

    } // namespace events
} // namespace ui::framework::wx
