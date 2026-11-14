#include "platform/settings/settings_registry.hpp"
#include "support/wx_helpers.hpp"
#include "ui/panels/settings_panel/settings_panel.hpp"
#include "ui/panels/side_dock/side_dock.hpp"
#include "ui/widgets/key_bind_button/key_bind_button.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>

using platform::settings::Kind;
using platform::settings::SettingsRegistry;
using tests::support::findDescendant;
using tests::support::pumpEvents;
using tests::support::ScopedFrame;
using ui::panels::SettingsPanel;
using ui::panels::SideDock;
using ui::widgets::KeyBindButton;

namespace
{
    template <typename T>
    std::vector<T*> findAll(wxWindow* root)
    {
        std::vector<T*> found;
        for (wxWindow* child : root->GetChildren())
        {
            if (auto* match = dynamic_cast<T*>(child))
                found.push_back(match);
            const auto nested = findAll<T>(child);
            found.insert(found.end(), nested.begin(), nested.end());
        }
        return found;
    }

    void pressKey(wxWindow* target, int keyCode, bool control = false, bool shift = false)
    {
        wxKeyEvent event(wxEVT_CHAR_HOOK);
        event.m_keyCode = keyCode;
        event.SetControlDown(control);
        event.SetShiftDown(shift);
        event.SetEventObject(target);
        target->ProcessWindowEvent(event);
    }

    struct SettingsFixture
    {
        ScopedFrame<>     frame{nullptr, wxID_ANY, "SettingsPanel test", wxDefaultPosition, wxSize(640, 480)};
        SettingsRegistry& settings = SettingsRegistry::getInstance();
        SettingsPanel*    panel    = nullptr;

        SettingsFixture()
        {
            settings.add({"test.toggle", "settings.section.general", "settings.tasks.auto_open", "", Kind::Toggle, "1", {}});
            settings.add({"test.choice", "settings.section.import", "settings.import.precision", "settings.import.precision_note", Kind::Choice, "single", {{"single", "settings.import.precision_single"}, {"double", "settings.import.precision_double"}}});
            settings.add({"test.key.a", "settings.section.keys", "settings.keys.save", "", Kind::KeyBind, "Ctrl+S", {}});
            settings.add({"test.key.b", "settings.section.keys", "settings.keys.open", "", Kind::KeyBind, "Ctrl+O", {}});

            panel       = new SettingsPanel(frame.get());
            auto* sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(panel, 1, wxEXPAND);
            frame->SetSizer(sizer);
            frame->Show();
            pumpEvents();
        }
    };
} // namespace

TEST_CASE_METHOD(SettingsFixture, "SettingsPanel: a checkbox edits a toggle, and follows it", "[integration][ui][settings]")
{
    auto* box = findDescendant<wxCheckBox>(panel);
    REQUIRE(box != nullptr);
    CHECK(box->GetValue());

    box->SetValue(false);
    wxCommandEvent event(wxEVT_CHECKBOX, box->GetId());
    event.SetInt(0);
    event.SetEventObject(box);
    box->ProcessWindowEvent(event);
    CHECK(settings.get("test.toggle") == "0");

    settings.set("test.toggle", "1");
    CHECK(box->GetValue());
}

TEST_CASE_METHOD(SettingsFixture, "SettingsPanel: a drop-down edits a choice", "[integration][ui][settings]")
{
    auto* choice = findDescendant<wxChoice>(panel);
    REQUIRE(choice != nullptr);
    REQUIRE(choice->GetCount() == 2);
    CHECK(choice->GetString(1) == "Double");
    CHECK(choice->GetSelection() == 0);

    choice->SetSelection(1);
    wxCommandEvent event(wxEVT_CHOICE, choice->GetId());
    event.SetInt(1);
    event.SetEventObject(choice);
    choice->ProcessWindowEvent(event);
    CHECK(settings.get("test.choice") == "double");
}

TEST_CASE_METHOD(SettingsFixture, "SettingsPanel: a key recorder takes the next combination; Escape cancels", "[integration][ui][settings]")
{
    auto keys = findAll<KeyBindButton>(panel);
    REQUIRE(keys.size() == 2);
    auto* save = keys[0];
    CHECK(save->GetValue() == "Ctrl+S");

    save->StartCapture();
    pressKey(save, WXK_ESCAPE);
    CHECK_FALSE(save->IsCapturing());
    CHECK(settings.get("test.key.a") == "Ctrl+S");

    save->StartCapture();
    CHECK(save->IsCapturing());
    pressKey(save, WXK_SHIFT, false, true); // a modifier alone: still waiting
    CHECK(save->IsCapturing());
    pressKey(save, 'E', true, true);
    CHECK_FALSE(save->IsCapturing());
    CHECK(settings.get("test.key.a") == "Ctrl+Shift+E");

    settings.reset("test.key.a");
    CHECK(save->GetValue() == "Ctrl+S");
}

TEST_CASE_METHOD(SettingsFixture, "SettingsPanel: a combination used twice is flagged", "[integration][ui][settings]")
{
    auto keys = findAll<KeyBindButton>(panel);
    REQUIRE(keys.size() == 2);
    CHECK(keys[1]->GetToolTipText().empty());

    settings.set("test.key.b", "Ctrl+S");
    CHECK_FALSE(keys[0]->GetToolTipText().empty());
    CHECK_FALSE(keys[1]->GetToolTipText().empty());

    settings.reset("test.key.b");
    CHECK(keys[1]->GetToolTipText().empty());
}

