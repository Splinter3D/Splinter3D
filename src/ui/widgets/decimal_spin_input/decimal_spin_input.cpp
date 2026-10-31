#include "decimal_spin_input.hpp"

namespace ui::widgets
{
    DecimalSpinInput::DecimalSpinInput(wxWindow* parent)
        : parent_(parent)
    {
    }

    DecimalSpinInput& DecimalSpinInput::setRange(double min_range, double max_range)
    {
        min_range_ = min_range;
        max_range_ = max_range;
        return *this;
    }

    DecimalSpinInput& DecimalSpinInput::setValue(double value)
    {
        value_ = value;
        return *this;
    }

    DecimalSpinInput& DecimalSpinInput::setIncrement(double increment)
    {
        increment_ = increment;
        return *this;
    }

    DecimalSpinInput& DecimalSpinInput::setDigits(int digits)
    {
        digits_ = digits;
        return *this;
    }

    DecimalSpinInput& DecimalSpinInput::onChange(
        std::function<void(wxSpinCtrlDouble&, double)> callback)
    {
        on_change_ = std::move(callback);
        return *this;
    }

    wxSpinCtrlDouble* DecimalSpinInput::build() const
    {
        auto* spin = new wxSpinCtrlDouble(
            parent_,
            wxID_ANY,
            wxString::Format("%.*f", digits_, value_),
            wxDefaultPosition,
            wxDefaultSize,
            wxSP_ARROW_KEYS,
            min_range_,
            max_range_,
            value_,
            increment_);
        spin->SetDigits(static_cast<unsigned int>(digits_));

        if (on_change_)
            spin->Bind(wxEVT_SPINCTRLDOUBLE,
                       [callback = on_change_, spin](wxSpinDoubleEvent&) {
                           callback(*spin, spin->GetValue());
                       });

        return spin;
    }

} // namespace ui::widgets
