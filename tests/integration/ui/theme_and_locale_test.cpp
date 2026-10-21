#include "platform/locale/locale_manager.hpp"
#include "support/wx_helpers.hpp"
#include "ui/dialogs/dialogs.hpp"
#include "ui/framework/wx/locale/locale.hpp"
#include "ui/framework/wx/theme/theme_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/settings.h>

using tests::support::ScopedFrame;
using ui::framework::wx::theme::ThemeManager;

namespace
{
    // ThemeManager is application-wide: restore its defaults after each test.
    struct ThemeFixture
    {
        ThemeFixture()
            : frame(nullptr, wxID_ANY, "Theme test")
            , panel(new wxPanel(frame.get()))
            , button(new wxButton(panel, wxID_ANY, "OK"))
        { }

        ~ThemeFixture()
        {
            ThemeManager::get().setDark(false);
            ThemeManager::get().setFontSize(11);
        }

        ScopedFrame<> frame;
        wxPanel*      panel;
        wxButton*     button;
    };
} // namespace

TEST_CASE_METHOD(ThemeFixture, "ThemeManager: dark theme recolors the whole window tree", "[integration][ui][theme]")
{
    auto& theme = ThemeManager::get();
    theme.setDark(true);
    REQUIRE(theme.isDark());

    theme.apply(frame.get());

    for (wxWindow* window : {static_cast<wxWindow*>(frame.get()), static_cast<wxWindow*>(panel),
                             static_cast<wxWindow*>(button)})
    {
        CHECK(window->GetBackgroundColour() == wxColour(30, 30, 30));
        CHECK(window->GetForegroundColour() == wxColour(220, 220, 220));
    }
}

TEST_CASE_METHOD(ThemeFixture, "ThemeManager: light theme uses the system colours", "[integration][ui][theme]")
{
    auto& theme = ThemeManager::get();
    theme.setDark(true);
    theme.apply(frame.get());

    theme.setDark(false);
    theme.apply(frame.get());

    CHECK(button->GetBackgroundColour() == wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    CHECK(button->GetForegroundColour() == wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
}

TEST_CASE_METHOD(ThemeFixture, "ThemeManager: font size applies recursively", "[integration][ui][theme]")
{
    auto& theme = ThemeManager::get();
    theme.setFontSize(17);
    CHECK(theme.getFontSize() == 17);

    theme.applyFontSize(frame.get(), 17);

    CHECK(panel->GetFont().GetPointSize() == 17);
    CHECK(button->GetFont().GetPointSize() == 17);
}

TEST_CASE("ThemeManager: null windows are ignored", "[integration][ui][theme]")
{
    ThemeManager::get().apply(nullptr);
    ThemeManager::get().applyFontSize(nullptr, 12);
    SUCCEED();
}

TEST_CASE("tr(): translates through the application-wide LocaleManager as UTF-8", "[integration][ui][locale]")
{
    auto& locale = platform::locale::LocaleManager::get();
    locale.init(SPLINTER3D_TEST_LOCALE_DIR);
    locale.setLanguage("fr");

    CHECK(ui::framework::wx::locale::tr("quit") == "Quitter");
    CHECK(ui::framework::wx::locale::tr("settings.title") == wxString::FromUTF8("Param\xC3\xA8tres"));
    CHECK(ui::framework::wx::locale::tr("no.such.key") == "no.such.key");

    locale.setLanguage("en");
}

TEST_CASE("dialogs::progress runs every step", "[integration][ui][dialogs]")
{
    ScopedFrame<>    frame(nullptr, wxID_ANY, "Progress test");
    std::vector<int> steps;

    ui::dialogs::progress(frame.get(), "Working", 3, [&steps](wxProgressDialog& dialog, int step) {
        steps.push_back(step);
        dialog.Update(step);
    });

    CHECK(steps == std::vector<int>{0, 1, 2, 3});
}
