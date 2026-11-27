#pragma once

#include "platform/locale/locale_manager.hpp"

#include <initializer_list>
#include <utility>
#include <wx/string.h>

namespace ui::framework::wx
{
    namespace locale
    {
        inline wxString tr(const std::string& key)
        {
            return wxString::FromUTF8(platform::locale::LocaleManager::get().gettext(key));
        }

        /// Translates @p key, then replaces each "{name}" with its value.
        inline wxString tr(const std::string& key, std::initializer_list<std::pair<const char*, wxString>> args)
        {
            wxString text = tr(key);
            for (const auto& [name, value] : args)
                text.Replace(wxString("{") + name + "}", value);
            return text;
        }

    } // namespace locale
} // namespace ui::framework::wx
