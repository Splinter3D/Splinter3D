#include "platform/settings/settings_registry.hpp"
#include "support/wx_helpers.hpp"
#include "ui/icons/Icons.hpp"
#include "ui/panels/models_panel/models_panel.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>

using tests::support::findDescendant;
using tests::support::pumpEvents;
using tests::support::ScopedFrame;
using ui::panels::ModelsPanel;

namespace
{
    // A panel over four models, its selection kept here as the viewer would.
    struct ModelsFixture
    {
        ScopedFrame<>            frame{nullptr, wxID_ANY, "ModelsPanel test", wxDefaultPosition, wxSize(400, 300)};
        ModelsPanel*             panel = nullptr;
        wxListCtrl*              list  = nullptr;
        std::vector<int>         selected;
        std::vector<bool>        visible{true, true, true, true};
        std::vector<std::string> calls; // what the buttons asked, e.g. "hide 0 2"

        ModelsFixture()
        {
            ModelsPanel::Actions actions;
            actions.names         = [] { return std::vector<wxString>{"a", "b", "c", "d"}; };
            actions.visibility    = [this] { return visible; };
            actions.selected      = [this] { return selected; };
            actions.setVisibility = [this](int index, bool shown) { visible[static_cast<std::size_t>(index)] = shown; };
            actions.select        = [this](std::vector<int> indices) {
                selected = std::move(indices);
                panel->ShowSelection();
            };
            actions.toggleSelected = [this](int index) {
                const auto found = std::find(selected.begin(), selected.end(), index);
                if (found != selected.end())
                    selected.erase(found);
                else
                    selected.push_back(index);
                panel->ShowSelection();
            };
            const auto record = [this](const char* what) {
                return [this, what](std::vector<int> indices) {
                    std::string call = what;
                    for (const int index : indices)
                        call += " " + std::to_string(index);
                    calls.push_back(call);
                };
            };
            actions.hide       = record("hide");
            actions.removeMany = record("delete");
            actions.showOnly   = record("show only");
            actions.duplicate  = record("duplicate");
            actions.showAll    = [this] { calls.emplace_back("show all"); };
            panel              = new ModelsPanel(frame.get(), std::move(actions));
            auto* sizer        = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(panel, 1, wxEXPAND);
            frame->SetSizer(sizer);
            frame->Show();
            pumpEvents();
            list = findDescendant<wxListCtrl>(panel);
            REQUIRE(list != nullptr);
        }

        // A click on row's name (or in its eye column), with modifiers.
        void click(int row, bool command = false, bool shift = false, int column = 0)
        {
            wxRect rect;
            REQUIRE(list->GetItemRect(row, rect));
            int x = rect.x + 4;
            for (int c = 0; c < column; ++c)
                x += list->GetColumnWidth(c);
            wxMouseEvent event(wxEVT_LEFT_DOWN);
            event.m_x = x;
            event.m_y = rect.y + rect.height / 2;
            event.SetControlDown(command); // Command on macOS
            event.SetShiftDown(shift);
            event.SetEventObject(list);
            list->ProcessWindowEvent(event);
        }

        wxButton* button(const wxString& label) const
        {
            for (wxWindow* child : panel->GetChildren())
                if (auto* found = dynamic_cast<wxButton*>(child); found != nullptr && found->GetLabel() == label)
                    return found;
            return nullptr;
        }

        void press(wxButton* target)
        {
            wxCommandEvent event(wxEVT_BUTTON, target->GetId());
            event.SetEventObject(target);
            target->ProcessWindowEvent(event);
        }

