#include "platform/settings/settings_registry.hpp"
#include "support/temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <string>
#include <vector>

using platform::settings::Kind;
using platform::settings::Setting;
using platform::settings::SettingsRegistry;
using platform::settings::Subscription;
using tests::support::TempDir;

namespace
{
    Setting toggle(std::string key, std::string defaultValue = "1")
    {
        Setting setting;
        setting.key          = std::move(key);
        setting.section      = "settings.section.general";
        setting.label        = "label";
        setting.kind         = Kind::Toggle;
        setting.defaultValue = std::move(defaultValue);
        return setting;
    }

    Setting choice(std::string key)
    {
        Setting setting;
        setting.key          = std::move(key);
        setting.kind         = Kind::Choice;
        setting.defaultValue = "a";
        setting.choices      = {{"a", "A"}, {"b", "B"}};
        return setting;
    }
} // namespace

TEST_CASE("SettingsRegistry: an unset setting reads its default", "[platform][settings]")
{
    SettingsRegistry registry;
    registry.add(toggle("x"));
    CHECK(registry.get("x") == "1");
    CHECK(registry.isOn("x"));
    CHECK(registry.get("unknown").empty());
}

TEST_CASE("SettingsRegistry: settings are listed in declaration order; redeclaring replaces", "[platform][settings]")
{
    SettingsRegistry registry;
    registry.add(toggle("b"));
    registry.add(toggle("a"));
    registry.set("b", "0");
    registry.add(toggle("b", "1"));

    REQUIRE(registry.settings().size() == 2);
    CHECK(registry.settings()[0].key == "b");
    CHECK(registry.settings()[1].key == "a");
    CHECK(registry.get("b") == "0"); // the value survives
}

TEST_CASE("SettingsRegistry: subscribers hear changes until their subscription goes", "[platform][settings]")
{
    SettingsRegistry         registry;
    std::vector<std::string> heard;
    {
        const Subscription subscription = registry.add(toggle("x"), [&](const std::string& value) { heard.push_back(value); });
        registry.set("x", "0");
        registry.set("x", "0"); // unchanged: not notified
        registry.set("y", "1"); // another key
        registry.reset("x");
    }
    registry.set("x", "0");
    CHECK(heard == std::vector<std::string>{"0", "1"});
}

TEST_CASE("SettingsRegistry: a subscriber may unsubscribe from its callback", "[platform][settings]")
{
    SettingsRegistry registry;
    registry.add(toggle("x"));
    int          calls = 0;
    Subscription subscription;
    subscription = registry.subscribe("x", [&](const std::string&) {
        ++calls;
        subscription.reset();
    });
    registry.set("x", "0");
    registry.set("x", "1");
    CHECK(calls == 1);
}

TEST_CASE("SettingsRegistry: values a setting can't take are ignored", "[platform][settings]")
{
    SettingsRegistry registry;
    registry.add(toggle("t"));
    registry.add(choice("c"));

    registry.set("t", "yes");
    registry.set("c", "z");
    CHECK(registry.get("t") == "1");
    CHECK(registry.get("c") == "a");

    registry.set("c", "b");
    CHECK(registry.get("c") == "b");
}

TEST_CASE("SettingsRegistry: values are saved and loaded back, undeclared ones included", "[platform][settings]")
{
    TempDir    tmp;
    const auto path = tmp.path() / "sub" / "settings.cfg"; // the folder is created on save
    {
        SettingsRegistry registry;
        registry.load(path);
        registry.add(choice("c"));
        registry.set("c", "b");
        registry.set("keys.open", "Ctrl+Shift+O");
    }
    SettingsRegistry registry;
    registry.load(path);
    CHECK(registry.get("keys.open") == "Ctrl+Shift+O");
    CHECK(registry.get("c") == "b");
    registry.add(choice("c"));
    CHECK(registry.get("c") == "b");
}

TEST_CASE("SettingsRegistry: a stored value the setting rejects reads as the default", "[platform][settings]")
{
    TempDir    tmp;
    const auto path = tmp.path() / "settings.cfg";
    std::ofstream(path) << "c nope\r\n\nbroken\n";

    SettingsRegistry registry;
    registry.load(path);
    registry.add(choice("c"));
    CHECK(registry.get("c") == "a");
}
