/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** Locale management for i18n with gettext
*/

#include <Splinter3D/Utils/Locale.hpp>
#include <Splinter3D/Utils/Logger.hpp>
#include <algorithm>
#include <clocale>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace splinter3D::utils
{
    // Static member definitions
    std::string                                  Locale::s_localePath;
    std::unordered_map<std::string, std::string> Locale::s_translations;

    /**
     * @brief Initialize i18n with system locale detection
     *
     * Detects the system language from:
     * - LANG environment variable (Linux/macOS)
     * - System locale settings (Windows)
     *
     * Supported languages: fr, en, es, de
     * Fallback: en (English)
     *
     * @param localePath Path to locale directory (e.g., "./locale")
     */
    void Locale::init(const char* localePath)
    {
        s_localePath = localePath;

        // Always detect language from system environment/API
        // (not from ACTIVE_LANG file, which should only be updated by setLanguage())
        std::string active = detectSystemLanguage();
        if (!hasTranslation(active.c_str()))
        {
            active = "en"; // fallback to English
        }

        // Create or update ACTIVE_LANG file in source locale/ directory
        // (always in ./locale/, regardless of localePath used for .mo files)
        std::string   sourceLocaleDir = "./locale";
        std::string   activeFile      = sourceLocaleDir + "/ACTIVE_LANG";
        std::ofstream ofs(activeFile, std::ios::trunc);
        if (ofs.good())
        {
            ofs << active << std::endl;
            ofs.close();
        }

        const std::string full = fullLocaleFor(active.c_str());
        if (!full.empty())
        {
            if (setlocale(LC_ALL, full.c_str()) == nullptr)
            {
                clog("[Locale::init] setlocale failed for ", full);
                setlocale(LC_ALL, "");
                setenv("LANGUAGE", active.c_str(), 1);
            }
            else
            {
                setenv("LANGUAGE", active.c_str(), 1);
            }
        }
        else
        {
            setlocale(LC_ALL, "");
            setenv("LANGUAGE", active.c_str(), 1);
        }

        loadTranslations(active.c_str());

        clog("[Locale::init] Path: ", localePath);
        clog("[Locale::init] Detected language from system: ", active);
    }

    /**
     * @brief Initialize i18n with a specific language override
     *
     * @param localePath Path to locale directory (e.g., "./locale")
     * @param forceLang Language code to force (e.g., "fr", "en", "es", "de")
     */
    void Locale::init(const char* localePath, const char* forceLang)
    {
        s_localePath = localePath;

        std::string active = validateLanguage(forceLang);

        // Save to ACTIVE_LANG in source locale/ directory
        // (always in ./locale/, regardless of localePath used for .mo files)
        std::string   sourceLocaleDir = "./locale";
        std::string   activeFile      = sourceLocaleDir + "/ACTIVE_LANG";
        std::ofstream ofs(activeFile, std::ios::trunc);
        if (ofs.good())
        {
            ofs << active << std::endl;
            ofs.close();
        }

        // Apply the language
        const std::string full = fullLocaleFor(active.c_str());
        if (!full.empty())
        {
            if (setlocale(LC_ALL, full.c_str()) == nullptr)
            {
                clog("[Locale::init(forced)] setlocale failed for ", full);
                setlocale(LC_ALL, "");
                setenv("LANGUAGE", active.c_str(), 1);
            }
            else
            {
                setenv("LANGUAGE", active.c_str(), 1);
            }
        }
        else
        {
            setlocale(LC_ALL, "");
            setenv("LANGUAGE", active.c_str(), 1);
        }

        loadTranslations(active.c_str());

        clog("[Locale::init(forced)] Path: ", localePath);
        clog("[Locale::init(forced)] Active language: ", active);
    }

    /**
     * @brief Change the language dynamically
     *
     * Changes the translation language to the specified language code.
     * If the language is not supported, falls back to English.
     *
     * Supported languages: fr, en, es, de
     *
     * @param langCode Language code (e.g., "fr", "en", "es", "de")
     *
     * Example:
     *   Locale::setLanguage("fr");  // Switch to French
     *   std::cout << _("Play");     // Will display "Jouer"
     *   Locale::setLanguage("ja");  // Not supported, fallback to English
     *   std::cout << _("Play");     // Will display "Play"
     */
    void Locale::setLanguage(const char* langCode)
    {
        // Validate the language (with fallback to English)
        std::string validLang = validateLanguage(langCode);

        // Save to ACTIVE_LANG file in source locale/ directory
        // (always in ./locale/, regardless of localePath used for .mo files)
        std::string   sourceLocaleDir = "./locale";
        std::string   activeFile      = sourceLocaleDir + "/ACTIVE_LANG";
        std::ofstream ofs(activeFile, std::ios::trunc);
        if (ofs.good())
        {
            ofs << validLang << std::endl;
            ofs.close();
        }

        // Apply language by setting the C locale and LANGUAGE for gettext
        const std::string full = fullLocaleFor(validLang.c_str());
        if (!full.empty())
        {
            if (setlocale(LC_ALL, full.c_str()) == nullptr)
            {
                setlocale(LC_ALL, "");
            }
        }
        else
        {
            setlocale(LC_ALL, "");
        }

        // Set LANGUAGE in process environment so gettext recognizes it
        setenv("LANGUAGE", validLang.c_str(), 1);

        // Reload translations
        loadTranslations(validLang.c_str());

        clog("[Locale::setLanguage] Requested: ", langCode);
        clog("[Locale::setLanguage] Validated to: ", validLang);
        clog("[Locale::setLanguage] Saved to ACTIVE_LANG");
    }

    /**
     * @brief Get the translated string for a given key
     *
     * @param key Translation key (e.g., "play", "settings.title")
     * @param args Optional arguments for string formatting (e.g., {{"lang", "fr"}})
     * @return Translated string or the key itself if not found
     */
    std::string Locale::gettext(const std::string& key, std::unordered_map<std::string, std::string> args)
    {
        auto it = s_translations.find(key);
        if (it == s_translations.end())
        {
            // Key not found, return the key itself
            return key;
        }

        std::string translated = it->second;

        // Replace placeholders in the translated string with provided arguments
        for (const auto& [argKey, argValue] : args)
        {
            std::string placeholder = "{" + argKey + "}";
            size_t      pos         = 0;
            while ((pos = translated.find(placeholder, pos)) != std::string::npos)
            {
                translated.replace(pos, placeholder.length(), argValue);
                pos += argValue.length();
            }
        }

        return translated;
    }

    /**
     * @brief Get the currently active language
     * @return Active language code (e.g., "fr", "en", "es", "de")
     */
    std::string Locale::getActiveLanguage()
    {
        std::string   activeFile = "./locale/ACTIVE_LANG";
        std::ifstream ifs(activeFile);
        if (ifs.good())
        {
            std::string lang;
            std::getline(ifs, lang);
            ifs.close();
            return validateLanguage(lang.c_str());
        }

        std::string detectedLang = detectSystemLanguage();
        return validateLanguage(detectedLang.c_str());
    }

    /**
     * @brief Check if a translation exists for a language
     *
     * @param lang language code (e.g., "fr")
     * @return true if directory locale/<lang>/LC_MESSAGES/ exists
     */
    bool Locale::hasTranslation(const char* lang)
    {
        if (lang == nullptr || lang[0] == '\0')
            return false;

        std::string file = s_localePath + "/" + lang + ".json";
        return std::filesystem::exists(file);
    }

    /**
     * @brief Detect system language from environment
     * @return Language code (e.g., "fr", "en", "es", "de") or "en" as fallback
     */
    std::string Locale::detectSystemLanguage()
    {
        // Try LANG environment variable (Unix-like systems)
        const char* lang = std::getenv("LANG");
        if (lang != nullptr && lang[0] != '\0')
        {
            clog("[Locale::detectSystemLanguage] Found LANG: ", lang);
            return extractLanguageCode(lang);
        }

        // Try LC_ALL environment variable
        const char* lc_all = std::getenv("LC_ALL");
        if (lc_all != nullptr && lc_all[0] != '\0')
        {
            clog("[Locale::detectSystemLanguage] Found LC_ALL: ", lc_all);
            return extractLanguageCode(lc_all);
        }

        // Try LANGUAGE environment variable
        const char* language = std::getenv("LANGUAGE");
        if (language != nullptr && language[0] != '\0')
        {
            clog("[Locale::detectSystemLanguage] Found LANGUAGE: ", language);
            return extractLanguageCode(language);
        }

        // Windows fallback (check system locale via environment)
        // This is a simplified approach; more robust Windows detection
        // would require Windows API calls
        const char* windows_lang = std::getenv("USERLANG");
        if (windows_lang != nullptr && windows_lang[0] != '\0')
        {
            clog("[Locale::detectSystemLanguage] Found USERLANG: ", windows_lang);
            return extractLanguageCode(windows_lang);
        }

        // Default to English
        clog("[Locale::detectSystemLanguage] No locale found, defaulting to English");
        return "en";
    }

    /**
     * @brief Extract language code from locale string
     *
     * Examples:
     * - "fr_FR.UTF-8" → "fr"
     * - "en_US" → "en"
     * - "de" → "de"
     * - "fr_FR:en_US" → "fr" (first in colon-separated list)
     *
     * @param locale Full locale string
     * @return Supported language code or "en" if not found
     */
    std::string Locale::extractLanguageCode(const char* locale)
    {
        if (locale == nullptr || locale[0] == '\0')
        {
            return "en";
        }

        // Handle colon-separated list (e.g., "fr_FR:en_US")
        std::string localeStr(locale);
        size_t      colonPos = localeStr.find(':');
        if (colonPos != std::string::npos)
        {
            localeStr = localeStr.substr(0, colonPos);
        }

        // Extract first 2 characters (language code)
        if (localeStr.length() >= 2)
        {
            std::string langCode = localeStr.substr(0, 2);

            // Convert to lowercase
            for (char& c : langCode)
            {
                if (c >= 'A' && c <= 'Z')
                {
                    c = c - 'A' + 'a';
                }
            }

            clog("[Locale::extractLanguageCode] From locale '", locale, "' extracted: ", langCode);

            // Check if this language is supported
            for (const char* supported : SUPPORTED_LANGS)
            {
                if (langCode == supported)
                {
                    clog("[Locale::extractLanguageCode] Language '", langCode, "' is supported");
                    return langCode;
                }
            }
            clog("[Locale::extractLanguageCode] Language '", langCode, "' is NOT supported, falling back to English");
        }

        // Not a supported language, fallback to English
        return "en";
    }

    /**
     * @brief Validate language code and return supported language or fallback
     *
     * @param langCode Language code to validate (e.g., "fr", "en", "es", "de")
     * @return Supported language code or "en" if not found
     */
    std::string Locale::validateLanguage(const char* langCode)
    {
        if (langCode == nullptr || langCode[0] == '\0')
        {
            return "en";
        }

        // Convert to lowercase for comparison
        std::string lowercase = langCode;
        for (char& c : lowercase)
        {
            if (c >= 'A' && c <= 'Z')
            {
                c = c - 'A' + 'a';
            }
        }

        // Extract only first 2 characters
        if (lowercase.length() >= 2)
        {
            lowercase = lowercase.substr(0, 2);
        }

        clog("[Locale::validateLanguage] Input: '", langCode, "' -> Lowercase: '", lowercase, "'");

        // Check if this language is supported
        for (const char* supported : SUPPORTED_LANGS)
        {
            if (lowercase == supported)
            {
                clog("[Locale::validateLanguage] Language '", lowercase, "' is supported");
                return lowercase;
            }
        }

        // Not a supported language, fallback to English
        clog("[Locale::validateLanguage] Language '", lowercase, "' is NOT supported, falling back to English");
        return "en";
    }

    /**
     * @brief Map short language code to full locale name
     *
     * @param lang two-letter language code (e.g., "fr", "en")
     * @return full locale string suitable for setlocale (or empty to use system default)
     */
    std::string Locale::fullLocaleFor(const char* lang)
    {
        if (lang == nullptr || lang[0] == '\0')
            return std::string();

        std::string l = lang;
        for (char& c : l)
            if (c >= 'A' && c <= 'Z')
                c = c - 'A' + 'a';
        if (l.size() >= 2)
            l = l.substr(0, 2);

        if (l == "fr")
            return "fr_FR.UTF-8";
        if (l == "en")
            return "en_US.UTF-8";
        if (l == "es")
            return "es_ES.UTF-8";
        if (l == "de")
            return "de_DE.UTF-8";

        return std::string();
    }

    /**
     * @brief Load translations from JSON file for the specified language
     *
     * @param lang Language code (e.g., "fr", "en", "es", "de")
     */
    void Locale::loadTranslations(const char* lang)
    {
        std::ifstream                                file(s_localePath + "/" + lang + ".json");
        std::unordered_map<std::string, std::string> translations;
        if (!file.is_open())
        {
            clog("[Locale::loadTranslations] Failed to open translation file for language: ", lang);
            return;
        }

        try
        {
            nlohmann::json jsonTranslations;
            file >> jsonTranslations;

            const auto flatten = [&translations](const auto& self, const nlohmann::json& node, std::string& keyPath) -> void {
                if (node.is_object())
                {
                    for (auto it = node.begin(); it != node.end(); ++it)
                    {
                        const std::size_t previousSize = keyPath.size();
                        if (previousSize != 0)
                            keyPath.push_back('.');
                        keyPath.append(it.key());
                        self(self, it.value(), keyPath);
                        keyPath.resize(previousSize);
                    }
                    return;
                }

                if (keyPath.empty())
                    return;

                if (node.is_string())
                    translations.emplace(keyPath, node.get_ref<const std::string&>());
                else if (!node.is_null())
                    translations.emplace(keyPath, node.dump());
            };

            std::string keyPath;
            if (jsonTranslations.is_object())
                translations.reserve(jsonTranslations.size());
            flatten(flatten, jsonTranslations, keyPath);

            s_translations = translations;
        }
        catch (const std::exception& e)
        {
            clog("[Locale::loadTranslations] Error parsing translation file: ", e.what());
        }
    }
} // namespace splinter3D::utils
