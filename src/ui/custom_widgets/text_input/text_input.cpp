#include "text_input.hpp"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr double kCornerRadius = 4.0;
        constexpr int    kSuffixWidth  = 26;
    } // namespace

    TextInput::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    TextInput::Builder& TextInput::Builder::setValue(const wxString& value)
    {
        value_ = value;
        return *this;
    }

    TextInput::Builder& TextInput::Builder::setSuffix(const wxString& suffix)
    {
        suffix_ = suffix;
        return *this;
    }

    TextInput::Builder& TextInput::Builder::setWidth(int width)
    {
        width_ = width;
        return *this;
    }

    TextInput::Builder& TextInput::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    TextInput::Builder& TextInput::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    TextInput::Builder&
    TextInput::Builder::onValueChanged(std::function<void(const wxString&)> callback)
    {
        on_value_changed_ = std::move(callback);
        return *this;
    }

    TextInput* TextInput::Builder::build() const
    {
        return new TextInput(parent_, value_, suffix_, width_, palette_, on_value_changed_);
    }

    TextInput::TextInput(wxWindow*                              parent,
                          wxString                               value,
                          wxString                               suffix,
                          int                                    width,
                          Palette                                palette,
                          std::function<void(const wxString&)> on_value_changed)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          suffix_(std::move(suffix)),
          palette_(std::move(palette)),
          on_value_changed_(std::move(on_value_changed))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &TextInput::onPaint, this);

        text_ctrl_ = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxDefaultSize,
                                    wxBORDER_NONE | wxTE_PROCESS_ENTER);
        text_ctrl_->SetBackgroundColour(palette_.background);
        text_ctrl_->SetForegroundColour(palette_.text);

        text_ctrl_->Bind(wxEVT_TEXT, [this](wxCommandEvent& event) {
            event.Skip();
            if (on_value_changed_)
            {
                on_value_changed_(text_ctrl_->GetValue());
            }
        });

        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(text_ctrl_, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, kPaddingX);
        if (!suffix_.empty())
        {
            sizer->AddSpacer(kSuffixWidth);
        }
        SetSizer(sizer);

        SetMinSize(wxSize(width, kHeight));
    }

    void TextInput::setPalette(const Palette& palette)
    {
        palette_ = palette;
        text_ctrl_->SetBackgroundColour(palette_.background);
        text_ctrl_->SetForegroundColour(palette_.text);
        Refresh();
    }

    wxString TextInput::value() const
    {
        return text_ctrl_->GetValue();
    }

    void TextInput::setValue(const wxString& value)
    {
        text_ctrl_->ChangeValue(value);
    }

    void TextInput::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
        {
            return;
        }

        const wxSize size = GetClientSize();

        gc->SetBrush(wxBrush(palette_.background));
        gc->SetPen(wxPen(palette_.border, 1));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, kCornerRadius);

        if (!suffix_.empty())
        {
            gc->SetFont(GetFont(), palette_.textMuted);
            double textWidth  = 0;
            double textHeight = 0;
            gc->GetTextExtent(suffix_, &textWidth, &textHeight);
            gc->DrawText(suffix_, size.x - textWidth - kPaddingX, (size.y - textHeight) / 2.0);
        }
    }

} // namespace ui::custom_widgets
