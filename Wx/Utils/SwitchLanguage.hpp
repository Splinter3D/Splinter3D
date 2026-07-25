// Utility to perform application language switch and recreate main window
#pragma once

#include "../Config/Config.hpp"

#include <functional>
#include <string>
#include <wx/wx.h>

namespace wx::utils
{
    // Replaces `frame` by creating a new one using `createFrame`, finalizing it with `finalize`.
    // - `frame` is the current top-level frame pointer (will be destroyed and replaced).
    // - `code` is the language code to init.
    // - `createFrame` should return a newly constructed frame (not shown).
    // - `finalize` is called with the new frame to perform app-specific init (restore menus, show, etc.).
    // - `cfg` is the loaded AppConfig used to reapply theme/font preferences.
    void switchLanguage(wxFrame*&                     frame,
                        const std::string&            code,
                        std::function<wxFrame*()>     createFrame,
                        std::function<void(wxFrame*)> finalize,
                        const wx::config::AppConfig&  cfg);

} // namespace wx::utils
