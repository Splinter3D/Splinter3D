// Language bindings helper
#pragma once

#include <functional>
#include <string>
#include <wx/wx.h>

namespace wx::events
{
    void bindLang(wxFrame* frame, std::function<void(const std::string&)> onSwitch);

} // namespace wx::events
