#include "toggle_button.hpp"

namespace ui::widgets
{
    ToggleButton::ToggleButton(wxWindow* parent) : parent_(parent)
    {
    }

    ToggleButton& ToggleButton::setLabel(const wxString& label)
    {
        label_ = label;
        return *this;
    }

    ToggleButton& ToggleButton::onToggle(std::function<void(wxToggleButton&, bool)> callback)
    {
        on_toggle_ = std::move(callback);
        return *this;
    }

    wxToggleButton* ToggleButton::build() const
    {
        auto* toggle_button = new wxToggleButton(parent_, wxID_ANY, label_);

        if (on_toggle_)
            toggle_button->Bind(wxEVT_TOGGLEBUTTON,
                                [callback = on_toggle_, toggle_button](wxCommandEvent& e) {
                                    callback(*toggle_button, e.IsChecked());
                                });

        return toggle_button;
    }

} // namespace ui::widgets
