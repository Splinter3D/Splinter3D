#include "dropdown.hpp"

#include <algorithm>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <wx/graphics.h>
#include <wx/popupwin.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr double kCornerRadius = 4.0;
        constexpr int    kPaddingX     = 10;
        constexpr int    kChevronBox   = 24;
        constexpr int    kItemHeight   = 26;
        constexpr int    kPopupPadding = 4;
        constexpr int    kMaxPopupRows = 10;
    } // namespace

    class Dropdown::ListPopup : public wxPopupTransientWindow
    {
      public:
        ListPopup(wxWindow*                    parent,
                  const std::vector<wxString>& choices,
                  Palette                      palette,
                  const wxFont&                font,
                  std::function<void(int)>     on_picked)
            : wxPopupTransientWindow(parent, wxBORDER_NONE),
              choices_(choices),
              palette_(std::move(palette)),
              on_picked_(std::move(on_picked))
        {
            SetFont(font);
            SetBackgroundStyle(wxBG_STYLE_PAINT);

            Bind(wxEVT_PAINT, &ListPopup::onPaint, this);
            Bind(wxEVT_MOTION, &ListPopup::onMotion, this);
            Bind(wxEVT_LEFT_DOWN, &ListPopup::onLeftDown, this);
            Bind(wxEVT_LEFT_UP, &ListPopup::onLeftUp, this);
            Bind(wxEVT_LEAVE_WINDOW, &ListPopup::onMouseLeave, this);
        }

        /**
         * @brief Sizes the list to its owner's width and shows it directly
         * below, flipping above when there is no room underneath.
         */
        void popupBelow(wxWindow* owner, int selected_index)
        {
            selected_index_ = selected_index;
            hovered_index_  = -1;
            pressed_inside_ = false;
            dragged_inside_ = false;

            const int rows   = std::min(static_cast<int>(choices_.size()), kMaxPopupRows);
            const int height = rows * kItemHeight + 2 * kPopupPadding;
            SetSize(owner->GetSize().x, height);

            const wxPoint ownerScreenPos = owner->GetScreenPosition();
            wxPoint       position(ownerScreenPos.x, ownerScreenPos.y + owner->GetSize().y);

            const int displayIndex = wxDisplay::GetFromWindow(owner);
            const wxRect screen =
                wxDisplay(displayIndex == wxNOT_FOUND ? 0u
                                                      : static_cast<unsigned int>(displayIndex))
                    .GetClientArea();
            if (position.y + height > screen.GetBottom())
            {
                position.y = ownerScreenPos.y - height;
            }

            Position(position, wxSize(0, 0));
            Popup();
        }

      private:
        int itemAt(const wxPoint& pos) const
        {
            const int index = (pos.y - kPopupPadding) / kItemHeight;
            return (index >= 0 && index < static_cast<int>(choices_.size())) ? index : -1;
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

            gc->SetBrush(wxBrush(palette_.surfaceRaised));
            gc->SetPen(wxPen(palette_.borderStrong, 1));
            gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, kCornerRadius);

            for (std::size_t i = 0; i < choices_.size(); ++i)
            {
                const double y        = kPopupPadding + static_cast<double>(i) * kItemHeight;
                const bool   selected = static_cast<int>(i) == selected_index_;
                const bool   hovered  = static_cast<int>(i) == hovered_index_;

                if (hovered)
                {
                    gc->SetBrush(wxBrush(palette_.accent));
                    gc->SetPen(*wxTRANSPARENT_PEN);
                    gc->DrawRoundedRectangle(kPopupPadding, y,
                                              size.x - 2.0 * kPopupPadding, kItemHeight,
                                              kCornerRadius - 1.0);
                }

                gc->SetFont(GetFont(), hovered    ? palette_.onAccent
                                       : selected ? palette_.accent
                                                  : palette_.text);
                double textWidth  = 0;
                double textHeight = 0;
                gc->GetTextExtent(choices_[i], &textWidth, &textHeight);
                gc->DrawText(choices_[i], kPopupPadding + kPaddingX,
                             y + (kItemHeight - textHeight) / 2.0);
            }
        }

        void onMotion(wxMouseEvent& event)
        {
            event.Skip();

            if (event.LeftIsDown())
            {
                dragged_inside_ = true;
            }

            const int index = itemAt(event.GetPosition());
            if (index != hovered_index_)
            {
                hovered_index_ = index;
                Refresh();
            }
        }

        void onMouseLeave(wxMouseEvent& event)
        {
            event.Skip();
            if (hovered_index_ != -1)
            {
                hovered_index_ = -1;
                Refresh();
            }
        }

        void onLeftDown(wxMouseEvent& event)
        {
            event.Skip();
            pressed_inside_ = true;
        }

        void onLeftUp(wxMouseEvent& event)
        {
            event.Skip();

            if (!pressed_inside_ && !dragged_inside_)
            {
                return;
            }

            const int index = itemAt(event.GetPosition());
            if (index >= 0 && on_picked_)
            {
                on_picked_(index);
            }
            Dismiss();
        }

        std::vector<wxString>    choices_;
        int                      selected_index_ = 0;
        int                      hovered_index_  = -1;
        bool                     pressed_inside_ = false;
        bool                     dragged_inside_ = false;
        Palette                  palette_;
        std::function<void(int)> on_picked_;
    };

    Dropdown::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    Dropdown::Builder& Dropdown::Builder::setChoices(std::vector<wxString> choices)
    {
        choices_ = std::move(choices);
        return *this;
    }

    Dropdown::Builder& Dropdown::Builder::setSelectedIndex(int index)
    {
        selected_index_ = index;
        return *this;
    }

    Dropdown::Builder& Dropdown::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    Dropdown::Builder& Dropdown::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    Dropdown::Builder& Dropdown::Builder::onSelectionChanged(std::function<void(int)> callback)
    {
        on_selection_changed_ = std::move(callback);
        return *this;
    }

    Dropdown* Dropdown::Builder::build() const
    {
        return new Dropdown(parent_, choices_, selected_index_, palette_, on_selection_changed_);
    }

    Dropdown::Dropdown(wxWindow*                 parent,
                        std::vector<wxString>     choices,
                        int                       selected_index,
                        Palette                   palette,
                        std::function<void(int)> on_selection_changed)
        : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
          choices_(std::move(choices)),
          selected_index_(selected_index),
          palette_(std::move(palette)),
          on_selection_changed_(std::move(on_selection_changed))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &Dropdown::onPaint, this);
        Bind(wxEVT_ENTER_WINDOW, &Dropdown::onMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &Dropdown::onMouseLeave, this);
        Bind(wxEVT_LEFT_DOWN, &Dropdown::onLeftDown, this);

        Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& event) {
            event.Skip();
            if (popup_ != nullptr)
            {
                popup_->Destroy();
                popup_ = nullptr;
            }
        });

        SetInitialSize(DoGetBestSize());
    }

    void Dropdown::setPalette(const Palette& palette)
    {
        palette_ = palette;
        Refresh();
    }

    void Dropdown::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
        {
            return;
        }

        const wxSize size = GetClientSize();

        gc->SetBrush(wxBrush(hovered_ || isPopupOpen() ? palette_.backgroundHover
                                                    : palette_.background));
        gc->SetPen(wxPen(isPopupOpen() ? palette_.accent : palette_.border, 1));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, kCornerRadius);

        const wxString label =
            (selected_index_ >= 0 && selected_index_ < static_cast<int>(choices_.size()))
                ? choices_[static_cast<std::size_t>(selected_index_)]
                : wxString();

        gc->SetFont(GetFont(), palette_.text);
        double textWidth  = 0;
        double textHeight = 0;
        gc->GetTextExtent(label, &textWidth, &textHeight);
        gc->DrawText(label, kPaddingX, (size.y - textHeight) / 2.0);

        const double cx = size.x - kChevronBox / 2.0;
        const double cy = size.y / 2.0;
        wxGraphicsPath path = gc->CreatePath();
        if (isPopupOpen())
        {
            path.MoveToPoint(cx - 4, cy + 2);
            path.AddLineToPoint(cx, cy - 2);
            path.AddLineToPoint(cx + 4, cy + 2);
        }
        else
        {
            path.MoveToPoint(cx - 4, cy - 2);
            path.AddLineToPoint(cx, cy + 2);
            path.AddLineToPoint(cx + 4, cy - 2);
        }
        gc->SetPen(wxPen(isPopupOpen() ? palette_.accent : palette_.textMuted, 2));
        gc->StrokePath(path);
    }

    void Dropdown::onMouseEnter(wxMouseEvent& event)
    {
        event.Skip();
        hovered_ = true;
        Refresh();
    }

    void Dropdown::onMouseLeave(wxMouseEvent& event)
    {
        event.Skip();
        hovered_ = false;
        Refresh();
    }

    void Dropdown::onLeftDown(wxMouseEvent& event)
    {
        event.Skip();
        openPopup();
    }

    void Dropdown::selectIndex(int index)
    {
        if (index < 0 || index >= static_cast<int>(choices_.size()) || index == selected_index_)
        {
            return;
        }
        selected_index_ = index;
        Refresh();
        if (on_selection_changed_)
        {
            on_selection_changed_(index);
        }
    }

    bool Dropdown::isPopupOpen() const
    {
        return popup_ != nullptr && popup_->IsShown();
    }

    void Dropdown::openPopup()
    {
        if (choices_.empty() || isPopupOpen())
        {
            return;
        }

        if (popup_ == nullptr)
        {
            popup_ = new ListPopup(this, choices_, palette_, GetFont(),
                                    [this](int index) { selectIndex(index); });

            popup_->Bind(wxEVT_SHOW, [this](wxShowEvent& event) {
                event.Skip();
                Refresh();
            });
        }

        Refresh();
        popup_->popupBelow(this, selected_index_);
    }

    wxSize Dropdown::DoGetBestSize() const
    {
        wxClientDC dc(const_cast<Dropdown*>(this));
        dc.SetFont(GetFont());

        wxCoord maxWidth = 0;
        wxCoord height   = 0;
        for (const auto& choice : choices_)
        {
            wxCoord w = 0, h = 0;
            dc.GetTextExtent(choice, &w, &h);
            maxWidth = std::max(maxWidth, w);
            height   = std::max(height, h);
        }
        return wxSize(maxWidth + kPaddingX + kChevronBox, height + 16);
    }

} // namespace ui::custom_widgets
