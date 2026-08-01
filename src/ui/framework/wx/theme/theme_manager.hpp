#pragma once

#include <wx/wx.h>

namespace ui::framework::wx
{
    namespace theme
    {
        /**
         * @brief Applies application themes to wxWidgets windows.
         */
        class ThemeManager
        {

          public:
            static ThemeManager& get();

            /**
             * @brief Changes current theme mode.
             */
            void setDark(bool enabled);
            /**
             * @brief Returns current theme mode.
             */
            bool isDark() const;

            /**
             * @brief Changes current theme font.
             */
            void setFontSize(int points);
            /**
             * @brief Returns current theme font.
             */
            int getFontSize() const;

            /**
             * @brief Applies theme recursively to a window tree.
             */
            void apply(wxWindow* window);
            /**
             * @brief Changes font size recursively.
             */
            void applyFontSize(wxWindow* window, int points);

          private:
            ThemeManager() = default;
            bool dark_     = false;
            int  fontSize_ = 11;
        };

    } // namespace theme
} // namespace ui::framework::wx
