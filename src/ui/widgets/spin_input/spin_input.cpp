#include "spin_input.hpp"

namespace ui::widgets
{
    SpinInput::SpinInput(wxWindow* parent) : parent_(parent)
    {
    }

    SpinInput& SpinInput::setRange(int min_range, int max_range)
    {
        min_range_ = min_range;
        max_range_ = max_range;
        return *this;
    }

    SpinInput& SpinInput::setValue(int value)
    {
        value_ = value;
        return *this;
    }

    SpinInput& SpinInput::onChange(std::function<void(wxSpinCtrl&, int)> callback)
    {
        on_change_ = std::move(callback);
        return *this;
    }

    wxSpinCtrl* SpinInput::build() const
    {
        auto* spin = new wxSpinCtrl(
            parent_,
            wxID_ANY,
            wxString::Format("%d", value_),
            wxDefaultPosition,
            wxDefaultSize,
            wxSP_ARROW_KEYS,
            min_range_,
            max_range_,
            value_);

        if (on_change_)
            spin->Bind(wxEVT_SPINCTRL,
                       [callback = on_change_, spin](wxSpinEvent&) {
                           callback(*spin, spin->GetValue());
                       });

        return spin;
    }

} // namespace ui::widgets
