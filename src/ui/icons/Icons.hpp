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

    wxBitmap     MakeIcon(const char* pathsSvg);
    wxImageList* CreateImageList();
} // namespace ui::icons
