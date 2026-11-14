#include "platform/locale/locale_manager.hpp"
#include "platform/preference/preference_manager.hpp"
#include "platform/settings/settings_registry.hpp"
#include "support/temp_dir.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/wx/events/event_manager.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/framework/wx/theme/theme_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include <wx/spinctrl.h>
#include <wx/textentry.h>

namespace ids = ui::framework::wx::ids;
using tests::support::ScopedFrame;
using tests::support::sendMenuCommand;
using ui::framework::wx::events::EventManager;

namespace
{
    // A bare frame with an EventManager whose callbacks record what fired.
    class EventManagerFixture
    {
      public:
        EventManagerFixture()
            : frame(nullptr, wxID_ANY, "EventManager test")
        {
            spin = new wxSpinCtrlDouble(frame.get(), wxID_ANY);

            EventManager::Callbacks callbacks;
            callbacks.onNew              = [this] { fired.emplace_back("new"); };
            callbacks.onOpen             = [this] { fired.emplace_back("open"); };
            callbacks.onSave             = [this] { fired.emplace_back("save"); };
            callbacks.onExit             = [this] { fired.emplace_back("exit"); };
            callbacks.onTransformChanged = [this] { fired.emplace_back("transform"); };
            callbacks.onModelsPanel      = [this] { fired.emplace_back("models"); };
            callbacks.onTasksPanel       = [this] { fired.emplace_back("tasks"); };
            callbacks.onSettingsPanel    = [this] { fired.emplace_back("settings"); };
            callbacks.onDeselect         = [this] {
                fired.emplace_back("deselect");
                return true;
            };
            callbacks.onDeleteSelection = [this] {
                fired.emplace_back("delete");
                return somethingToDelete;
            };
            callbacks.transformControls = {spin};

            manager = std::make_unique<EventManager>(frame.get(), std::move(callbacks));
            manager->bindAll();
        }

        void pressKey(int keyCode, bool control, bool shift = false)
        {
            wxKeyEvent event(wxEVT_CHAR_HOOK);
            event.m_keyCode = keyCode;
            event.SetControlDown(control);
            event.SetShiftDown(shift);
            event.SetEventObject(frame.get());
            frame->ProcessWindowEvent(event);
        }

        // The cwd is a temp dir: theme/language handlers save ./splinter3D.cfg.
        tests::support::TempCwd       cwd;
        ScopedFrame<>                 frame;
        wxSpinCtrlDouble*             spin = nullptr;
        std::unique_ptr<EventManager> manager;
        std::vector<std::string>      fired;
        bool                          somethingToDelete = true;
    };
} // namespace

