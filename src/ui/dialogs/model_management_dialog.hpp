#pragma once

#include "ui/framework/wx/floating_menu/floating_menu.hpp"

#include <functional>
#include <vector>
#include <wx/listctrl.h>

namespace ui::dialogs
{
    class ModelManagementDialog final : public ui::framework::wx::FloatingMenu
    {
      public:
        ModelManagementDialog(wxWindow*                              parent,
                              std::function<std::vector<wxString>()> names,
                              std::function<std::vector<bool>()>     visibility,
                              std::function<void(int, bool)>         setVisibility,
                              std::function<void(int)>               remove,
                              std::function<void(int)>               focus);

        void RefreshModels();

      private:
        void OnItemActivated(wxListEvent& event);
        void OnToggle(wxCommandEvent& event);
        void OnDelete(wxCommandEvent& event);

        wxListCtrl*                            list_ = nullptr;
        std::function<std::vector<wxString>()> names_;
        std::function<std::vector<bool>()>     visibility_;
        std::function<void(int, bool)>         set_visibility_;
        std::function<void(int)>               remove_;
        std::function<void(int)>               focus_;
    };
} // namespace ui::dialogs