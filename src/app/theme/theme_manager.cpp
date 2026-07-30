#include "theme_manager.hpp"

namespace app::theme
{
    ThemeManager &ThemeManager::get()
    {
        static ThemeManager instance;
        return instance;
    }

    void ThemeManager::setDark(bool enabled)
    {
        theme_.dark = enabled;
    }

    bool ThemeManager::isDark() const
    {
        return theme_.dark;
    }

    void ThemeManager::apply(wxWindow *window)
    {
        if (!window)
            return;

        const wxColour background =
            theme_.dark
                ? wxColour(30, 30, 30)
                : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

        const wxColour foreground =
            theme_.dark
                ? wxColour(220, 220, 220)
                : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

        window->SetBackgroundColour(background);
        window->SetForegroundColour(foreground);

        for (auto *child : window->GetChildren())
            apply(child);

        window->Refresh();
    }

    void ThemeManager::applyFontSize(wxWindow *window, int points)
    {
        if (!window)
            return;

        wxFont font = window->GetFont();

        font.SetPointSize(points);

        window->SetFont(font);

        for (auto *child : window->GetChildren())
            applyFontSize(child, points);

        window->Refresh();
        window->Layout();
    }

} // namespace app::theme