TEST_CASE_METHOD(EventManagerFixture, "EventManager: File menu commands reach their callbacks", "[integration][ui][events]")
{
    sendMenuCommand(frame.get(), ids::file::kNew);
    sendMenuCommand(frame.get(), ids::file::kOpen);
    sendMenuCommand(frame.get(), ids::file::kSave);
    sendMenuCommand(frame.get(), ids::file::kExit);

    REQUIRE(fired == std::vector<std::string>{"new", "open", "save", "exit"});
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: keyboard shortcuts trigger menu commands", "[integration][ui][events]")
{
    pressKey('S', true);
    pressKey('O', true);
    pressKey('Q', true);

    CHECK(fired == std::vector<std::string>{"save", "open", "exit"});

    fired.clear();
    pressKey('S', false); // no Ctrl: plain typing
    CHECK(fired.empty());
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: shortcuts follow their settings", "[integration][ui][events]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    REQUIRE(settings.find("keys.save") != nullptr);
    settings.set("keys.save", "Ctrl+E");

    pressKey('S', true);
    CHECK(fired.empty());
    pressKey('E', true);
    CHECK(fired == std::vector<std::string>{"save"});

    settings.reset("keys.save");
    fired.clear();
    pressKey('S', true);
    CHECK(fired == std::vector<std::string>{"save"});
}

#if defined(__WXOSX__)
TEST_CASE_METHOD(EventManagerFixture, "EventManager: on macOS, Control+key is not Command+key", "[integration][ui][events]")
{
    wxKeyEvent control(wxEVT_CHAR_HOOK);
    control.m_keyCode = 'S';
    control.SetRawControlDown(true);
    control.SetEventObject(frame.get());
    frame->ProcessWindowEvent(control);
    CHECK(fired.empty()); // Save is Command+S

    platform::settings::SettingsRegistry::getInstance().set("keys.save", "RawCtrl+S");
    frame->ProcessWindowEvent(control);
    CHECK(fired == std::vector<std::string>{"save"});
}
#endif

TEST_CASE_METHOD(EventManagerFixture, "EventManager: editing a transform field notifies", "[integration][ui][events]")
{
    spin->SetValue(12.5);
    wxSpinDoubleEvent event(wxEVT_SPINCTRLDOUBLE, spin->GetId(), 12.5);
    event.SetEventObject(spin);
    spin->HandleWindowEvent(event);

    REQUIRE(fired == std::vector<std::string>{"transform"});
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: theme menu applies and persists the theme", "[integration][ui][events]")
{
    auto& theme = ui::framework::wx::theme::ThemeManager::get();

    sendMenuCommand(frame.get(), ids::theme::kDarkMode);
    CHECK(theme.isDark());
    CHECK(platform::preference::PreferenceManager::load().darkMode);

    sendMenuCommand(frame.get(), ids::theme::kLightMode);
    CHECK_FALSE(theme.isDark());
    CHECK_FALSE(platform::preference::PreferenceManager::load().darkMode);
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: language menu sets the language setting", "[integration][ui][events]")
{
    const auto& settings = platform::settings::SettingsRegistry::getInstance();
    sendMenuCommand(frame.get(), ids::language::kFrench);
    CHECK(settings.get(platform::locale::kLanguageSetting) == "fr");

    sendMenuCommand(frame.get(), ids::language::kDefault);
    CHECK(settings.get(platform::locale::kLanguageSetting) == "default");
}

namespace
{
#if defined(__WXOSX__)
    constexpr int kDeleteKey = WXK_BACK;
#else
    constexpr int kDeleteKey = WXK_DELETE;
#endif
} // namespace

TEST_CASE_METHOD(EventManagerFixture, "EventManager: the delete shortcut calls its action", "[integration][ui][events][shortcuts]")
{
    REQUIRE(platform::settings::SettingsRegistry::getInstance().find("keys.delete_model") != nullptr);
    pressKey(kDeleteKey, false);
    CHECK(fired == std::vector<std::string>{"delete"});

    fired.clear();
    pressKey('O', true); // Open, not the models panel
    pressKey('O', true, true);
    CHECK(fired == std::vector<std::string>{"open", "models"});
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: each side dock page has a shortcut", "[integration][ui][events][shortcuts]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    CHECK(settings.get("keys.model_management") == "Ctrl+Shift+O");
    CHECK(settings.get("keys.tasks_panel") == "Ctrl+Shift+T");
    CHECK(settings.get("keys.settings_panel") == "Ctrl+,");

    pressKey('O', true, true);
    pressKey('T', true, true);
    pressKey(',', true);
    CHECK(fired == std::vector<std::string>{"models", "tasks", "settings"});
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: bare keys are left to text fields", "[integration][ui][events][shortcuts]")
{
    frame.showAndWait();
    spin->SetFocus();
    tests::support::pumpEvents();
    REQUIRE(dynamic_cast<wxTextEntry*>(wxWindow::FindFocus()) != nullptr);

    pressKey(kDeleteKey, false); // edits the field
    CHECK(fired.empty());

    pressKey('S', true); // a shortcut with a modifier still works there
    CHECK(fired == std::vector<std::string>{"save"});
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: the delete key goes on when there is nothing to delete", "[integration][ui][events][shortcuts]")
{
    wxKeyEvent key(wxEVT_CHAR_HOOK);
    key.m_keyCode = kDeleteKey;
    key.SetEventObject(frame.get());

    frame->ProcessWindowEvent(key);
    CHECK_FALSE(key.GetSkipped()); // deleted: handled

    somethingToDelete = false;
    wxKeyEvent again(key);
    again.Skip(false);
    frame->ProcessWindowEvent(again);
    CHECK(again.GetSkipped()); // nothing selected: the focused control gets it
}

TEST_CASE_METHOD(EventManagerFixture, "EventManager: Escape deselects, even from a text field", "[integration][ui][events][shortcuts]")
{
    REQUIRE(platform::settings::SettingsRegistry::getInstance().find("keys.deselect") != nullptr);
    pressKey(WXK_ESCAPE, false);
    CHECK(fired == std::vector<std::string>{"deselect"});

    frame.showAndWait();
    spin->SetFocus();
    tests::support::pumpEvents();
    REQUIRE(dynamic_cast<wxTextEntry*>(wxWindow::FindFocus()) != nullptr);
    pressKey(WXK_ESCAPE, false);
    CHECK(fired == std::vector<std::string>{"deselect", "deselect"});
}
