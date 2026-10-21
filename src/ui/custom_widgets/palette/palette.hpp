#pragma once
#include <wx/colour.h>

namespace ui::custom_widgets
{
    /**
     * @brief Colour set used by custom_widgets components to paint themselves.
     */
    struct Palette
    {
        wxColour background;
        wxColour backgroundHover;
        wxColour backgroundPressed;
        wxColour backgroundDisabled;
        wxColour border;
        wxColour borderStrong;
        wxColour text;
        wxColour textDisabled;
        wxColour textMuted;
        wxColour accent;
        wxColour accentHover;
        wxColour accentPressed;
        wxColour accentMuted;

        /**
         * @brief Text/icon colour on top of an accent-filled surface.
         */
        wxColour onAccent;

        wxColour surface;
        wxColour surfaceRaised;
        wxColour success;
        wxColour warning;
        wxColour danger;

        /**
         * @brief Brand accent colour.
         */
        static wxColour Brand() { return wxColour(0xEA, 0x97, 0x3C); }

        /**
         * @brief Dark palette preset.
         */
        static Palette Dark()
        {
            return Palette{
                .background         = wxColour(31, 31, 34),
                .backgroundHover    = wxColour(44, 44, 48),
                .backgroundPressed  = wxColour(24, 24, 26),
                .backgroundDisabled = wxColour(36, 36, 39),
                .border             = wxColour(58, 58, 63),
                .borderStrong       = wxColour(82, 82, 88),
                .text               = wxColour(232, 230, 227),
                .textDisabled       = wxColour(110, 110, 116),
                .textMuted          = wxColour(154, 152, 150),
                .accent             = Brand(),
                .accentHover        = wxColour(0xF2, 0xA8, 0x55),
                .accentPressed      = wxColour(0xC9, 0x7E, 0x2C),
                .accentMuted        = wxColour(0x5A, 0x42, 0x24),
                .onAccent           = wxColour(26, 20, 12),
                .surface            = wxColour(38, 38, 42),
                .surfaceRaised      = wxColour(46, 46, 51),
                .success            = wxColour(74, 170, 108),
                .warning            = wxColour(222, 184, 74),
                .danger             = wxColour(214, 92, 84),
            };
        }

        /**
         * @brief Light palette preset.
         */
        static Palette Light()
        {
            return Palette{
                .background         = wxColour(250, 249, 247),
                .backgroundHover    = wxColour(240, 238, 234),
                .backgroundPressed  = wxColour(228, 225, 220),
                .backgroundDisabled = wxColour(242, 241, 239),
                .border             = wxColour(221, 218, 212),
                .borderStrong       = wxColour(190, 186, 179),
                .text               = wxColour(30, 29, 27),
                .textDisabled       = wxColour(168, 165, 160),
                .textMuted          = wxColour(109, 106, 101),
                .accent             = Brand(),
                .accentHover        = wxColour(0xD9, 0x87, 0x2E),
                .accentPressed      = wxColour(0xBF, 0x74, 0x24),
                .accentMuted        = wxColour(0xFB, 0xEA, 0xD6),
                .onAccent           = wxColour(38, 26, 12),
                .surface            = wxColour(255, 255, 255),
                .surfaceRaised      = wxColour(255, 255, 255),
                .success            = wxColour(47, 140, 86),
                .warning            = wxColour(183, 140, 26),
                .danger             = wxColour(190, 62, 54),
            };
        }
    };

} // namespace ui::custom_widgets
