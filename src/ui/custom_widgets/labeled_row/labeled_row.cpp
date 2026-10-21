#include "labeled_row.hpp"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr int kPaddingX = 4;
    } // namespace

    class LabeledRow::LabelPanel : public wxPanel
    {
      public:
        LabelPanel(wxWindow* parent, wxString label, Palette palette)
            : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
              label_(std::move(label)),
              palette_(std::move(palette))
        {
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            Bind(wxEVT_PAINT, &LabelPanel::onPaint, this);
            SetMinSize(DoGetBestSize());
        }

        void setPalette(const Palette& palette)
        {
            palette_ = palette;
            Refresh();
        }

      private:
        wxSize DoGetBestSize() const override
        {
            wxClientDC dc(const_cast<LabelPanel*>(this));
            dc.SetFont(GetFont());
            wxCoord textWidth  = 0;
            wxCoord textHeight = 0;
            dc.GetTextExtent(label_, &textWidth, &textHeight);
            return wxSize(textWidth + 2 * kPaddingX, textHeight);
        }

        void onPaint(wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(this);
            dc.Clear();

            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (!gc)
            {
                return;
            }

            const wxSize size = GetClientSize();

            gc->SetBrush(wxBrush(palette_.surface));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(0, 0, size.x, size.y);

            gc->SetFont(GetFont(), palette_.textMuted);
            double textWidth  = 0;
            double textHeight = 0;
            gc->GetTextExtent(label_, &textWidth, &textHeight);
            gc->DrawText(label_, kPaddingX, (size.y - textHeight) / 2.0);
        }

        wxString label_;
        Palette  palette_;
    };

    LabeledRow::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    LabeledRow::Builder& LabeledRow::Builder::setLabel(const wxString& label)
    {
        label_ = label;
        return *this;
    }

    LabeledRow::Builder& LabeledRow::Builder::setValue(const wxString& value)
    {
        value_ = value;
        return *this;
    }

    LabeledRow::Builder& LabeledRow::Builder::setSuffix(const wxString& suffix)
    {
        suffix_ = suffix;
        return *this;
    }

    LabeledRow::Builder& LabeledRow::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    LabeledRow::Builder& LabeledRow::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    LabeledRow::Builder&
    LabeledRow::Builder::onValueChanged(std::function<void(const wxString&)> callback)
    {
        on_value_changed_ = std::move(callback);
        return *this;
    }

    LabeledRow* LabeledRow::Builder::build() const
    {
        return new LabeledRow(parent_, label_, value_, suffix_, palette_, on_value_changed_);
    }

    LabeledRow::LabeledRow(wxWindow*                              parent,
                            wxString                               label,
                            wxString                               value,
                            wxString                               suffix,
                            Palette                                palette,
                            std::function<void(const wxString&)> on_value_changed)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          palette_(std::move(palette))
    {
        SetBackgroundColour(palette_.surface);

        label_panel_ = new LabelPanel(this, std::move(label), palette_);

        input_ = TextInput::Builder(this)
                      .setValue(value)
                      .setSuffix(suffix)
                      .setPalette(palette_)
                      .onValueChanged(std::move(on_value_changed))
                      .build();

        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(label_panel_, 1, wxEXPAND | wxRIGHT, 8);
        sizer->Add(input_, 0, wxALIGN_CENTER_VERTICAL);
        SetSizer(sizer);

        SetMinSize(wxSize(-1, kRowHeight));
    }

    void LabeledRow::setPalette(const Palette& palette)
    {
        palette_ = palette;
        SetBackgroundColour(palette_.surface);
        label_panel_->setPalette(palette_);
        input_->setPalette(palette_);
        Refresh();
    }

    wxString LabeledRow::value() const
    {
        return input_->value();
    }

    void LabeledRow::setValue(const wxString& value)
    {
        input_->setValue(value);
    }

} // namespace ui::custom_widgets