TEST_CASE("SettingsPanel: long labels wrap or shorten; nothing leaves the side dock", "[integration][ui][settings]")
{
    auto& settings = SettingsRegistry::getInstance();
    settings.add({"test.toggle.long", "settings.section.general", "Open the tasks panel whenever a long task starts, far too long to fit on one line", "", Kind::Toggle, "0", {}});
    settings.add({"test.choice.long", "settings.section.general", "A drop-down label that is far too long to fit on one line of the side dock", "", Kind::Choice, "a", {{"a", "settings.an_option_far_too_long_to_fit_in_the_drop_down_of_the_side_dock_panel"}}});
    settings.add({"test.key.long", "settings.section.keys", "settings.keys.a_label_far_too_long_to_ever_fit_next_to_its_shortcut_in_the_side_dock", "", Kind::KeyBind, "Ctrl+Shift+Alt+F12", {}});
    settings.add({"test.key.short", "settings.section.keys", "settings.keys.save", "", Kind::KeyBind, "Ctrl+S", {}});

    ScopedFrame<> frame{nullptr, wxID_ANY, "SettingsPanel dock test", wxDefaultPosition, wxSize(900, 300)};
    auto*         dock  = new SideDock(frame.get());
    auto*         panel = new SettingsPanel(dock);
    dock->AddPage(panel, wxBitmap(), "Settings");
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(new wxPanel(frame.get()), 1, wxEXPAND);
    sizer->Add(dock, 0, wxEXPAND);
    frame->SetSizer(sizer);
    frame->Show();
    dock->Open(panel);
    pumpEvents();

    std::vector<wxButton*> resets;
    for (auto* button : findAll<wxButton>(panel))
        if (dynamic_cast<KeyBindButton*>(button) == nullptr)
            resets.push_back(button);
    REQUIRE(resets.size() == 2);
    const int visible = panel->GetClientSize().GetWidth();
    CHECK(panel->GetVirtualSize().GetWidth() <= visible);
    for (wxWindow* child : panel->GetChildren())
    {
        INFO(child->GetLabel().utf8_string());
        CHECK(child->GetRect().GetLeft() >= 0);
        CHECK(child->GetRect().GetRight() < visible);
    }
    // The reset buttons line up in one column.
    CHECK(resets[0]->GetPosition().x == resets[1]->GetPosition().x);
}

TEST_CASE_METHOD(SettingsFixture, "SettingsPanel: settings declared later show once it is shown again", "[integration][ui][settings]")
{
    settings.add({"test.late", "settings.section.general", "settings.import.compute_brep", "", Kind::Toggle, "0", {}});
    CHECK(findAll<wxCheckBox>(panel).size() == 1);

    panel->Hide();
    panel->Show();
    pumpEvents();
    CHECK(findAll<wxCheckBox>(panel).size() == 2);
}

TEST_CASE("Key binds: Command and Control are told apart on macOS", "[integration][ui][settings]")
{
    wxKeyEvent command(wxEVT_CHAR_HOOK);
    command.m_keyCode = 'E';
    command.SetControlDown(true); // Command on macOS, Control elsewhere

    wxKeyEvent control(wxEVT_CHAR_HOOK);
    control.m_keyCode = 'E';
    control.SetRawControlDown(true); // the Control key (elsewhere, the same as above)
    control.SetShiftDown(true);

    CHECK(ui::widgets::keyBindOf(command) == "Ctrl+E");
    CHECK(ui::widgets::matchesKeyBind(command, "Ctrl+E"));
#if defined(__WXOSX__)
    CHECK(ui::widgets::keyBindOf(control) == "Shift+RawCtrl+E");
    CHECK_FALSE(ui::widgets::matchesKeyBind(control, "Ctrl+Shift+E"));
    CHECK(ui::widgets::matchesKeyBind(control, "RawCtrl+Shift+E")); // any modifier order
    CHECK(ui::widgets::formatKeyBind("Ctrl+E") == wxString::FromUTF8("⌘E"));
    CHECK(ui::widgets::formatKeyBind("RawCtrl+Alt+Shift+Ctrl+F11") == wxString::FromUTF8("⌃⌥⇧⌘F11"));
#else
    CHECK(ui::widgets::keyBindOf(control) == "Ctrl+Shift+E");
    CHECK(ui::widgets::formatKeyBind("Ctrl+Shift+E") == "Ctrl+Shift+E");
#endif
    CHECK(ui::widgets::formatKeyBind("").empty());
}

TEST_CASE_METHOD(SettingsFixture, "SettingsPanel: the recorder keeps Command and Control apart", "[integration][ui][settings]")
{
    auto* save = findAll<KeyBindButton>(panel)[0];
    save->StartCapture();
    wxKeyEvent event(wxEVT_CHAR_HOOK);
    event.m_keyCode = 'K';
    event.SetRawControlDown(true);
    event.SetEventObject(save);
    save->ProcessWindowEvent(event);
#if defined(__WXOSX__)
    CHECK(settings.get("test.key.a") == "RawCtrl+K");
    CHECK(save->GetLabel() == wxString::FromUTF8("⌃K"));
#else
    CHECK(settings.get("test.key.a") == "Ctrl+K");
    CHECK(save->GetLabel() == "Ctrl+K");
#endif
}
