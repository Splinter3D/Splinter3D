#pragma once
#include <wx/wx.h>

namespace wx::theme
{

    // ─────────────────────────────────────────────
    //  Theme — dark/light + font size, recursive
    // ─────────────────────────────────────────────
    class Theme
    {
      public:
        static Theme& get()
        {
            static Theme instance;
            return instance;
        }

        bool isDark() const
        {
            return m_dark;
        }

        void setDark(bool dark)
        {
            m_dark = dark;
        }

        // Recursively repaint a window tree with current theme colours
        void apply(wxWindow* win) const
        {
            wxColour bg = m_dark ? wxColour(30, 30, 30)
                                 : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
            wxColour fg = m_dark ? wxColour(220, 220, 220)
                                 : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
            win->SetBackgroundColour(bg);
            win->SetForegroundColour(fg);
            for (auto* child : win->GetChildren())
                apply(child);
            win->Refresh();
        }

        // Recursively change font size on a window tree
        void applyFontSize(wxWindow* win, int pts) const
        {
            wxFont f = win->GetFont();
            f.SetPointSize(pts);
            win->SetFont(f);
            for (auto* child : win->GetChildren())
                applyFontSize(child, pts);
            win->Refresh();
            win->Layout();
        }

      private:
        bool m_dark = false;
        Theme()     = default;
    };

} // namespace wx