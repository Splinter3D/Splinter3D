#pragma once

#include <vector>
#include <wx/bmpbndl.h>

namespace ui::icons
{
    constexpr int svgIconSize     = 18;
    constexpr int svgIconColWidth = 36;

    enum IconIndex
    {
        IconEye    = 0,
        IconEyeOff = 1,
        IconTrash  = 2,
        IconCount
    };

    // Standalone icons (not in the image list): pass to MakeIcon().
    inline constexpr const char* modelsSvg =
        "<path d='M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16Z'/>"
        "<path d='m3.3 7 8.7 5 8.7-5'/><path d='M12 22V12'/>";
    inline constexpr const char* tasksSvg =
        "<path d='m3 17 2 2 4-4'/><path d='m3 7 2 2 4-4'/>"
        "<path d='M13 6h8'/><path d='M13 12h8'/><path d='M13 18h8'/>";
    inline constexpr const char* settingsSvg =
        "<path d='M21 4h-7M10 4H3M21 12h-9M8 12H3M21 20h-5M12 20H3'/>"
        "<path d='M14 2v4M8 10v4M16 18v4'/>";
    inline constexpr const char* undoSvg =
        "<path d='M9 14 4 9l5-5'/><path d='M4 9h10.5a5.5 5.5 0 0 1 0 11H11'/>";
    inline constexpr const char* redoSvg =
        "<path d='m15 14 5-5-5-5'/><path d='M20 9H9.5a5.5 5.5 0 0 0 0 11H13'/>";

    /** @brief The icon at size DIPs, sharp at any display scale. */
    wxBitmapBundle MakeIcon(const char* pathsSvg, int size = svgIconSize);
    /** @brief The list icons, indexed by IconIndex (for wxListCtrl::SetSmallImages). */
    std::vector<wxBitmapBundle> ListIcons();
} // namespace ui::icons
