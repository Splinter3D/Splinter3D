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
#include <libintl.h>
#include <string>

namespace splinter3D::utils
{
    // Static member definitions
    std::string Locale::s_domain;
    std::string Locale::s_localePath;

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
     * @param domainName The gettext domain name (e.g., "splinter3D")
     * @param localePath Path to locale directory (e.g., "./locale")
     */
    void Locale::init(const char* domainName, const char* localePath)
    {
        s_domain     = domainName;
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

        bindtextdomain(domainName, localePath);
        bind_textdomain_codeset(domainName, "UTF-8");
        textdomain(domainName);

        clog("[Locale::init] Domain: ", domainName);
        clog("[Locale::init] Path: ", localePath);
        clog("[Locale::init] Detected language from system: ", active);
        clog("[Locale::init] bindtextdomain result: ", bindtextdomain(domainName, nullptr));
    }

    /**
     * @brief Initialize i18n with a specific language override
     *
     * @param domainName The gettext domain name (e.g., "splinter3D")
     * @param localePath Path to locale directory (e.g., "./locale")
     * @param forceLang Language code to force (e.g., "fr", "en", "es", "de")
     */
    void Locale::init(const char* domainName, const char* localePath, const char* forceLang)
    {
        s_domain     = domainName;
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

        bindtextdomain(domainName, localePath);
        bind_textdomain_codeset(domainName, "UTF-8");
        textdomain(domainName);

        clog("[Locale::init(forced)] Domain: ", domainName);
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

        // Get current domain and rebind to reload translations
        const char* currentDomain = textdomain(nullptr);
        if (currentDomain != nullptr)
        {
            const char* currentPath = bindtextdomain(currentDomain, nullptr);
            if (currentPath != nullptr)
            {
                // Re-bind to force reload with new LANGUAGE
                bindtextdomain(currentDomain, currentPath);
                bind_textdomain_codeset(currentDomain, "UTF-8");
                textdomain(currentDomain);
            }
        }

        clog("[Locale::setLanguage] Requested: ", langCode);
        clog("[Locale::setLanguage] Validated to: ", validLang);
        clog("[Locale::setLanguage] Saved to ACTIVE_LANG");
        clog("[Locale::setLanguage] Current domain: ", (currentDomain ? currentDomain : "none"));
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

        std::string   dir = s_localePath + "/" + lang + "/LC_MESSAGES";
        std::ifstream test(dir + "/" + s_domain + ".mo");
        return test.good();
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
} // namespace splinter3D::utils
