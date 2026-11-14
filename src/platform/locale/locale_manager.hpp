#pragma once

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <utility>

namespace platform::settings
{
    class SettingsRegistry;
}

namespace platform::locale
{
    /// The language setting: "default" (the system's) or a language code.
    inline constexpr const char* kLanguageSetting = "general.language";

    /**
     * @brief Loads assets/locale/<language>.json files and translates dotted keys.
     *
     * Each instance has its own locale folder, language and translations, so
     * tests (or several windows) can use independent managers. get() returns
     * the application-wide instance, initialized by app/bootstrap and used by
     * ui::framework::wx::locale::tr().
     */
    class LocaleManager
    {
      public:
        /// The application-wide instance.
        static LocaleManager& get();

        LocaleManager() = default;

        void init(
            const std::string& path = "./assets/locale");

        void setLanguage(
            const std::string& language);

        std::string getLanguage() const;

        /**
         * @brief Declares the language setting in settings and switches to the
         * language it holds (other than "default"). Later changes apply once
         * the main window is rebuilt (reloaded, or on the next start): strings
         * are read when windows are built.
         */
        void useSettings(settings::SettingsRegistry& settings);

        /** @brief The language a kLanguageSetting value stands for: the system's for "default". */
        static std::string resolve(const std::string& setting);

        std::string gettext(
            const std::string& key) const;

      private:
        void loadTranslations(
            const std::string& language);

        static std::string validateLanguage(
            const std::string& language);

        static std::string detectSystemLanguage();

      private:
        std::string m_currentLanguage = "en";

        std::string m_localePath = "./assets/locale";

        std::unordered_map<
            std::string,
            std::string>
            m_translations;
    };

    /// Translates @p key with the application-wide manager, then replaces each "{name}" with its value.
    std::string tr(
        const std::string&                                         key,
        std::initializer_list<std::pair<std::string, std::string>> args = {});

} // namespace platform::locale
