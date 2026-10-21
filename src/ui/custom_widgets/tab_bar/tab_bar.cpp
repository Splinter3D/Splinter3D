#include "tab_bar.hpp"

#include <algorithm>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace ui::custom_widgets
{
    TabBar::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    TabBar::Builder& TabBar::Builder::setTabs(std::vector<wxString> tabs)
    {
        tabs_ = std::move(tabs);
        return *this;
    }

    TabBar::Builder& TabBar::Builder::setSelectedIndex(int index)
    {
        selected_index_ = index;
        return *this;
    }

    TabBar::Builder& TabBar::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    TabBar::Builder& TabBar::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    TabBar::Builder& TabBar::Builder::onTabChanged(std::function<void(int)> callback)
    {
        on_tab_changed_ = std::move(callback);
        return *this;
    }

    TabBar* TabBar::Builder::build() const
    {
        return new TabBar(parent_, tabs_, selected_index_, palette_, on_tab_changed_);
    }

    TabBar::TabBar(wxWindow*                 parent,
                   std::vector<wxString>     tabs,
                   int                       selected_index,
                   Palette                   palette,
                   std::function<void(int)> on_tab_changed)
        : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          tabs_(std::move(tabs)),
          selected_index_(selected_index),
          palette_(std::move(palette)),
          on_tab_changed_(std::move(on_tab_changed))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &TabBar::onPaint, this);
        Bind(wxEVT_LEFT_DOWN, &TabBar::onLeftDown, this);
        Bind(wxEVT_MOTION, &TabBar::onMotion, this);
        Bind(wxEVT_LEAVE_WINDOW, &TabBar::onMouseLeave, this);

        SetInitialSize(DoGetBestSize());
    }

    void TabBar::setPalette(const Palette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    void TabBar::setSelectedIndex(int index)
    {
        if (index < 0 || index >= static_cast<int>(tabs_.size()) || index == selected_index_)
        {
            return;
        }
        selected_index_ = index;
        Refresh();
    }

    int TabBar::tabAt(const wxPoint& pos) const
    {
        const wxSize size    = GetClientSize();
        const int    tabW    = tabs_.empty() ? 0 : size.x / static_cast<int>(tabs_.size());
        if (tabW <= 0)
        {
            return -1;
        }
        const int index = pos.x / tabW;
        return (index >= 0 && index < static_cast<int>(tabs_.size())) ? index : -1;
    }

    void TabBar::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc || tabs_.empty())
        {
            return;
        }

        const wxSize size = GetClientSize();
        const double tabW = static_cast<double>(size.x) / static_cast<double>(tabs_.size());

        gc->SetBrush(wxBrush(palette_.surface));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, size.x, size.y);

        for (std::size_t i = 0; i < tabs_.size(); ++i)
        {
            const double x = static_cast<double>(i) * tabW;

            if (static_cast<int>(i) == hovered_index_ && static_cast<int>(i) != selected_index_)
            {
                gc->SetBrush(wxBrush(palette_.backgroundHover));
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRectangle(x, 0, tabW, size.y);
            }

            const bool selected = static_cast<int>(i) == selected_index_;
            gc->SetFont(selected ? GetFont().Bold() : GetFont(),
                        selected ? palette_.text : palette_.textMuted);

            double textWidth  = 0;
            double textHeight = 0;
            gc->GetTextExtent(tabs_[i], &textWidth, &textHeight);
            gc->DrawText(tabs_[i], x + (tabW - textWidth) / 2.0, (size.y - textHeight) / 2.0);

            if (selected)
            {
                gc->SetBrush(wxBrush(palette_.accent));
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRectangle(x, size.y - kIndicatorHeight, tabW, kIndicatorHeight);
            }
        }
    }

    void TabBar::onLeftDown(wxMouseEvent& event)
    {
        event.Skip();
        const int index = tabAt(event.GetPosition());
        if (index < 0)
        {
            return;
        }
        const bool changed = index != selected_index_;
        selected_index_      = index;
        Refresh();
        if (changed && on_tab_changed_)
        {
            on_tab_changed_(index);
        }
    }

    void TabBar::onMotion(wxMouseEvent& event)
    {
        event.Skip();
        const int index = tabAt(event.GetPosition());
        if (index != hovered_index_)
        {
            hovered_index_ = index;
            Refresh();
        }
    }

    void TabBar::onMouseLeave(wxMouseEvent& event)
    {
        event.Skip();
        hovered_index_ = -1;
        Refresh();
    }

    wxSize TabBar::DoGetBestSize() const
    {
        wxClientDC dc(const_cast<TabBar*>(this));
        dc.SetFont(GetFont().Bold());

        int totalWidth = 0;
        int maxHeight  = 0;
        for (const auto& tab : tabs_)
        {
            wxCoord w = 0, h = 0;
            dc.GetTextExtent(tab, &w, &h);
            totalWidth += w + 2 * kTabPaddingX;
            maxHeight = std::max(maxHeight, static_cast<int>(h));
        }
        return wxSize(totalWidth, maxHeight + 20);
    }

} // namespace ui::custom_widgets
