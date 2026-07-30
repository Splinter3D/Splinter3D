#pragma once

#include "locale_manager.hpp"
#include <wx/string.h>

namespace app::locale
{
    inline wxString tr(const std::string &key)
    {
        return wxString::FromUTF8(LocaleManager::get().gettext(key));
    }

} // namespace app::locale
