#include "button.hpp"

namespace ui::widgets
{
    Button::Button(wxWindow *parent) : parent_(parent)
    {
    }

    Button &Button::setLabel(const wxString &label)
    {
        label_ = label;
        return *this;
    }

    Button &Button::setTooltip(const wxString &tooltip)
    {
        tooltip_ = tooltip;
        return *this;
    }

    Button &Button::setEnabled(bool state)
    {
        enabled_ = state;
        return *this;
    }

    Button &Button::onClick(std::function<void()> callback)
    {
        on_click_ = std::move(callback);
        return *this;
    }

    wxButton *Button::build() const
    {
        auto *button = new wxButton(parent_, wxID_ANY, label_);

        if (!enabled_)
            button->Disable();
        if (!tooltip_.empty())
            button->SetToolTip(tooltip_);
        if (on_click_)
            button->Bind(wxEVT_BUTTON, [callback = on_click_](wxCommandEvent &)
                         { callback(); });

        return button;
    }

} // namespace ui::widgets
