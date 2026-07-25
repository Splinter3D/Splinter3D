// BindEvents — orchestrates smaller bind helpers (Basic / Font / Lang)
#pragma once

#include <functional>
#include <string>
#include <wx/wx.h>

namespace wx::events
{
    class BindEvents
    {
      public:
        explicit BindEvents(wxFrame* frame);
        void bindAll(const std::function<void(const std::string&)>& onSwitch);

      private:
        wxFrame* m_frame = nullptr;
    };

} // namespace wx::events
