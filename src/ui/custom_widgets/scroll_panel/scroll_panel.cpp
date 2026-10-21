#include "scroll_panel.hpp"

#include <algorithm>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>

namespace ui::custom_widgets
{
    ScrollPanel::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    ScrollPanel::Builder& ScrollPanel::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    ScrollPanel::Builder& ScrollPanel::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    ScrollPanel* ScrollPanel::Builder::build() const
    {
        return new ScrollPanel(parent_, palette_);
    }

    ScrollPanel::ScrollPanel(wxWindow* parent, Palette palette)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          palette_(std::move(palette))
    {
        SetBackgroundColour(palette_.background);
        SetBackgroundStyle(wxBG_STYLE_PAINT);

        content_panel_ = new wxPanel(this, wxID_ANY);
        content_panel_->SetBackgroundColour(palette_.background);
        content_sizer_ = new wxBoxSizer(wxVERTICAL);
        content_panel_->SetSizer(content_sizer_);

        Bind(wxEVT_PAINT, &ScrollPanel::onPaint, this);
        Bind(wxEVT_SIZE, &ScrollPanel::onSize, this);
        Bind(wxEVT_LEFT_DOWN, &ScrollPanel::onLeftDown, this);
        Bind(wxEVT_LEFT_UP, &ScrollPanel::onLeftUp, this);
        Bind(wxEVT_MOTION, &ScrollPanel::onMotion, this);

        Bind(wxEVT_MOUSEWHEEL, &ScrollPanel::onMouseWheel, this);
    }

    void ScrollPanel::routeWheelEvents(wxWindow* window)
    {
        if (window == nullptr || !wheel_routed_.insert(window).second)
        {
            return;
        }

        window->Bind(wxEVT_MOUSEWHEEL, &ScrollPanel::onMouseWheel, this);

        for (wxWindow* child : window->GetChildren())
        {
            routeWheelEvents(child);
        }
    }

    void ScrollPanel::setPalette(const Palette& palette)
    {
        palette_ = palette;
        SetBackgroundColour(palette_.background);
        content_panel_->SetBackgroundColour(palette_.background);
        Refresh();
    }

    void ScrollPanel::refreshLayout()
    {
        if (laying_out_)
        {
            return;
        }
        laying_out_ = true;

        last_viewport_size_ = GetClientSize();

        const int width = std::max(0, GetClientSize().x - kScrollbarWidth);

        content_panel_->SetSize(0, -scroll_offset_, width, content_panel_->GetSize().y);
        content_panel_->Layout();

        content_height_ = content_sizer_->GetMinSize().y;
        content_panel_->SetSize(0, -scroll_offset_, width, content_height_);

        laying_out_ = false;

        routeWheelEvents(content_panel_);

        scrollTo(scroll_offset_);
        Refresh();
    }

    int ScrollPanel::maxScroll() const
    {
        return std::max(0, content_height_ - GetClientSize().y);
    }

    void ScrollPanel::scrollTo(int offset)
    {
        const int clamped = std::clamp(offset, 0, maxScroll());
        const bool offsetChanged   = clamped != scroll_offset_;
        const bool positionDrifted = content_panel_->GetPosition().y != -clamped;

        scroll_offset_ = clamped;

        if (offsetChanged || positionDrifted)
        {
            content_panel_->Move(0, -scroll_offset_);
            Refresh();
        }
    }

    std::pair<int, int> ScrollPanel::thumbGeometry() const
    {
        const int viewportHeight = GetClientSize().y;
        if (content_height_ <= viewportHeight || content_height_ <= 0)
        {
            return {0, 0};
        }

        const double visibleRatio = static_cast<double>(viewportHeight) / content_height_;
        const int    thumbHeight =
            std::max(kMinThumbHeight, static_cast<int>(viewportHeight * visibleRatio));

        const int travel = viewportHeight - thumbHeight;
        const int top    = maxScroll() == 0
                               ? 0
                               : static_cast<int>(static_cast<double>(scroll_offset_) /
                                               maxScroll() * travel);
        return {top, thumbHeight};
    }

    void ScrollPanel::onPaint(wxPaintEvent&)
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
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, size.x, size.y);

        const auto [thumbTop, thumbHeight] = thumbGeometry();
        if (thumbHeight <= 0)
        {
            return;
        }

        const double x = size.x - kScrollbarWidth + 2.0;
        const double w = kScrollbarWidth - 4.0;

        gc->SetBrush(wxBrush(dragging_thumb_ ? palette_.textMuted : palette_.border));
        gc->DrawRoundedRectangle(x, thumbTop, w, thumbHeight, w / 2.0);
    }

    void ScrollPanel::onMouseWheel(wxMouseEvent& event)
    {
        if (maxScroll() == 0)
        {
            event.Skip();
            return;
        }
        const int delta = event.GetWheelRotation() > 0 ? -kWheelStep : kWheelStep;
        scrollTo(scroll_offset_ + delta);
    }

    void ScrollPanel::onLeftDown(wxMouseEvent& event)
    {
        event.Skip();

        const auto [thumbTop, thumbHeight] = thumbGeometry();
        if (thumbHeight <= 0)
        {
            return;
        }

        const wxPoint pos = event.GetPosition();
        if (pos.x < GetClientSize().x - kScrollbarWidth)
        {
            return;
        }

        if (pos.y >= thumbTop && pos.y < thumbTop + thumbHeight)
        {
            dragging_thumb_   = true;
            drag_start_y_      = pos.y;
            drag_start_offset_ = scroll_offset_;
            drag_travel_      = GetClientSize().y - thumbHeight;
            CaptureMouse();
        }
        else
        {
            const int travel = GetClientSize().y - thumbHeight;
            if (travel > 0)
            {
                const double ratio =
                    std::clamp((pos.y - thumbHeight / 2.0) / travel, 0.0, 1.0);
                scrollTo(static_cast<int>(ratio * maxScroll()));
            }
        }
    }

    void ScrollPanel::onLeftUp(wxMouseEvent& event)
    {
        event.Skip();
        if (HasCapture())
        {
            ReleaseMouse();
        }
        if (dragging_thumb_)
        {
            dragging_thumb_ = false;
            Refresh();
        }
    }

    void ScrollPanel::onMotion(wxMouseEvent& event)
    {
        event.Skip();
        if (!dragging_thumb_)
        {
            return;
        }

        if (drag_travel_ <= 0)
        {
            return;
        }

        const int pixelDelta = event.GetPosition().y - drag_start_y_;
        const int scrollDelta =
            static_cast<int>(static_cast<double>(pixelDelta) / drag_travel_ * maxScroll());
        scrollTo(drag_start_offset_ + scrollDelta);
    }

    void ScrollPanel::onSize(wxSizeEvent& event)
    {
        event.Skip();

        const wxSize size = GetClientSize();
        if (size == last_viewport_size_)
        {
            return;
        }
        last_viewport_size_ = size;

        refreshLayout();
    }

} // namespace ui::custom_widgets
