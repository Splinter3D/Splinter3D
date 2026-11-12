#pragma once

#include <wx/bitmap.h>
#include <wx/imaglist.h>

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
    inline constexpr const char* tasksSvg =
        "<path d='m3 17 2 2 4-4'/><path d='m3 7 2 2 4-4'/>"
        "<path d='M13 6h8'/><path d='M13 12h8'/><path d='M13 18h8'/>";
    inline constexpr const char* settingsSvg =
        "<path d='M21 4h-7M10 4H3M21 12h-9M8 12H3M21 20h-5M12 20H3'/>"
        "<path d='M14 2v4M8 10v4M16 18v4'/>";

    wxBitmap     MakeIcon(const char* pathsSvg);
    wxImageList* CreateImageList();
} // namespace ui::icons
