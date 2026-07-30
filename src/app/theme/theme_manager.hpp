#pragma once

#include "theme.hpp"

#include <wx/wx.h>

namespace app::theme
{
    /**
     * @brief Applies application themes to wxWidgets windows.
     */
    class ThemeManager
    {

    public:
        static ThemeManager &get();

        /**
         * @brief Changes current theme mode.
         */
        void setDark(bool enabled);

        /**
         * @brief Returns current theme state.
         */
        bool isDark() const;

        /**
         * @brief Applies theme recursively to a window tree.
         */
        void apply(wxWindow *window);

        /**
         * @brief Changes font size recursively.
         */
        void applyFontSize(wxWindow *window, int points);

    private:
        ThemeManager() = default;
        Theme theme_;
    };

} // namespace app::theme
