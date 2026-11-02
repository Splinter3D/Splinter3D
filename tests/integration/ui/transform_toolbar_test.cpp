#include "support/wx_helpers.hpp"
#include "ui/toolbars/transform_toolbar/transform_toolbar.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <wx/combobox.h>
#include <wx/tglbtn.h>

using tests::support::ScopedFrame;
using ui::toolbars::TransformToolbar;
using ActiveTool = TransformToolbar::ActiveTool;

namespace
{
    std::vector<wxWindow*> childrenOfType(wxWindow* root, const wxClassInfo* type)
    {
        std::vector<wxWindow*> found;
        for (wxWindow* child : root->GetChildren())
        {
            if (child->IsKindOf(type))
                found.push_back(child);
            const auto nested = childrenOfType(child, type);
            found.insert(found.end(), nested.begin(), nested.end());
        }
        return found;
    }

    class ToolbarFixture
    {
      public:
        ToolbarFixture()
            : frame(nullptr, wxID_ANY, "TransformToolbar test")
            , toolbar(new TransformToolbar(frame.get()))
        {
            const auto toggles = childrenOfType(toolbar, wxCLASSINFO(wxToggleButton));
            const auto combos  = childrenOfType(toolbar, wxCLASSINFO(wxComboBox));
            REQUIRE(toggles.size() == 2);
            REQUIRE(combos.size() == 2);
            moveButton   = static_cast<wxToggleButton*>(toggles[0]);
            rotateButton = static_cast<wxToggleButton*>(toggles[1]);
            moveCombo    = static_cast<wxComboBox*>(combos[0]);
            rotateCombo  = static_cast<wxComboBox*>(combos[1]);

            toolbar->SetOnActiveToolChanged([this](ActiveTool tool) { tools.push_back(tool); });
            toolbar->SetOnTargetChanged([this](int index) { targets.push_back(index); });
        }

        // Simulates a user click on a toggle button.
        static void click(wxToggleButton* button)
        {
            button->SetValue(!button->GetValue());
            wxCommandEvent event(wxEVT_TOGGLEBUTTON, button->GetId());
            event.SetEventObject(button);
            event.SetInt(button->GetValue() ? 1 : 0);
            button->HandleWindowEvent(event);
        }

        // Simulates the user picking an entry in a target dropdown.
        static void pick(wxComboBox* combo, int index)
        {
            combo->SetSelection(index);
            wxCommandEvent event(wxEVT_COMBOBOX, combo->GetId());
            event.SetEventObject(combo);
            event.SetInt(index);
            combo->HandleWindowEvent(event);
        }

        ScopedFrame<>           frame;
        TransformToolbar*       toolbar;
        wxToggleButton*         moveButton   = nullptr;
        wxToggleButton*         rotateButton = nullptr;
        wxComboBox*             moveCombo    = nullptr;
        wxComboBox*             rotateCombo  = nullptr;
        std::vector<ActiveTool> tools;
        std::vector<int>        targets;
    };
} // namespace

TEST_CASE_METHOD(ToolbarFixture, "TransformToolbar: starts disabled with only \"All models\"", "[integration][ui][toolbar]")
{
    CHECK_FALSE(moveButton->IsEnabled());
    CHECK_FALSE(rotateButton->IsEnabled());
    CHECK(toolbar->GetActiveTool() == ActiveTool::None);
    CHECK(toolbar->GetTargetIndex() == -1);
    CHECK(moveCombo->GetCount() == 1);
}

TEST_CASE_METHOD(ToolbarFixture, "TransformToolbar: Move and Rotate are mutually exclusive", "[integration][ui][toolbar]")
{
    toolbar->SetEnabled(true);
    tools.clear();

    click(moveButton);
    CHECK(toolbar->GetActiveTool() == ActiveTool::Move);

    click(rotateButton);
    CHECK(toolbar->GetActiveTool() == ActiveTool::Rotate);
    CHECK_FALSE(moveButton->GetValue());

    click(rotateButton);
    CHECK(toolbar->GetActiveTool() == ActiveTool::None);

    CHECK(tools == std::vector<ActiveTool>{ActiveTool::Move, ActiveTool::Rotate, ActiveTool::None});
}

TEST_CASE_METHOD(ToolbarFixture, "TransformToolbar: disabling resets the active tool", "[integration][ui][toolbar]")
{
    toolbar->SetEnabled(true);
    click(moveButton);
    tools.clear();

    toolbar->SetEnabled(false);

    CHECK(toolbar->GetActiveTool() == ActiveTool::None);
    CHECK_FALSE(moveButton->GetValue());
    CHECK(tools == std::vector<ActiveTool>{ActiveTool::None});
}

TEST_CASE_METHOD(ToolbarFixture, "TransformToolbar: targets stay in sync across both dropdowns", "[integration][ui][toolbar]")
{
    wxArrayString names;
    names.Add("a.stl");
    names.Add("b.stl");
    toolbar->SetTargets(names);

    REQUIRE(moveCombo->GetCount() == 3);
    CHECK(moveCombo->GetString(0) == "All models");
    CHECK(toolbar->GetTargetIndex() == -1);

    pick(rotateCombo, 2);

    CHECK(toolbar->GetTargetIndex() == 1);
    CHECK(moveCombo->GetSelection() == 2);
    CHECK(targets == std::vector<int>{1});
}

TEST_CASE_METHOD(ToolbarFixture, "TransformToolbar: SetTargets keeps the selected model if still present", "[integration][ui][toolbar]")
{
    wxArrayString names;
    names.Add("a.stl");
    names.Add("b.stl");
    toolbar->SetTargets(names);
    pick(moveCombo, 2); // b.stl

    names.Insert("new.stl", 0);
    toolbar->SetTargets(names);
    CHECK(moveCombo->GetStringSelection() == "b.stl");
    CHECK(toolbar->GetTargetIndex() == 2);

    names.Remove("b.stl");
    toolbar->SetTargets(names);
    CHECK(toolbar->GetTargetIndex() == -1);
}
