#pragma once

#include "../event_binder.hpp"

#include <functional>
#include <utility>

namespace ui::framework::wx
{
    namespace events
    {
        class MenuEvents final : public EventBinder
        {
          public:
            using Action = std::function<void()>;

            MenuEvents(wxFrame* frame,
                       Action   on_new,
                       Action   on_open,
                       Action   on_save,
                       Action   on_exit)
                : EventBinder(frame), on_new_(std::move(on_new)), on_open_(std::move(on_open)), on_save_(std::move(on_save)), on_exit_(std::move(on_exit))
            {
            }

            void bind() override;

          private:
            void bindFileMenu();
            void bindHelpMenu();

            void onAbout();

            Action on_new_;
            Action on_open_;
            Action on_save_;
            Action on_exit_;
        };

    } // namespace events
} // namespace ui::framework::wx
