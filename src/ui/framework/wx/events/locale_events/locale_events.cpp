#include "locale_events.hpp"

#include "ui/framework/wx/ids/ids.hpp"

#include "app/config/config_manager.hpp"

namespace ui::framework::wx
{
    namespace events
    {
        void LocaleEvents::bind()
        {
            bindLanguages();
        }

        void LocaleEvents::setLanguageCallback(
            std::function<void(const std::string &)> callback)
        {
            on_language_changed_ = std::move(callback);
        }

        void LocaleEvents::bindLanguages()
        {

            auto bindLanguage =
                [this](int id, const std::string &language)
            {
                frame_->Bind(
                    wxEVT_MENU,

                    [this, language](wxCommandEvent &)
                    {
                        auto config =
                            app::config::ConfigManager::load();

                        config.language = language;

                        app::config::ConfigManager::save(config);

                        if (on_language_changed_)
                        {
                            on_language_changed_(language);
                        }
                    },

                    id);
            };
            bindLanguage(ids::language::kDefault, "default");
            bindLanguage(ids::language::kEnglish, "en");
            bindLanguage(ids::language::kFrench, "fr");
            bindLanguage(ids::language::kGerman, "de");
            bindLanguage(ids::language::kSpanish, "es");
        }

    } // namespace events
} // namespace ui::framework::wx
