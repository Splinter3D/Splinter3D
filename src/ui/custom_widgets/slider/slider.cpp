#include "slider.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace ui::custom_widgets
{
    Slider::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    Slider::Builder& Slider::Builder::setRange(int min_value, int max_value)
    {
        min_value_ = min_value;
        max_value_ = max_value;
        return *this;
    }

    Slider::Builder& Slider::Builder::setValue(int value)
    {
        value_ = value;
        return *this;
    }

    Slider::Builder& Slider::Builder::setEnabled(bool enabled)
    {
        enabled_ = enabled;
        return *this;
    }

    Slider::Builder& Slider::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    Slider::Builder& Slider::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    Slider::Builder& Slider::Builder::onValueChanged(std::function<void(int)> callback)
    {
        on_value_changed_ = std::move(callback);
        return *this;
    }

    Slider* Slider::Builder::build() const
    {
        Slider* slider =
            new Slider(parent_, min_value_, max_value_, value_, palette_, on_value_changed_);
        slider->Enable(enabled_);
        return slider;
    }

    Slider::Slider(wxWindow*                 parent,
                   int                       min_value,
                   int                       max_value,
                   int                       value,
                   Palette                   palette,
                   std::function<void(int)> on_value_changed)
        : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          min_value_(min_value),
          max_value_(std::max(max_value, min_value + 1)),
          value_(std::clamp(value, min_value, std::max(max_value, min_value + 1))),
          palette_(std::move(palette)),
          on_value_changed_(std::move(on_value_changed))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        Bind(wxEVT_PAINT, &Slider::onPaint, this);
        Bind(wxEVT_LEFT_DOWN, &Slider::onLeftDown, this);
        Bind(wxEVT_LEFT_UP, &Slider::onLeftUp, this);
        Bind(wxEVT_MOTION, &Slider::onMotion, this);
        Bind(wxEVT_LEAVE_WINDOW, &Slider::onMouseLeave, this);

        SetInitialSize(DoGetBestSize());
    }

    void Slider::setPalette(const Palette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    void Slider::setValue(int value)
    {
        const int clamped = std::clamp(value, min_value_, max_value_);
        if (clamped == value_)
        {
            return;
        }
        value_ = clamped;
        Refresh();
    }

    bool Slider::Enable(bool enable)
    {
        const bool changed = wxControl::Enable(enable);
        if (changed)
        {
            hovered_  = false;
            dragging_ = false;
            Refresh();
        }
        return changed;
    }

    int Slider::valueAt(int x) const
    {
        const int usableWidth = GetClientSize().x - 2 * kKnobRadius;
        if (usableWidth <= 0)
        {
            return min_value_;
        }
        const double ratio =
            std::clamp(static_cast<double>(x - kKnobRadius) / usableWidth, 0.0, 1.0);
        return min_value_ + static_cast<int>(std::lround(ratio * (max_value_ - min_value_)));
    }

    void Slider::applyValueFromPointer(int x)
    {
        const int newValue = valueAt(x);
        if (newValue == value_)
        {
            return;
        }
        value_ = newValue;
        Refresh();
        if (on_value_changed_)
        {
            on_value_changed_(value_);
        }
    }

    double Slider::knobCentreX() const
    {
        const int usableWidth = GetClientSize().x - 2 * kKnobRadius;
        const double ratio    = static_cast<double>(value_ - min_value_) /
                             static_cast<double>(max_value_ - min_value_);
        return kKnobRadius + ratio * usableWidth;
    }

    void Slider::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
        {
            return;
        }

        const wxSize size    = GetClientSize();
        const double centreY = size.y / 2.0;
        const double knobX   = knobCentreX();

        const bool enabled = IsEnabled();

        gc->SetBrush(wxBrush(enabled ? palette_.border : palette_.backgroundDisabled));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(kKnobRadius, centreY - kTrackHeight / 2.0,
                                  size.x - 2.0 * kKnobRadius, kTrackHeight,
                                  kTrackHeight / 2.0);

        gc->SetBrush(wxBrush(enabled ? palette_.accent : palette_.textDisabled));
        gc->DrawRoundedRectangle(kKnobRadius, centreY - kTrackHeight / 2.0,
                                  std::max(0.0, knobX - kKnobRadius), kTrackHeight,
                                  kTrackHeight / 2.0);

        wxColour knobColour = palette_.textDisabled;
        if (enabled)
        {
            knobColour = dragging_ ? palette_.accentPressed
                       : hovered_  ? palette_.accentHover
                                   : palette_.accent;
        }
        gc->SetBrush(wxBrush(knobColour));
        gc->SetPen(wxPen(enabled ? palette_.accentPressed : palette_.border, 1));
        gc->DrawEllipse(knobX - kKnobRadius, centreY - kKnobRadius, 2.0 * kKnobRadius,
                        2.0 * kKnobRadius);
    }

    void Slider::onLeftDown(wxMouseEvent& event)
    {
        event.Skip();
        if (!IsEnabled())
        {
            return;
        }
        dragging_ = true;
        CaptureMouse();
        applyValueFromPointer(event.GetPosition().x);
    }

    void Slider::onLeftUp(wxMouseEvent& event)
    {
        event.Skip();
        if (HasCapture())
        {
            ReleaseMouse();
        }
        dragging_ = false;
        Refresh();
    }

    void Slider::onMotion(wxMouseEvent& event)
    {
        event.Skip();
        if (!IsEnabled())
        {
            return;
        }

        if (dragging_)
        {
            applyValueFromPointer(event.GetPosition().x);
            return;
        }

        const bool nowHovered = wxRect(wxPoint(0, 0), GetClientSize()).Contains(event.GetPosition());
        if (nowHovered != hovered_)
        {
            hovered_ = nowHovered;
            Refresh();
        }
    }

    void Slider::onMouseLeave(wxMouseEvent& event)
    {
        event.Skip();
        if (hovered_)
        {
            hovered_ = false;
            Refresh();
        }
    }

    wxSize Slider::DoGetBestSize() const
    {
        return wxSize(160, kHeight);
    }

} // namespace ui::custom_widgets
