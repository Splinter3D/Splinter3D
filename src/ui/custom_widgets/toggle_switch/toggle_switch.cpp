#include "toggle_switch.hpp"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr int kWidth  = 40;
        constexpr int kHeight = 22;
        constexpr int kInset  = 2;
    } // namespace

    ToggleSwitch::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    ToggleSwitch::Builder& ToggleSwitch::Builder::setChecked(bool checked)
    {
        checked_ = checked;
        return *this;
    }

    ToggleSwitch::Builder& ToggleSwitch::Builder::setEnabled(bool enabled)
    {
        enabled_ = enabled;
        return *this;
    }

    ToggleSwitch::Builder& ToggleSwitch::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    ToggleSwitch::Builder& ToggleSwitch::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    ToggleSwitch::Builder& ToggleSwitch::Builder::onToggled(std::function<void(bool)> callback)
    {
        on_toggled_ = std::move(callback);
        return *this;
    }

    ToggleSwitch* ToggleSwitch::Builder::build() const
    {
        ToggleSwitch* toggle = new ToggleSwitch(parent_, checked_, palette_, on_toggled_);
        toggle->Enable(enabled_);
        return toggle;
    }

    ToggleSwitch::ToggleSwitch(wxWindow*                  parent,
                                bool                       checked,
                                Palette                    palette,
                                std::function<void(bool)> on_toggled)
        : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          checked_(checked),
          palette_(std::move(palette)),
          on_toggled_(std::move(on_toggled))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &ToggleSwitch::onPaint, this);
        Bind(wxEVT_LEFT_UP, &ToggleSwitch::onLeftUp, this);
        Bind(wxEVT_ENTER_WINDOW, &ToggleSwitch::onMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &ToggleSwitch::onMouseLeave, this);

        SetInitialSize(DoGetBestSize());
    }

    void ToggleSwitch::setPalette(const Palette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    void ToggleSwitch::setChecked(bool checked)
    {
        if (checked == checked_)
        {
            return;
        }
        checked_ = checked;
        Refresh();
    }

    bool ToggleSwitch::Enable(bool enable)
    {
        const bool changed = wxControl::Enable(enable);
        if (changed)
        {
            Refresh();
        }
        return changed;
    }

    void ToggleSwitch::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
        {
            return;
        }

        wxColour trackColour = checked_ ? palette_.accent : palette_.border;
        if (!IsEnabled())
        {
            trackColour = palette_.backgroundDisabled;
        }
        else if (hovered_)
        {
            trackColour = checked_ ? palette_.accentHover : palette_.borderStrong;
        }

        gc->SetBrush(wxBrush(trackColour));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRoundedRectangle(0, 0, kWidth, kHeight, kHeight / 2.0);

        const double knobDiameter = kHeight - 2.0 * kInset;
        const double knobX        = checked_ ? kWidth - kInset - knobDiameter : kInset;

        gc->SetBrush(wxBrush(IsEnabled() ? palette_.text : palette_.textDisabled));
        gc->DrawEllipse(knobX, kInset, knobDiameter, knobDiameter);
    }

    void ToggleSwitch::onLeftUp(wxMouseEvent& event)
    {
        event.Skip();
        if (!IsEnabled())
        {
            return;
        }
        checked_ = !checked_;
        Refresh();
        if (on_toggled_)
        {
            on_toggled_(checked_);
        }
    }

    void ToggleSwitch::onMouseEnter(wxMouseEvent& event)
    {
        event.Skip();
        hovered_ = true;
        Refresh();
    }

    void ToggleSwitch::onMouseLeave(wxMouseEvent& event)
    {
        event.Skip();
        hovered_ = false;
        Refresh();
    }

    wxSize ToggleSwitch::DoGetBestSize() const
    {
        return wxSize(kWidth, kHeight);
    }

} // namespace ui::custom_widgets
