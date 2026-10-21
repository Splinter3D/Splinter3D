#include "custom_button.hpp"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr double kCornerRadius = 6.0;
    } // namespace

    CustomButton::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    CustomButton::Builder& CustomButton::Builder::setLabel(const wxString& label)
    {
        label_ = label;
        return *this;
    }

    CustomButton::Builder& CustomButton::Builder::setTooltip(const wxString& tooltip)
    {
        tooltip_ = tooltip;
        return *this;
    }

    CustomButton::Builder& CustomButton::Builder::setEnabled(bool enabled)
    {
        enabled_ = enabled;
        return *this;
    }

    CustomButton::Builder& CustomButton::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    CustomButton::Builder& CustomButton::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    CustomButton::Builder& CustomButton::Builder::onClick(std::function<void()> callback)
    {
        on_click_ = std::move(callback);
        return *this;
    }

    CustomButton* CustomButton::Builder::build() const
    {
        CustomButton* button = new CustomButton(parent_, label_, palette_, on_click_);
        button->Enable(enabled_);
        if (!tooltip_.empty())
        {
            button->SetToolTip(tooltip_);
        }
        return button;
    }

    CustomButton::CustomButton(wxWindow*              parent,
                                wxString               label,
                                Palette                palette,
                                std::function<void()>  on_click)
        : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          label_(std::move(label)),
          palette_(std::move(palette)),
          on_click_(std::move(on_click))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &CustomButton::onPaint, this);
        Bind(wxEVT_ENTER_WINDOW, &CustomButton::onMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &CustomButton::onMouseLeave, this);
        Bind(wxEVT_LEFT_DOWN, &CustomButton::onLeftDown, this);
        Bind(wxEVT_LEFT_UP, &CustomButton::onLeftUp, this);

        SetInitialSize(DoGetBestSize());
    }

    void CustomButton::setPalette(const Palette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    bool CustomButton::Enable(bool enable)
    {
        const bool changed = wxControl::Enable(enable);
        if (changed)
        {
            hovered_ = false;
            pressed_ = false;
            Refresh();
        }
        return changed;
    }

    void CustomButton::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
        {
            return;
        }

        const wxSize size = GetClientSize();

        wxColour bg = palette_.background;
        if (!IsEnabled())
        {
            bg = palette_.backgroundDisabled;
        }
        else if (pressed_)
        {
            bg = palette_.backgroundPressed;
        }
        else if (hovered_)
        {
            bg = palette_.backgroundHover;
        }

        gc->SetBrush(wxBrush(bg));
        gc->SetPen(wxPen(palette_.border, 1));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, kCornerRadius);

        gc->SetFont(GetFont(), IsEnabled() ? palette_.text : palette_.textDisabled);
        double textWidth = 0;
        double textHeight = 0;
        gc->GetTextExtent(label_, &textWidth, &textHeight);
        gc->DrawText(label_, (size.x - textWidth) / 2.0, (size.y - textHeight) / 2.0);
    }

    void CustomButton::onMouseEnter(wxMouseEvent& event)
    {
        event.Skip();
        if (!IsEnabled())
        {
            return;
        }
        hovered_ = true;
        Refresh();
    }

    void CustomButton::onMouseLeave(wxMouseEvent& event)
    {
        event.Skip();
        hovered_ = false;
        pressed_ = false;
        Refresh();
    }

    void CustomButton::onLeftDown(wxMouseEvent& event)
    {
        event.Skip();
        if (!IsEnabled())
        {
            return;
        }
        pressed_ = true;
        CaptureMouse();
        Refresh();
    }

    void CustomButton::onLeftUp(wxMouseEvent& event)
    {
        event.Skip();
        if (!IsEnabled())
        {
            return;
        }
        if (HasCapture())
        {
            ReleaseMouse();
        }
        const bool wasPressed = pressed_;
        pressed_              = false;
        Refresh();

        const wxRect clientRect(wxPoint(0, 0), GetClientSize());
        if (wasPressed && clientRect.Contains(event.GetPosition()) && on_click_)
        {
            on_click_();
        }
    }

    wxSize CustomButton::DoGetBestSize() const
    {
        wxClientDC dc(const_cast<CustomButton*>(this));
        dc.SetFont(GetFont());
        wxCoord textWidth  = 0;
        wxCoord textHeight = 0;
        dc.GetTextExtent(label_, &textWidth, &textHeight);
        return wxSize(textWidth + 32, textHeight + 16);
    }

} // namespace ui::custom_widgets
