#include "combo_box.hpp"

namespace ui::widgets
{
    ComboBox::ComboBox(wxWindow* parent) : parent_(parent)
    {
    }

    ComboBox& ComboBox::setPlaceholder(const wxString& placeholder)
    {
        placeholder_ = placeholder;
        return *this;
    }

    ComboBox& ComboBox::setChoices(const wxArrayString& choices)
    {
        choices_ = choices;
        return *this;
    }

    ComboBox& ComboBox::onSelect(std::function<void(wxComboBox&, const wxString&)> callback)
    {
        on_select_ = std::move(callback);
        return *this;
    }

    wxComboBox* ComboBox::build() const
    {
        auto* combo_box = new wxComboBox(
            parent_,
            wxID_ANY,
            placeholder_,
            wxDefaultPosition,
            wxDefaultSize,
            choices_,
            wxCB_READONLY);

        if (on_select_)
            combo_box->Bind(wxEVT_COMBOBOX,
                            [callback = on_select_, combo_box](wxCommandEvent&) {
                                callback(*combo_box, combo_box->GetStringSelection());
                            });

        return combo_box;
    }

} // namespace ui::widgets
