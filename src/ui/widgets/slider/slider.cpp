#include "slider.hpp"

namespace ui::widgets
{
    Slider::Slider(wxWindow *parent) : parent_(parent)
    {
    }

    Slider &Slider::setRange(int min_range, int max_range)
    {
        min_range_ = min_range;
        max_range_ = max_range;
        return *this;
    }

    Slider &Slider::setValue(int value)
    {
        value_ = value;
        return *this;
    }

    Slider &Slider::vertical()
    {
        vertical_ = true;
        return *this;
    }

    Slider &Slider::withLabels()
    {
        with_labels_ = true;
        return *this;
    }

    Slider &Slider::setLength(int length)
    {
        length_ = length;
        return *this;
    }

    Slider &Slider::onChange(std::function<void(wxSlider &, int)> callback)
    {
        on_change_ = std::move(callback);
        return *this;
    }

    wxSlider *Slider::build() const
    {
        long style = vertical_ ? wxSL_VERTICAL : wxSL_HORIZONTAL;
        if (with_labels_)
        {
            style |= wxSL_LABELS;

            if (vertical_)
                style |= wxSL_INVERSE; // top = high, bottom = low
        }

        wxSize size = wxDefaultSize;
        if (length_ > 0)
        {
            const int length = parent_->FromDIP(length_);

            size = vertical_
                       ? wxSize(-1, length)
                       : wxSize(length, -1);
        }

        auto *slider = new wxSlider(
            parent_,
            wxID_ANY,
            value_,
            min_range_,
            max_range_,
            wxDefaultPosition,
            size,
            style);

        if (on_change_)
            slider->Bind(wxEVT_SLIDER,
                         [callback = on_change_, slider](wxCommandEvent &)
                         {
                             callback(*slider, slider->GetValue());
                         });

        return slider;
    }

} // namespace ui::widgets
