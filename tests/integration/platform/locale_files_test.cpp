#include "platform/locale/locale_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// Loads the real locale/*.json files through the C++ LocaleManager (key/placeholder
// consistency between languages is checked separately by scripts/check-locales.py,
// registered as the integration.check-locales CTest).

using platform::locale::LocaleManager;

namespace
{
    const std::filesystem::path kLocaleDir = SPLINTER3D_TEST_LOCALE_DIR;

    void collectKeys(const nlohmann::json& node, const std::string& prefix, std::vector<std::string>& out)
    {
        if (!node.is_object())
        {
            out.push_back(prefix);
            return;
        }
        for (const auto& [key, value] : node.items())
            collectKeys(value, prefix.empty() ? key : prefix + "." + key, out);
    }

    std::vector<std::string> keysOf(const std::string& language)
    {
        std::ifstream            file(kLocaleDir / (language + ".json"));
        std::vector<std::string> keys;
        collectKeys(nlohmann::json::parse(file), "", keys);
        return keys;
    }

    struct RealLocaleFixture
    {
        RealLocaleFixture()
        {
            manager.init(kLocaleDir.string());
        }

        LocaleManager manager;
    };
} // namespace

TEST_CASE_METHOD(RealLocaleFixture, "Locale files: every key of every language is translated", "[integration][locale]")
{
    const std::string language = GENERATE("en", "fr", "de", "es");
    CAPTURE(language);

    manager.setLanguage(language);
    REQUIRE(manager.getLanguage() == language);

    const auto keys = keysOf(language);
    REQUIRE_FALSE(keys.empty());
    for (const auto& key : keys)
    {
        CAPTURE(key);
        // gettext() returns the key itself when the file failed to load or lacks it.
        CHECK(manager.gettext(key) != key);
    }
}

TEST_CASE_METHOD(RealLocaleFixture, "Locale files: non-ASCII text survives loading", "[integration][locale]")
{
    manager.setLanguage("fr");

    // "è" escape in fr.json, decoded to UTF-8.
    REQUIRE(manager.gettext("settings.title") == "Param\xC3\xA8tres");
}
