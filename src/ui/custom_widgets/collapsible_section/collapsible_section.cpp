#include "collapsible_section.hpp"

#include <functional>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr int kPaddingX = 8;
    } // namespace

    class CollapsibleSection::HeaderPanel : public wxPanel
    {
      public:
        HeaderPanel(wxWindow* parent, wxString title, bool expanded, Palette palette)
            : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
              title_(std::move(title)),
              expanded_(expanded),
              palette_(std::move(palette))
        {
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            SetCursor(wxCursor(wxCURSOR_HAND));
            Bind(wxEVT_PAINT, &HeaderPanel::onPaint, this);
            Bind(wxEVT_LEFT_DOWN, &HeaderPanel::onLeftDown, this);
            SetMinSize(wxSize(-1, kHeaderHeight));
        }

        void setPalette(const Palette& palette)
        {
            palette_ = palette;
            Refresh();
        }

        void setExpanded(bool expanded)
        {
            expanded_ = expanded;
            Refresh();
        }

        void setOnToggle(std::function<void()> on_toggle) { on_toggle_ = std::move(on_toggle); }

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
            gc->DrawText(title_, kPaddingX + 14, (size.y - textHeight) / 2.0);

            const double cx = kPaddingX + 5;
            const double cy = size.y / 2.0;
            wxGraphicsPath path = gc->CreatePath();
            if (expanded_)
            {
                path.MoveToPoint(cx - 4, cy - 2);
                path.AddLineToPoint(cx, cy + 2);
                path.AddLineToPoint(cx + 4, cy - 2);
            }
            else
            {
                path.MoveToPoint(cx - 2, cy - 4);
                path.AddLineToPoint(cx + 2, cy);
                path.AddLineToPoint(cx - 2, cy + 4);
            }
            gc->SetPen(wxPen(palette_.textMuted, 2));
            gc->StrokePath(path);
        }

        void onLeftDown(wxMouseEvent& event)
        {
            event.Skip();
            if (on_toggle_)
            {
                on_toggle_();
            }
        }

        wxString               title_;
        bool                    expanded_ = true;
        Palette                 palette_;
        std::function<void()>  on_toggle_;
    };

    CollapsibleSection::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    CollapsibleSection::Builder& CollapsibleSection::Builder::setTitle(const wxString& title)
    {
        title_ = title;
        return *this;
    }

    CollapsibleSection::Builder& CollapsibleSection::Builder::setExpanded(bool expanded)
    {
        expanded_ = expanded;
        return *this;
    }

    CollapsibleSection::Builder& CollapsibleSection::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    CollapsibleSection::Builder& CollapsibleSection::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    CollapsibleSection* CollapsibleSection::Builder::build() const
    {
        return new CollapsibleSection(parent_, title_, expanded_, palette_);
    }

    CollapsibleSection::CollapsibleSection(wxWindow* parent, wxString title, bool expanded, Palette palette)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          title_(std::move(title)),
          expanded_(expanded),
          palette_(std::move(palette))
    {
        SetBackgroundColour(palette_.surface);

        wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

        header_panel_ = new HeaderPanel(this, title_, expanded_, palette_);
        header_panel_->setOnToggle([this] { setExpanded(!expanded_); });
        rootSizer->Add(header_panel_, 0, wxEXPAND);

        content_panel_ = new wxPanel(this, wxID_ANY);
        content_panel_->SetBackgroundColour(palette_.surface);
        content_sizer_ = new wxBoxSizer(wxVERTICAL);
        content_panel_->SetSizer(content_sizer_);
        content_panel_->Show(expanded_);
        rootSizer->Add(content_panel_, 0, wxEXPAND);

        SetSizer(rootSizer);
    }

    void CollapsibleSection::setPalette(const Palette& palette)
    {
        palette_ = palette;
        SetBackgroundColour(palette_.surface);
        content_panel_->SetBackgroundColour(palette_.surface);
        header_panel_->setPalette(palette_);
        Refresh();
    }

    void CollapsibleSection::setExpanded(bool expanded)
    {
        if (expanded == expanded_)
        {
            return;
        }
        expanded_ = expanded;
        content_panel_->Show(expanded_);
        header_panel_->setExpanded(expanded_);

        for (wxWindow* ancestor = this; ancestor != nullptr; ancestor = ancestor->GetParent())
        {
            ancestor->Layout();
            if (wxSizer* sizer = ancestor->GetSizer())
            {
                sizer->Layout();
            }
        }

        if (on_toggled_)
        {
            on_toggled_(expanded_);
        }
    }

    void CollapsibleSection::setOnToggled(std::function<void(bool)> callback)
    {
        on_toggled_ = std::move(callback);
    }

} // namespace ui::custom_widgets
