#pragma once
#include <wx/popupwin.h>
#include <wx/timer.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn tooltip popup.
     */
    class Tooltip : public wxPopupWindow
    {
      public:
        /**
         * @brief Attaches a tooltip to the target window.
         *
         * @param target Window to attach the tooltip to.
         * @param text Tooltip text to display.
         * @param palette Colours used to paint the popup.
         */
        static void Attach(wxWindow*       target,
                           const wxString& text,
                           const Palette&  palette = Palette::Dark());

      private:
        Tooltip(wxWindow* parent, Palette palette);

        void showFor(wxWindow* target, const wxString& text);
        void hide();
        void onPaint(wxPaintEvent& event);

        wxString  text_;
        Palette   palette_;
        wxTimer   show_timer_;
        wxWindow* pending_target_ = nullptr;

        static constexpr int kPaddingX  = 8;
        static constexpr int kPaddingY  = 5;
        static constexpr int kDelayMs   = 450;
        static constexpr int kGapToHost = 6;
    };

} // namespace ui::custom_widgets
