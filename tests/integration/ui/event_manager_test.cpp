#include "platform/preference/preference_manager.hpp"
#include "support/temp_dir.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/wx/events/event_manager.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/framework/wx/theme/theme_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include <wx/spinctrl.h>

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
            callbacks.transformControls  = {spin};

            manager = std::make_unique<EventManager>(frame.get(), std::move(callbacks));
            manager->bindAll();
        }

        void pressKey(int keyCode, bool control)
        {
            wxKeyEvent event(wxEVT_CHAR_HOOK);
            event.m_keyCode = keyCode;
            event.SetControlDown(control);
            event.SetEventObject(frame.get());
            frame->ProcessWindowEvent(event);
        }

        // The cwd is a temp dir: theme/language handlers save ./splinter3D.cfg.
        tests::support::TempCwd       cwd;
        ScopedFrame<>                 frame;
        wxSpinCtrlDouble*             spin = nullptr;
        std::unique_ptr<EventManager> manager;
        std::vector<std::string>      fired;
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

TEST_CASE_METHOD(EventManagerFixture, "EventManager: language menu persists the choice", "[integration][ui][events]")
{
    sendMenuCommand(frame.get(), ids::language::kFrench);
    CHECK(platform::preference::PreferenceManager::load().language == "fr");

    sendMenuCommand(frame.get(), ids::language::kDefault);
    CHECK(platform::preference::PreferenceManager::load().language == "default");
}
