#include "locale_manager.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

namespace platform::locale
{

    std::string LocaleManager::s_localePath = "./locale";

    std::unordered_map<std::string, std::string>
        LocaleManager::s_translations;

    LocaleManager& LocaleManager::get()
    {
        static LocaleManager instance;
        return instance;
    }

    void LocaleManager::init(const std::string& path)
    {
        s_localePath = path;

        const std::string language = detectSystemLanguage();

        setLanguage(language);
    }

    void LocaleManager::setLanguage(const std::string& language)
    {
        const std::string valid = validateLanguage(language);

        loadTranslations(valid);
        m_currentLanguage = valid;
    }

    std::string LocaleManager::getLanguage() const
    {
        return m_currentLanguage;
    }

    std::string LocaleManager::gettext(
        const std::string& key)
    {
        auto it = s_translations.find(key);

        if (it == s_translations.end())
            return key;

        return it->second;
    }

    void LocaleManager::loadTranslations(
        const std::string& language)
    {
        std::ifstream file(
            s_localePath + "/" + language + ".json");

        if (!file.is_open())
        {
            s_translations.clear();
            return;
        }

        try
        {
            nlohmann::json json;
            file >> json;

            std::unordered_map<std::string, std::string>
                translations;

            std::function<void(
                const nlohmann::json&,
                std::string)>
                flatten;

            flatten =
                [&](const nlohmann::json& node,
                    std::string           prefix) {
                    if (node.is_object())
                    {
                        for (auto& [key, value] : node.items())
                        {
                            std::string next =
                                prefix.empty()
                                    ? key
                                    : prefix + "." + key;

                            flatten(value, next);
                        }

                        return;
                    }

                    if (node.is_string())
                    {
                        translations[prefix] =
                            node.get<std::string>();
                    }
                };

            flatten(json, "");

            s_translations =
                std::move(translations);
        }
        catch (...)
        {
            s_translations.clear();
        }
    }

    std::string LocaleManager::validateLanguage(
        const std::string& language)
    {
        static constexpr const char* SUPPORTED[] =
            {
                "fr",
                "en",
                "es",
                "de"};

        std::string code = language;

        std::transform(
            code.begin(),
            code.end(),
            code.begin(),
            [](unsigned char c) {
                return std::tolower(c);
            });

        if (code.size() > 2)
            code = code.substr(0, 2);

        for (const char* supported : SUPPORTED)
        {
            if (code == supported)
                return code;
        }

        return "en";
    }

    std::string LocaleManager::detectSystemLanguage()
    {
#if defined(_WIN32)

        char*  lang = nullptr;
        size_t size = 0;

        if (_dupenv_s(
                &lang,
                &size,
                "LANG") == 0 &&
            lang)
        {
            std::string result(lang);

            free(lang);

            return validateLanguage(result);
        }

#else

        const char* lang =
            std::getenv("LANG");

        if (lang)
            return validateLanguage(lang);

#endif

        return "en";
    }

} // namespace platform::locale
