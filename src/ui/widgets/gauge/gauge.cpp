#include "gauge.hpp"

namespace ui::widgets
{

    Gauge::Gauge(wxWindow *parent) : parent_(parent)
    {
    }

    Gauge &Gauge::setRange(int range)
    {
        range_ = range;
        return *this;
    }

    Gauge &Gauge::setValue(int value)
    {
        value_ = value;
        return *this;
    }

    wxGauge *Gauge::build() const
    {
        auto *gauge = new wxGauge(parent_, wxID_ANY, range_);

        gauge->SetValue(value_);

        return gauge;
    }

} // namespace ui::widgets
