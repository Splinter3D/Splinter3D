#include "event_manager.hpp"

#include "locale_events/locale_events.hpp"
#include "menu_events/menu_events.hpp"
#include "shortcut_events/shortcut_events.hpp"
#include "theme_events/theme_events.hpp"
#include "widget_events/widget_events.hpp"
#include "window_events/window_events.hpp"

#include <utility>

namespace ui::framework::wx
{
    namespace events
    {
        EventManager::EventManager(wxFrame* frame, Callbacks callbacks)
        {
            if (!frame)
                return;

            binders_.emplace_back(
                std::make_unique<MenuEvents>(
                    frame,
                    std::move(callbacks.onNew),
                    std::move(callbacks.onOpen),
                    std::move(callbacks.onSave),
                    std::move(callbacks.onExit)));

            binders_.emplace_back(
                std::make_unique<ThemeEvents>(frame));

            binders_.emplace_back(
                std::make_unique<LocaleEvents>(frame));

            binders_.emplace_back(
                std::make_unique<ShortcutEvents>(frame, std::move(callbacks.onModelManagement)));

            binders_.emplace_back(
                std::make_unique<WidgetEvents>(
                    frame,
                    std::move(callbacks.transformControls),
                    std::move(callbacks.onTransformChanged)));

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
