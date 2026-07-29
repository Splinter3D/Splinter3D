#include "radio_box.hpp"

namespace ui::widgets
{
    RadioBox::RadioBox(wxWindow *parent) : parent_(parent)
    {
    }

    RadioBox &RadioBox::setTitle(const wxString &title)
    {
        title_ = title;
        return *this;
    }

    RadioBox &RadioBox::setColumns(int columns)
    {
        columns_ = columns;
        return *this;
    }

    RadioBox &RadioBox::setChoices(const wxArrayString &choices)
    {
        choices_ = choices;
        return *this;
    }

    RadioBox &RadioBox::onChange(std::function<void(wxRadioBox &, const wxString &)> callback)
    {
        on_change_ = std::move(callback);
        return *this;
    }

    wxRadioBox *RadioBox::build() const
    {
        auto *radio_box = new wxRadioBox(
            parent_,
            wxID_ANY,
            title_,
            wxDefaultPosition,
            wxDefaultSize,
            choices_,
            columns_,
            wxRA_SPECIFY_COLS);

        if (on_change_)
            radio_box->Bind(wxEVT_RADIOBOX,
                            [callback = on_change_, radio_box](wxCommandEvent &)
                            {
                                callback(*radio_box, radio_box->GetStringSelection());
                            });

        return radio_box;
    }

} // namespace ui::widgets
