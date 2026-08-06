#include "event_manager.hpp"

#include "locale_events/locale_events.hpp"
#include "menu_events/menu_events.hpp"
#include "shortcut_events/shortcut_events.hpp"
#include "theme_events/theme_events.hpp"
#include "widget_events/widget_events.hpp"
#include "window_events/window_events.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        EventManager::EventManager(wxFrame* frame)
        {
            if (!frame)
                return;

            binders_.emplace_back(
                std::make_unique<MenuEvents>(frame));

            binders_.emplace_back(
                std::make_unique<ThemeEvents>(frame));

            binders_.emplace_back(
                std::make_unique<LocaleEvents>(frame));

            binders_.emplace_back(
                std::make_unique<ShortcutEvents>(frame));

            binders_.emplace_back(
                std::make_unique<WidgetEvents>(frame));

            binders_.emplace_back(
                std::make_unique<WindowEvents>(frame));
        }

        void EventManager::bindAll()
        {
            for (auto& binder : binders_)
            {
                if (binder)
                    binder->bind();
            }
        }

    } // namespace events
} // namespace ui::framework::wx
