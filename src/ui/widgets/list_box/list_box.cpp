#include "list_box.hpp"

namespace ui::widgets
{
    ListBox::ListBox(wxWindow* parent) : parent_(parent)
    {
    }

    ListBox& ListBox::setChoices(const wxArrayString& choices)
    {
        choices_ = choices;
        return *this;
    }

    ListBox& ListBox::multiSelection()
    {
        multi_selection_ = true;
        return *this;
    }

    ListBox& ListBox::setRows(int rows)
    {
        rows_ = rows;
        return *this;
    }

    ListBox& ListBox::onSelect(std::function<void(wxListBox&, int)> callback)
    {
        on_select_ = std::move(callback);
        return *this;
    }

    wxListBox* ListBox::build() const
    {
        long  style   = multi_selection_ ? wxLB_MULTIPLE : wxLB_SINGLE;
        auto* listBox = new wxListBox(
            parent_,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            choices_,
            style);

        if (rows_ > 0)
        {
            listBox->SetMinSize(
                wxSize(-1, rows_ * listBox->GetCharHeight()));
        }
        if (on_select_)
            listBox->Bind(wxEVT_LISTBOX,
                          [callback = on_select_, listBox](wxCommandEvent& e) {
                              callback(*listBox, e.GetSelection());
                          });

        return listBox;
    }

} // namespace ui::widgets