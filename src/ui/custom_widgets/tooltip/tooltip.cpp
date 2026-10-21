#include "tooltip.hpp"

#include <algorithm>
#include <map>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <wx/graphics.h>

namespace ui::custom_widgets
{
    namespace
    {
        constexpr double kCornerRadius = 4.0;

        Tooltip*& tooltipSlotFor(wxWindow* topLevel)
        {
            static std::map<wxWindow*, Tooltip*> slots;
            return slots[topLevel];
        }
    } // namespace

    void Tooltip::Attach(wxWindow* target, const wxString& text, const Palette& palette)
    {
        if (target == nullptr)
        {
            return;
        }

        wxWindow* topLevel = wxGetTopLevelParent(target);
        if (topLevel == nullptr)
        {
            return;
        }

        Tooltip*& tooltip = tooltipSlotFor(topLevel);
        if (tooltip == nullptr)
        {
            tooltip = new Tooltip(topLevel, palette);

            topLevel->Bind(wxEVT_DESTROY, [topLevel](wxWindowDestroyEvent& event) {
                event.Skip();
                tooltipSlotFor(topLevel) = nullptr;
            });
        }

        target->Bind(wxEVT_ENTER_WINDOW, [tooltip, target, text](wxMouseEvent& event) {
            event.Skip();
            tooltip->showFor(target, text);
        });
        target->Bind(wxEVT_LEAVE_WINDOW, [tooltip](wxMouseEvent& event) {
            event.Skip();
            tooltip->hide();
        });
        target->Bind(wxEVT_LEFT_DOWN, [tooltip](wxMouseEvent& event) {
            event.Skip();
            tooltip->hide();
        });
    }

    Tooltip::Tooltip(wxWindow* parent, Palette palette)
        : wxPopupWindow(parent, wxBORDER_NONE),
          palette_(std::move(palette)),
          show_timer_(this)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &Tooltip::onPaint, this);

        Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
            if (pending_target_ == nullptr)
            {
                return;
            }

            wxClientDC dc(this);
            dc.SetFont(GetFont());
            wxCoord textWidth  = 0;
            wxCoord textHeight = 0;
            dc.GetTextExtent(text_, &textWidth, &textHeight);

            const wxSize popupSize(textWidth + 2 * kPaddingX, textHeight + 2 * kPaddingY);
            SetSize(popupSize);

            const wxRect targetRect(pending_target_->GetScreenPosition(),
                                     pending_target_->GetSize());
            wxPoint position(targetRect.x, targetRect.GetBottom() + kGapToHost);

            const int displayIndex = wxDisplay::GetFromWindow(pending_target_);
            const wxRect screen =
                wxDisplay(displayIndex == wxNOT_FOUND
                              ? 0u
                              : static_cast<unsigned int>(displayIndex))
                    .GetClientArea();
            if (position.y + popupSize.y > screen.GetBottom())
            {
                position.y = targetRect.y - popupSize.y - kGapToHost;
            }
            position.x = std::min(position.x, screen.GetRight() - popupSize.x);
            position.x = std::max(position.x, screen.x);

            Position(position, wxSize(0, 0));
            Show();
            Refresh();
        });
    }

    void Tooltip::showFor(wxWindow* target, const wxString& text)
    {
        if (text.empty())
        {
            hide();
            return;
        }
        text_          = text;
        pending_target_ = target;
        show_timer_.StartOnce(kDelayMs);
    }

    void Tooltip::hide()
    {
        show_timer_.Stop();
        pending_target_ = nullptr;
        if (IsShown())
        {
            Hide();
        }
    }

    void Tooltip::onPaint(wxPaintEvent&)
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
        gc->SetPen(wxPen(palette_.border, 1));
        gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1.0, size.y - 1.0, kCornerRadius);

        gc->SetFont(GetFont(), palette_.text);
        double textWidth  = 0;
        double textHeight = 0;
        gc->GetTextExtent(text_, &textWidth, &textHeight);
        gc->DrawText(text_, kPaddingX, (size.y - textHeight) / 2.0);
    }

} // namespace ui::custom_widgets
