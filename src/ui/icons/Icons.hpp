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

    wxBitmap     MakeIcon(const char* pathsSvg);
    wxImageList* CreateImageList();
} // namespace ui::icons