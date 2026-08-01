#include "check_box.hpp"

namespace ui::widgets
{
    CheckBox::CheckBox(wxWindow* parent) : parent_(parent)
    {
    }

    CheckBox& CheckBox::setLabel(const wxString& label)
    {
        label_ = label;
        return *this;
    }

    CheckBox& CheckBox::setChecked(bool checked)
    {
        checked_ = checked;
        return *this;
    }

    CheckBox& CheckBox::setEnabled(bool state)
    {
        enabled_ = state;
        return *this;
    }

    CheckBox& CheckBox::onToggle(std::function<void(wxCheckBox&, bool)> callback)
    {
        on_toggle_ = std::move(callback);
        return *this;
    }

    wxCheckBox* CheckBox::build() const
    {
        auto* check_box = new wxCheckBox(parent_, wxID_ANY, label_);

        check_box->SetValue(checked_);
        if (!enabled_)
            check_box->Disable();
        if (on_toggle_)
            check_box->Bind(wxEVT_CHECKBOX,
                            [callback = on_toggle_, check_box](wxCommandEvent& e) {
                                callback(*check_box, e.IsChecked());
                            });

        return check_box;
    }

} // namespace ui::widgets
