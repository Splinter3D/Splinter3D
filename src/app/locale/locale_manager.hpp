#pragma once

#include <string>
#include <unordered_map>

namespace app::locale
{

    class LocaleManager
    {
    public:
        static LocaleManager &get();

        void init(
            const std::string &path = "./locale");

        void setLanguage(
            const std::string &language);

        std::string getLanguage() const;

        std::string gettext(
            const std::string &key);

    private:
        LocaleManager() = default;

        void loadTranslations(
            const std::string &language);

        static std::string validateLanguage(
            const std::string &language);

        static std::string detectSystemLanguage();

    private:
        std::string m_currentLanguage = "en";

        static std::string s_localePath;

        static std::unordered_map<
            std::string,
            std::string>
            s_translations;
    };

} // namespace app::locale
