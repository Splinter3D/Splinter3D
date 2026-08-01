#pragma once

#include "locale_manager.hpp"

#include <wx/string.h>

namespace ui::framework::wx
{
    namespace locale
    {
        inline wxString tr(const std::string& key)
        {
            return wxString::FromUTF8(LocaleManager::get().gettext(key));
        }

    } // namespace locale
} // namespace ui::framework::wx
