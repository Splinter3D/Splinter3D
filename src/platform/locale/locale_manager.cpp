#include "locale_manager.hpp"

#include "platform/settings/settings_registry.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

namespace platform::locale
{

    LocaleManager& LocaleManager::get()
    {
        static LocaleManager instance;
        return instance;
    }

    void LocaleManager::init(const std::string& path)
    {
        m_localePath = path;

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

    void LocaleManager::useSettings(settings::SettingsRegistry& settings)
    {
        settings.add({kLanguageSetting, "settings.section.general", "settings.general.language", "settings.general.restart", settings::Kind::Choice, "default", {{"default", "settings.general.language_system"}, {"en", "settings.languages.en"}, {"fr", "settings.languages.fr"}, {"de", "settings.languages.de"}, {"es", "settings.languages.es"}}});
        const std::string language = settings.get(kLanguageSetting);
        if (language != "default")
            setLanguage(language);
    }

    std::string LocaleManager::resolve(const std::string& setting)
    {
        return validateLanguage(setting == "default" ? detectSystemLanguage() : setting);
    }

    std::string LocaleManager::gettext(
        const std::string& key) const
    {
        auto it = m_translations.find(key);

        if (it == m_translations.end())
            return key;

        return it->second;
    }

    void LocaleManager::loadTranslations(
        const std::string& language)
    {
        std::ifstream file(
            m_localePath + "/" + language + ".json");

        if (!file.is_open())
        {
            m_translations.clear();
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

            m_translations =
                std::move(translations);
        }
        catch (...)
        {
            m_translations.clear();
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

    std::string tr(
        const std::string&                                         key,
        std::initializer_list<std::pair<std::string, std::string>> args)
    {
        std::string text = LocaleManager::get().gettext(key);

        for (const auto& [name, value] : args)
        {
            const std::string placeholder = "{" + name + "}";

            for (std::size_t at = text.find(placeholder); at != std::string::npos;
                 at             = text.find(placeholder, at + value.size()))
                text.replace(at, placeholder.size(), value);
        }

        return text;
    }

} // namespace platform::locale
