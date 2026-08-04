#include "theme_events.hpp"

#include "platform/preference/preference_manager.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/framework/wx/theme/theme_manager.hpp"

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
            auto& manager = theme::ThemeManager::get();

            manager.setDark(dark);
            manager.apply(frame_);

            auto config = platform::preference::PreferenceManager::load();

            config.darkMode = dark;

            platform::preference::PreferenceManager::save(config);
        }

    } // namespace events
} // namespace ui::framework::wx
