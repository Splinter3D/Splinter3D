#include "ui/icons/Icons.hpp"

#include <array>
#include <string>
#include <wx/bmpbndl.h>
#include <wx/button.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/settings.h>
#include <wx/sizer.h>

namespace ui::icons
{
    namespace
    {
        constexpr const char* eyeSvg =
            "<path d='M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z'/>"
            "<circle cx='12' cy='12' r='3'/>";

        constexpr const char* eyeOffSvg =
            "<path d='M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z'/>"
            "<circle cx='12' cy='12' r='3'/>"
            "<line x1='3' y1='3' x2='21' y2='21'/>";

        constexpr const char* trashSvg =
            "<polyline points='3 6 5 6 21 6'/>"
            "<path d='M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6'/>"
            "<path d='M10 11v6M14 11v6'/>"
            "<path d='M9 6V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v2'/>";

        // this ensure that the icons are connected to the enum name
        constexpr std::array<const char*, IconCount> svgs = [] {
            std::array<const char*, IconCount> a{};
            a[IconEye]    = eyeSvg;
            a[IconEyeOff] = eyeOffSvg;
            a[IconTrash]  = trashSvg;
            return a;
        }();
    } // namespace

    wxBitmap MakeIcon(const char* pathsSvg)
    {
        const wxColour    c = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
        const std::string svg =
            std::string("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' "
                        "fill='none' stroke='") +
            c.GetAsString(wxC2S_HTML_SYNTAX).ToStdString() +
            "' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>" +
            pathsSvg + "</svg>";

        return wxBitmapBundle::FromSVG(svg.c_str(), wxSize(svgIconSize, svgIconSize))
            .GetBitmap(wxSize(svgIconSize, svgIconSize));
    }

    /** this create an image list linking the enum to the svgs*/
    wxImageList* CreateImageList()
    {
        auto* images = new wxImageList(svgIconSize, svgIconSize, true);
        for (const char* svg : svgs)
            images->Add(MakeIcon(svg));
        return images;
    }
} // namespace ui::icons