#include "theme_manager.hpp"

namespace ui::framework::wx
{
    namespace theme
    {
        ThemeManager& ThemeManager::get()
        {
            static ThemeManager instance;
            return instance;
        }

        void ThemeManager::setDark(bool enabled)
        {
            dark_ = enabled;
        }

        bool ThemeManager::isDark() const
        {
            return dark_;
        }

        void ThemeManager::setFontSize(int points)
        {
            font_size_ = points;
        }

        int ThemeManager::getFontSize() const
        {
            return font_size_;
        }

        void ThemeManager::apply(wxWindow* window)
        {
            if (!window)
                return;

            const wxColour background =
                dark_
                    ? wxColour(30, 30, 30)
                    : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

            const wxColour foreground =
                dark_
                    ? wxColour(220, 220, 220)
                    : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

            window->SetBackgroundColour(background);
            window->SetForegroundColour(foreground);

            for (auto* child : window->GetChildren())
                apply(child);

            window->Refresh();
        }

        void ThemeManager::applyFontSize(wxWindow* window, int points)
        {
            if (!window)
                return;

            wxFont font = window->GetFont();

            font.SetPointSize(points);

            window->SetFont(font);

            for (auto* child : window->GetChildren())
                applyFontSize(child, points);

            window->Refresh();
            window->Layout();
        }

    } // namespace theme
} // namespace ui::framework::wx
