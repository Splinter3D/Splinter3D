#include "theme_events.hpp"

#include "app/config/config_manager.hpp"
#include "app/theme/theme_manager.hpp"
#include "ui/framework/wx/ids/ids.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        void ThemeEvents::bind()
        {
            bindToggleDarkMode();
            bindThemeModes();
        }

        void ThemeEvents::bindToggleDarkMode()
        {
            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent& event) {
                    applyTheme(event.IsChecked());
                },
                ids::theme::kToggleDarkMode);
        }

        void ThemeEvents::bindThemeModes()
        {
            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    applyTheme(false);
                },
                ids::theme::kLightMode);

            frame_->Bind(
                wxEVT_MENU,
                [this](wxCommandEvent&) {
                    applyTheme(true);
                },
                ids::theme::kDarkMode);
        }

        void ThemeEvents::applyTheme(bool dark)
        {
            auto& manager = app::theme::ThemeManager::get();

            manager.setDark(dark);
            manager.apply(frame_);

            auto config = app::config::ConfigManager::load();

            config.darkMode = dark;

            app::config::ConfigManager::save(config);
        }

    } // namespace events
} // namespace ui::framework::wx