        std::vector<int> highlighted() const
        {
            std::vector<int> rows;
            for (int row = 0; row < list->GetItemCount(); ++row)
                if (list->GetItemState(row, wxLIST_STATE_SELECTED) != 0)
                    rows.push_back(row);
            return rows;
        }
    };
} // namespace

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: rows are tall enough for the eye and trash icons", "[integration][ui][models]")
{
    REQUIRE(list->GetItemCount() == 4);
    wxRect first;
    wxRect second;
    REQUIRE(list->GetItemRect(0, first));
    REQUIRE(list->GetItemRect(1, second));
    const int icon = list->FromDIP(ui::icons::svgIconSize);
    CHECK(first.GetHeight() >= icon);
    CHECK(second.GetTop() - first.GetTop() >= icon);
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: a click selects just that model", "[integration][ui][models][selection]")
{
    click(1);
    CHECK(selected == std::vector<int>{1});
    click(2);
    CHECK(selected == std::vector<int>{2});
    CHECK(highlighted() == std::vector<int>{2});
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: Command/Ctrl-click adds or removes a model", "[integration][ui][models][selection]")
{
    click(0);
    click(2, true);
    CHECK(selected == std::vector<int>{0, 2});
    CHECK(highlighted() == std::vector<int>{0, 2});
    click(0, true);
    CHECK(selected == std::vector<int>{2});
    CHECK(highlighted() == std::vector<int>{2});
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: Shift-click selects the range from the last clicked row", "[integration][ui][models][selection]")
{
    click(1);
    click(3, false, true);
    CHECK(selected == std::vector<int>{1, 2, 3});
    CHECK(highlighted() == std::vector<int>{1, 2, 3});

    click(0, false, true); // same anchor, other way
    CHECK(selected == std::vector<int>{0, 1});

    click(3, true); // Command-click moves the anchor
    click(2, false, true);
    CHECK(selected == std::vector<int>{2, 3});
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: Shift-click before any click selects just that row", "[integration][ui][models][selection]")
{
    click(2, false, true);
    CHECK(selected == std::vector<int>{2});
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: a click below the rows clears the selection", "[integration][ui][models][selection]")
{
    click(1);
    wxRect last;
    REQUIRE(list->GetItemRect(3, last));
    wxMouseEvent event(wxEVT_LEFT_DOWN);
    event.m_x = last.x + 4;
    event.m_y = last.GetBottom() + last.height * 2;
    event.SetEventObject(list);
    list->ProcessWindowEvent(event);
    CHECK(selected.empty());
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: the eye and trash columns don't change the selection", "[integration][ui][models][selection]")
{
    click(0);
    click(1, false, false, 1); // eye
    CHECK(selected == std::vector<int>{0});
    CHECK_FALSE(visible[1]);
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: shows a selection made elsewhere", "[integration][ui][models][selection]")
{
    selected = {3, 1};
    panel->ShowSelection();
    CHECK(highlighted() == std::vector<int>{1, 3});
    selected.clear();
    panel->ShowSelection();
    CHECK(highlighted().empty());
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: Hide, Delete and Show only act on the selection", "[integration][ui][models][selection]")
{
    auto* hide     = button("Hide");
    auto* remove   = button("Delete");
    auto* showOnly = button("Show only");
    auto* copy     = button("Duplicate");
    REQUIRE(copy != nullptr);
    REQUIRE(hide != nullptr);
    REQUIRE(remove != nullptr);
    REQUIRE(showOnly != nullptr);
    CHECK_FALSE(hide->IsEnabled()); // nothing selected
    CHECK_FALSE(remove->IsEnabled());
    CHECK_FALSE(showOnly->IsEnabled());

    click(2);
    click(0, true);
    CHECK(hide->IsEnabled());
    CHECK(remove->IsEnabled());
    CHECK(showOnly->IsEnabled());
    press(hide);
    press(remove);
    press(showOnly);
    press(copy);
    CHECK(calls == std::vector<std::string>{"hide 2 0", "delete 2 0", "show only 2 0", "duplicate 2 0"});

    selected.clear(); // cleared elsewhere
    panel->ShowSelection();
    CHECK_FALSE(hide->IsEnabled());
    CHECK_FALSE(copy->IsEnabled());
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: hidden models can be clicked, but ranges skip them", "[integration][ui][models][selection]")
{
    visible = {true, false, true, true};
    panel->RefreshModelRows();

    click(1);
    CHECK(selected == std::vector<int>{1});
    click(3, true);
    CHECK(selected == std::vector<int>{1, 3});

    click(0);
    click(3, false, true);
    CHECK(selected == std::vector<int>{0, 2, 3});

    click(1);              // a hidden row as the anchor
    click(2, false, true); // the range still leaves it out
    CHECK(selected == std::vector<int>{2});
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: Show all works without a selection", "[integration][ui][models]")
{
    auto* showAll = button("Show all");
    REQUIRE(showAll != nullptr);
    CHECK(showAll->IsEnabled());
    press(showAll);
    CHECK(calls == std::vector<std::string>{"show all"});
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: with \"Include hidden\", ranges take hidden models too", "[integration][ui][models][selection]")
{
    visible = {true, false, false, true};
    panel->RefreshModelRows();
    auto* include = findDescendant<wxCheckBox>(panel);
    REQUIRE(include != nullptr);
    CHECK_FALSE(include->GetValue());

    click(0);
    click(3, false, true);
    CHECK(selected == std::vector<int>{0, 3});

    include->SetValue(true);
    wxCommandEvent event(wxEVT_CHECKBOX, include->GetId());
    event.SetInt(1);
    event.SetEventObject(include);
    include->ProcessWindowEvent(event);
    click(3, false, true);
    CHECK(selected == std::vector<int>{0, 1, 2, 3});
}

TEST_CASE_METHOD(ModelsFixture, "ModelsPanel: \"Include hidden\" is a saved setting", "[integration][ui][models][settings]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    REQUIRE(settings.find(ui::panels::kIncludeHiddenSetting) != nullptr);
    CHECK(settings.get(ui::panels::kIncludeHiddenSetting) == "0");
    auto* include = findDescendant<wxCheckBox>(panel);
    REQUIRE(include != nullptr);

    include->SetValue(true);
    wxCommandEvent event(wxEVT_CHECKBOX, include->GetId());
    event.SetInt(1);
    event.SetEventObject(include);
    include->ProcessWindowEvent(event);
    CHECK(settings.get(ui::panels::kIncludeHiddenSetting) == "1");

    settings.set(ui::panels::kIncludeHiddenSetting, "0"); // e.g. from the settings panel
    CHECK_FALSE(include->GetValue());
    visible = {true, false, true, true};
    panel->RefreshModelRows();
    click(0);
    click(2, false, true);
    CHECK(selected == std::vector<int>{0, 2});

    settings.set(ui::panels::kIncludeHiddenSetting, "1");
    CHECK(include->GetValue());
    click(2, false, true);
    CHECK(selected == std::vector<int>{0, 1, 2});
}
