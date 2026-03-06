/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** Locale management for i18n with gettext
*/

#pragma once

#include <Splinter3D/Utils/OSCompatibility.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <libintl.h>
#include <string>

// Safe getenv wrapper that avoids MSVC deprecation warnings
static inline std::string getenv_string(const char* name)
{
#if defined(SPLINTER3D_WINDOWS)
    char*  buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, name) == 0 && buf != nullptr)
    {
        std::string s(buf);
        free(buf);
        return s;
    }
    return std::string();
#else
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string();
#endif
}

namespace splinter3D::utils
{
    class Locale
    {
      public:
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
        static void init(const char* domainName, const char* localePath);

        /**
         * @brief Initialize i18n with a specific language override
         *
         * @param domainName The gettext domain name (e.g., "splinter3D")
         * @param localePath Path to locale directory (e.g., "./locale")
         * @param forceLang Language code to force (e.g., "fr", "en", "es", "de")
         */
        static void init(const char* domainName, const char* localePath, const char* forceLang);

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
        static void setLanguage(const char* langCode);

      private:
        static constexpr const char* SUPPORTED_LANGS[]   = {"fr", "en", "es", "de"};
        static constexpr size_t      NUM_SUPPORTED_LANGS = 4;
        static std::string           s_domain;
        static std::string           s_localePath;

        /**
         * @brief Check if a translation exists for a language
         *
         * @param lang language code (e.g., "fr")
         * @return true if directory locale/<lang>/LC_MESSAGES/ exists
         */
        static bool hasTranslation(const char* lang);

        /**
         * @brief Detect system language from environment
         * @return Language code (e.g., "fr", "en", "es", "de") or "en" as fallback
         */
        static std::string detectSystemLanguage();

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
        static std::string extractLanguageCode(const char* locale);

        /**
         * @brief Validate language code and return supported language or fallback
         *
         * @param langCode Language code to validate (e.g., "fr", "en", "es", "de")
         * @return Supported language code or "en" if not found
         */
        static std::string validateLanguage(const char* langCode);

        /**
         * @brief Map short language code to full locale name
         *
         * @param lang two-letter language code (e.g., "fr", "en")
         * @return full locale string suitable for setlocale (or empty to use system default)
         */
        static std::string fullLocaleFor(const char* lang);
    };
} // namespace splinter3D::utils

#define _(String) gettext(String)
