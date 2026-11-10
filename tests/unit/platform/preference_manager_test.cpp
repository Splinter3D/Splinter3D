#include "platform/preference/preference_manager.hpp"
#include "support/temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <string>

using platform::preference::Preference;
using platform::preference::PreferenceManager;
using tests::support::TempDir;

namespace
{
    void writeFile(const std::filesystem::path& path, const std::string& content)
    {
        std::ofstream(path) << content;
    }

    void requireDefaults(const Preference& p)
    {
        const Preference defaults;
        CHECK(p.language == defaults.language);
        CHECK(p.darkMode == defaults.darkMode);
        CHECK(p.fontSize == defaults.fontSize);
    }
} // namespace

TEST_CASE("PreferenceManager: a missing file yields defaults", "[platform][preference]")
{
    TempDir tmp;
    requireDefaults(PreferenceManager::load((tmp.path() / "missing.cfg").string()));
}

TEST_CASE("PreferenceManager: save then load round-trips every field", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = (tmp.path() / "splinter3D.cfg").string();
    Preference saved;
    saved.language = "fr";
    saved.darkMode = true;
    saved.fontSize = 14;

    PreferenceManager::save(saved, path);
    const Preference loaded = PreferenceManager::load(path);

    CHECK(loaded.language == "fr");
    CHECK(loaded.darkMode);
    CHECK(loaded.fontSize == 14);
}

TEST_CASE("PreferenceManager: save overwrites the previous file", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = (tmp.path() / "splinter3D.cfg").string();
    Preference first;
    first.fontSize = 20;
    PreferenceManager::save(first, path);

    Preference second;
    second.fontSize = 9;
    PreferenceManager::save(second, path);

    REQUIRE(PreferenceManager::load(path).fontSize == 9);
}

TEST_CASE("PreferenceManager: unknown keys are ignored", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = tmp.path() / "splinter3D.cfg";
    writeFile(path, "unknownKey whatever\nfontSize 16\n");

    const Preference loaded = PreferenceManager::load(path.string());

    CHECK(loaded.fontSize == 16);
    CHECK(loaded.language == Preference{}.language);
}

TEST_CASE("PreferenceManager: corrupt values fall back to defaults", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = tmp.path() / "splinter3D.cfg";

    SECTION("non-numeric font size")
    {
        writeFile(path, "language de\nfontSize big\n");
        const Preference loaded = PreferenceManager::load(path.string());
        CHECK(loaded.language == "de");
        CHECK(loaded.fontSize == Preference{}.fontSize);
    }

    SECTION("font size out of int range")
    {
        writeFile(path, "fontSize 99999999999999999999\n");
        CHECK(PreferenceManager::load(path.string()).fontSize == Preference{}.fontSize);
    }

    SECTION("font size with trailing garbage")
    {
        writeFile(path, "fontSize 12px\n");
        CHECK(PreferenceManager::load(path.string()).fontSize == Preference{}.fontSize);
    }
}

TEST_CASE("PreferenceManager: darkMode is only true for \"1\"", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = tmp.path() / "splinter3D.cfg";

    writeFile(path, "darkMode true\n");
    CHECK_FALSE(PreferenceManager::load(path.string()).darkMode);

    writeFile(path, "darkMode 1\n");
    CHECK(PreferenceManager::load(path.string()).darkMode);
}

TEST_CASE("PreferenceManager: values may contain spaces", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = (tmp.path() / "splinter3D.cfg").string();
    Preference saved;
    saved.language = "pt BR";
    saved.fontSize = 13;

    PreferenceManager::save(saved, path);
    const Preference loaded = PreferenceManager::load(path);

    CHECK(loaded.language == "pt BR");
    // The rest of the line used to be misread as the next key/value pairs.
    CHECK(loaded.fontSize == 13);
}

TEST_CASE("PreferenceManager: tolerates Windows line endings and blank lines", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = tmp.path() / "splinter3D.cfg";
    writeFile(path, "language fr\r\n\r\ndarkMode 1\r\nfontSize 15\r\n");

    const Preference loaded = PreferenceManager::load(path.string());

    CHECK(loaded.language == "fr");
    CHECK(loaded.darkMode);
    CHECK(loaded.fontSize == 15);
}

TEST_CASE("PreferenceManager: a key without a value is ignored", "[platform][preference]")
{
    TempDir    tmp;
    const auto path = tmp.path() / "splinter3D.cfg";
    writeFile(path, "language\nfontSize 12\n");

    const Preference loaded = PreferenceManager::load(path.string());

    CHECK(loaded.language == Preference{}.language);
    CHECK(loaded.fontSize == 12);
}
