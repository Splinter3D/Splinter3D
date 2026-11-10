#pragma once

#include <string>
#include <unordered_map>

namespace platform::locale
{

    /**
     * @brief Loads locale/<language>.json files and translates dotted keys.
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
            const std::string& path = "./locale");

        void setLanguage(
            const std::string& language);

        std::string getLanguage() const;

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

        std::string m_localePath = "./locale";

        std::unordered_map<
            std::string,
            std::string>
            m_translations;
    };

} // namespace platform::locale
