#include "segmented_control.hpp"

#include <algorithm>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr double kCornerRadius = 6.0;
        constexpr int    kPaddingX     = 14;
        constexpr int    kInset        = 2;
    } // namespace

    SegmentedControl::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    SegmentedControl::Builder& SegmentedControl::Builder::setSegments(std::vector<wxString> segments)
    {
        segments_ = std::move(segments);
        return *this;
    }

    SegmentedControl::Builder& SegmentedControl::Builder::setSelectedIndex(int index)
    {
        selected_index_ = index;
        return *this;
    }

    SegmentedControl::Builder& SegmentedControl::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    SegmentedControl::Builder& SegmentedControl::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    SegmentedControl::Builder&
    SegmentedControl::Builder::onSelectionChanged(std::function<void(int)> callback)
    {
        on_selection_changed_ = std::move(callback);
        return *this;
    }

    SegmentedControl* SegmentedControl::Builder::build() const
    {
        return new SegmentedControl(parent_, segments_, selected_index_, palette_, on_selection_changed_);
    }

    SegmentedControl::SegmentedControl(wxWindow*                 parent,
                                        std::vector<wxString>     segments,
                                        int                       selected_index,
                                        Palette                   palette,
                                        std::function<void(int)> on_selection_changed)
        : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          segments_(std::move(segments)),
          selected_index_(selected_index),
          palette_(std::move(palette)),
          on_selection_changed_(std::move(on_selection_changed))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &SegmentedControl::onPaint, this);
        Bind(wxEVT_LEFT_DOWN, &SegmentedControl::onLeftDown, this);
        Bind(wxEVT_MOTION, &SegmentedControl::onMotion, this);
        Bind(wxEVT_LEAVE_WINDOW, &SegmentedControl::onMouseLeave, this);

        SetInitialSize(DoGetBestSize());
    }

    void SegmentedControl::setPalette(const Palette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    int SegmentedControl::segmentAt(const wxPoint& pos) const
    {
        const wxSize size = GetClientSize();
        if (segments_.empty())
        {
            return -1;
        }
        const int segW = size.x / static_cast<int>(segments_.size());
        if (segW <= 0)
        {
            return -1;
        }
        const int index = pos.x / segW;
        return (index >= 0 && index < static_cast<int>(segments_.size())) ? index : -1;
    }

    void SegmentedControl::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc || segments_.empty())
        {
            return;
        }

        const wxSize size = GetClientSize();
        const double segW = static_cast<double>(size.x) / static_cast<double>(segments_.size());

        gc->SetBrush(wxBrush(palette_.background));
        gc->SetPen(wxPen(palette_.border, 1));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, kCornerRadius);

        for (std::size_t i = 0; i < segments_.size(); ++i)
        {
            const double x = static_cast<double>(i) * segW;

            if (static_cast<int>(i) == selected_index_)
            {
                gc->SetBrush(wxBrush(palette_.accent));
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRoundedRectangle(x + kInset, kInset, segW - 2 * kInset, size.y - 2 * kInset,
                                          kCornerRadius - 1.0);
            }
            else if (static_cast<int>(i) == hovered_index_)
            {
                gc->SetBrush(wxBrush(palette_.backgroundHover));
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRoundedRectangle(x + kInset, kInset, segW - 2 * kInset, size.y - 2 * kInset,
                                          kCornerRadius - 1.0);
            }

            const bool selected = static_cast<int>(i) == selected_index_;
            gc->SetFont(GetFont(), selected ? palette_.onAccent : palette_.text);

            double textWidth  = 0;
            double textHeight = 0;
            gc->GetTextExtent(segments_[i], &textWidth, &textHeight);
            gc->DrawText(segments_[i], x + (segW - textWidth) / 2.0, (size.y - textHeight) / 2.0);
        }
    }

    void SegmentedControl::onLeftDown(wxMouseEvent& event)
    {
        event.Skip();
        const int index = segmentAt(event.GetPosition());
        if (index < 0)
        {
            return;
        }
        const bool changed = index != selected_index_;
        selected_index_      = index;
        Refresh();
        if (changed && on_selection_changed_)
        {
            on_selection_changed_(index);
        }
    }

    void SegmentedControl::onMotion(wxMouseEvent& event)
    {
        event.Skip();
        const int index = segmentAt(event.GetPosition());
        if (index != hovered_index_)
        {
            hovered_index_ = index;
            Refresh();
        }
    }

    void SegmentedControl::onMouseLeave(wxMouseEvent& event)
    {
        event.Skip();
        hovered_index_ = -1;
        Refresh();
    }

    wxSize SegmentedControl::DoGetBestSize() const
    {
        wxClientDC dc(const_cast<SegmentedControl*>(this));
        dc.SetFont(GetFont());

        int totalWidth = 0;
        int maxHeight  = 0;
        for (const auto& segment : segments_)
        {
            wxCoord w = 0, h = 0;
            dc.GetTextExtent(segment, &w, &h);
            totalWidth += w + 2 * kPaddingX;
            maxHeight = std::max(maxHeight, static_cast<int>(h));
        }
        return wxSize(totalWidth, maxHeight + 16);
    }

} // namespace ui::custom_widgets
