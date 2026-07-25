// Centralized application IDs
#pragma once

#include <wx/wx.h>

namespace IDs
{
    enum BASIC
    {
        THEME_TOGGLE = wxID_HIGHEST + 1,
        FULLSCREEN,
    };

    // Reserve ranges to avoid colliding with wx predefined IDs
    enum FONT
    {
        FONT_BASE = wxID_HIGHEST + 100,
        SMALL     = FONT_BASE,
        NORMAL,
        LARGE,
        XLARGE,
    };

    enum LANG
    {
        LANG_BASE = wxID_HIGHEST + 200,
        DEFAULT   = LANG_BASE,
        EN,
        FR,
        ES,
        DE,
    };
} // namespace IDs
