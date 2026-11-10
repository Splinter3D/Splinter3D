#include "widgets_showcase_panel.hpp"

#include "ui/widgets/button/button.hpp"
#include "ui/widgets/check_box/check_box.hpp"
#include "ui/widgets/combo_box/combo_box.hpp"
#include "ui/widgets/gauge/gauge.hpp"
#include "ui/widgets/list_box/list_box.hpp"
#include "ui/widgets/radio_box/radio_box.hpp"
#include "ui/widgets/slider/slider.hpp"
#include "ui/widgets/spin_input/spin_input.hpp"
#include "ui/widgets/text_input/text_input.hpp"
#include "ui/widgets/toggle_button/toggle_button.hpp"

namespace ui::panels
{
    WidgetsShowcasePanel::WidgetsShowcasePanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        // Each ui::widgets::X is a fluent builder, not a wxWindow itself.
        // .build() is what actually constructs and returns the real
        // wxWidgets control - that's the pointer the sizer needs.

        auto* button = ui::widgets::Button(this)
                           .setLabel("Button")
                           .setTooltip("A plain button")
                           .build();
        sizer->Add(button, 0, wxEXPAND | wxALL, 4);

        auto* checkBox = ui::widgets::CheckBox(this)
                             .setLabel("CheckBox")
                             .build();
        sizer->Add(checkBox, 0, wxEXPAND | wxALL, 4);

        wxArrayString comboChoices;
        comboChoices.Add("Option A");
        comboChoices.Add("Option B");
        comboChoices.Add("Option C");

        auto* comboBox = ui::widgets::ComboBox(this)
                             .setPlaceholder("Pick an option")
                             .setChoices(comboChoices)
                             .build();
        sizer->Add(comboBox, 0, wxEXPAND | wxALL, 4);

        auto* gauge = ui::widgets::Gauge(this)
                          .setRange(100)
                          .setValue(50)
                          .build();
        sizer->Add(gauge, 0, wxEXPAND | wxALL, 4);

        wxArrayString listChoices;
        listChoices.Add("Item 1");
        listChoices.Add("Item 2");
        listChoices.Add("Item 3");

        auto* listBox = ui::widgets::ListBox(this)
                            .setChoices(listChoices)
                            .setRows(4)
                            .build();
        sizer->Add(listBox, 0, wxEXPAND | wxALL, 4);

        wxArrayString radioChoices;
        radioChoices.Add("A");
        radioChoices.Add("B");
        radioChoices.Add("C");

        auto* radioBox = ui::widgets::RadioBox(this)
                             .setTitle("RadioBox")
                             .setChoices(radioChoices)
                             .build();
        sizer->Add(radioBox, 0, wxEXPAND | wxALL, 4);

        auto* slider = ui::widgets::Slider(this)
                           .setRange(0, 100)
                           .setValue(30)
                           .build();
        sizer->Add(slider, 0, wxEXPAND | wxALL, 4);

        auto* spinInput = ui::widgets::SpinInput(this)
                              .setRange(0, 100)
                              .setValue(10)
                              .build();
        sizer->Add(spinInput, 0, wxEXPAND | wxALL, 4);

        auto* textInput = ui::widgets::TextInput(this)
                              .setHint("Type here")
                              .build();
        sizer->Add(textInput, 0, wxEXPAND | wxALL, 4);

        auto* toggleButton = ui::widgets::ToggleButton(this)
                                 .setLabel("ToggleButton")
                                 .build();
        sizer->Add(toggleButton, 0, wxEXPAND | wxALL, 4);

        SetSizer(sizer);
    }
} // namespace ui::panels
