#pragma once
#include <string>
#include <wx/intl.h>

namespace wx::locale
{

    // ─────────────────────────────────────────────
    //  Locale — wraps wxLocale lifecycle + catalog loading
    //
    //  Usage:
    //    wx::Locale::init("fr", "splinter3D");   // init + load catalog
    //    wx::Locale::init("default");            // system default
    // ─────────────────────────────────────────────
    class Locale
    {
      public:
        // Init (or re-init) the locale for the given language code.
        // Destroys the previous wxLocale instance first so gettext resets cleanly.
        // catalogName  : the .mo file base name  (e.g. "splinter3D")
        // catalogPath  : folder that contains the locale/ tree (default: "./locale")
        static void init(const std::string& code,
                         const std::string& catalogName = "",
                         const std::string& catalogPath = "./locale")
        {
            auto& self = get();
            delete self.m_locale;
            self.m_locale = nullptr;

            wxLanguage lang = codeToLang(code);

            self.m_locale = new wxLocale();
            if (!self.m_locale->Init(lang, wxLOCALE_DONT_LOAD_DEFAULT))
            {
                // Fallback to English if the requested locale is unavailable
                delete self.m_locale;
                self.m_locale = new wxLocale();
                self.m_locale->Init(wxLANGUAGE_ENGLISH, wxLOCALE_DONT_LOAD_DEFAULT);
            }

            if (!catalogName.empty())
            {
                self.m_locale->AddCatalogLookupPathPrefix(catalogPath);
                self.m_locale->AddCatalog(catalogName);
            }
        }

        // Destroy the current locale (call on app shutdown if needed)
        static void destroy()
        {
            auto& self = get();
            delete self.m_locale;
            self.m_locale = nullptr;
        }

      private:
        wxLocale* m_locale = nullptr;

        Locale() = default;

        ~Locale()
        {
            delete m_locale;
        }

        static Locale& get()
        {
            static Locale instance;
            return instance;
        }

        static wxLanguage codeToLang(const std::string& code)
        {
            if (code == "en")
                return wxLANGUAGE_ENGLISH;
            if (code == "fr")
                return wxLANGUAGE_FRENCH;
            if (code == "es")
                return wxLANGUAGE_SPANISH;
            if (code == "de")
                return wxLANGUAGE_GERMAN;
            return wxLANGUAGE_DEFAULT;
        }
    };

} // namespace wx