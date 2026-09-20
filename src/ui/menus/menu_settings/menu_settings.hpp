#pragma once

#include <wx/menu.h>

namespace ui::menus
{
    /**
     * @brief Menu that contains setting-related actions.
     */
    class MenuSettings : public wxMenu
    {
      public:
        /**
         * @brief Constructs the setting menu.
         *
         * Populates the menu with setting-related actions.
         */
        MenuSettings();
    };

} // namespace ui::menus
