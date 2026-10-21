#include "card.hpp"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr double kCornerRadius = 8.0;
    } // namespace

    class Card::HeaderPanel : public wxPanel
    {
      public:
        HeaderPanel(wxWindow* parent, wxString title, Palette palette)
            : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
              title_(std::move(title)),
              palette_(std::move(palette))
        {
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            Bind(wxEVT_PAINT, &HeaderPanel::onPaint, this);
            SetMinSize(wxSize(-1, kTitleBandHeight));
        }

        void setPalette(const Palette& palette)
        {
            palette_ = palette;
            Refresh();
        }

      private:
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

            gc->SetFont(GetFont().Bold(), palette_.text);
            double textWidth  = 0;
            double textHeight = 0;
            gc->GetTextExtent(title_, &textWidth, &textHeight);
            gc->DrawText(title_, kPadding, (size.y - textHeight) / 2.0);

            gc->SetPen(wxPen(palette_.border, 1));
            gc->StrokeLine(kPadding, size.y - 1, size.x - kPadding, size.y - 1);
        }

        wxString title_;
        Palette  palette_;
    };

    Card::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    Card::Builder& Card::Builder::setTitle(const wxString& title)
    {
        title_ = title;
        return *this;
    }

    Card::Builder& Card::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    Card::Builder& Card::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    Card* Card::Builder::build() const
    {
        return new Card(parent_, title_, palette_);
    }

    Card::Card(wxWindow* parent, wxString title, Palette palette)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          title_(std::move(title)),
          palette_(std::move(palette))
    {
        SetBackgroundColour(palette_.surface);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &Card::onPaint, this);

        wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

        if (!title_.empty())
        {
            header_panel_ = new HeaderPanel(this, title_, palette_);
            rootSizer->Add(header_panel_, 0, wxEXPAND);
        }

        content_panel_ = new wxPanel(this, wxID_ANY);
        content_panel_->SetBackgroundColour(palette_.surface);
        content_sizer_ = new wxBoxSizer(wxVERTICAL);
        content_panel_->SetSizer(content_sizer_);
        rootSizer->Add(content_panel_, 1, wxEXPAND | wxALL, kPadding);

        SetSizer(rootSizer);
    }

    void Card::setPalette(const Palette& palette)
    {
        palette_ = palette;
        SetBackgroundColour(palette_.surface);
        content_panel_->SetBackgroundColour(palette_.surface);
        if (header_panel_ != nullptr)
        {
            header_panel_->setPalette(palette_);
        }
        Refresh();
    }

    void Card::onPaint(wxPaintEvent& event)
    {
        event.Skip();

        wxClientDC dc(this);
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
        {
            return;
        }

        const wxSize size = GetClientSize();
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(wxPen(palette_.border, 1));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, kCornerRadius);
    }

} // namespace ui::custom_widgets
