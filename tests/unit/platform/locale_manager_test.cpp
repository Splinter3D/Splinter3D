#include "platform/locale/locale_manager.hpp"
#include "support/temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fstream>
#include <string>

using platform::locale::LocaleManager;
using tests::support::TempDir;

namespace
{
    // A locale folder with small fixtures, so tests don't depend on the real
    // translations in locale/ (those are checked by scripts/check-locales.py).
    class LocaleFixture
    {
      public:
        LocaleFixture()
        {
            write("en", R"({"quit": "Quit", "settings": {"title": "Settings", "nested": {"deep": "Deep"}}})");
            write("fr", R"({"quit": "Quitter", "settings": {"title": "Paramètres"}})");
            write("de", R"({ this is not json )");
            // "es" is a supported language with no file on purpose.

            manager.init(_dir.path().string());
            manager.setLanguage("en");
        }

        [[nodiscard]] std::string path() const
        {
            return _dir.path().string();
        }

        LocaleFixture(const LocaleFixture&)            = delete;
        LocaleFixture& operator=(const LocaleFixture&) = delete;

      private:
        void write(const std::string& language, const std::string& json) const
        {
            std::ofstream(_dir.path() / (language + ".json")) << json;
        }

        TempDir _dir;

      public:
        // Each test gets its own manager; the application-wide get() is untouched.
        LocaleManager manager;
    };
} // namespace

TEST_CASE_METHOD(LocaleFixture, "LocaleManager: translates top-level keys", "[platform][locale]")
{
    REQUIRE(manager.gettext("quit") == "Quit");
}

TEST_CASE_METHOD(LocaleFixture, "LocaleManager: nested keys are flattened with dots", "[platform][locale]")
{
    CHECK(manager.gettext("settings.title") == "Settings");
    CHECK(manager.gettext("settings.nested.deep") == "Deep");
}

TEST_CASE_METHOD(LocaleFixture, "LocaleManager: unknown keys are returned unchanged", "[platform][locale]")
{
    CHECK(manager.gettext("does.not.exist") == "does.not.exist");
    // An object node is not a translation either.
    CHECK(manager.gettext("settings") == "settings");
}

TEST_CASE_METHOD(LocaleFixture, "LocaleManager: switching language reloads translations", "[platform][locale]")
{
    manager.setLanguage("fr");

    CHECK(manager.getLanguage() == "fr");
    CHECK(manager.gettext("quit") == "Quitter");
    // Keys missing from the new language are not kept from the previous one.
    CHECK(manager.gettext("settings.nested.deep") == "settings.nested.deep");
}

TEST_CASE_METHOD(LocaleFixture, "LocaleManager: language codes are normalized", "[platform][locale]")
{
    auto [input, expected] = GENERATE(table<std::string, std::string>({
        {"FR", "fr"},
        {"fr_FR.UTF-8", "fr"},
        {"de-DE", "de"},
        {"it", "en"},
        {"", "en"},
        {"C.UTF-8", "en"},
    }));
    CAPTURE(input);

    manager.setLanguage(input);

    REQUIRE(manager.getLanguage() == expected);
}

TEST_CASE_METHOD(LocaleFixture, "LocaleManager: unusable locale files leave keys untranslated", "[platform][locale]")
{
    const std::string language = GENERATE("de", "es");
    CAPTURE(language);

    manager.setLanguage(language);

    CHECK(manager.getLanguage() == language);
    CHECK(manager.gettext("quit") == "quit");
}

TEST_CASE_METHOD(LocaleFixture, "LocaleManager: instances are independent", "[platform][locale]")
{
    LocaleManager other;
    other.init(path());
    other.setLanguage("fr");

    CHECK(other.gettext("quit") == "Quitter");
    CHECK(manager.gettext("quit") == "Quit");
    CHECK(manager.getLanguage() == "en");
}

TEST_CASE("LocaleManager: a fresh instance has no translations", "[platform][locale]")
{
    const LocaleManager fresh;

    CHECK(fresh.getLanguage() == "en");
    CHECK(fresh.gettext("quit") == "quit");
}

TEST_CASE("LocaleManager: get() is a single shared instance", "[platform][locale]")
{
    REQUIRE(&LocaleManager::get() == &LocaleManager::get());
}
