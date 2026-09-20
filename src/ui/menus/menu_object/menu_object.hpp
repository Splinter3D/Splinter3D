#pragma once

#include <wx/menu.h>

namespace ui::menus
{
    /**
     * @brief Menu that contains object-related actions.
     */
    class MenuObject : public wxMenu
    {
      public:
        /**
         * @brief Constructs the object menu.
         *
         * Populates the menu with object-related actions such as New, Open, Save, and Exit.
         */
        MenuObject();
    };

} // namespace ui::menus
