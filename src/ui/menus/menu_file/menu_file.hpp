#pragma once

#include <wx/menu.h>

namespace ui::menus
{
    /**
     * @brief Menu that contains file-related actions.
     */
    class MenuFile : public wxMenu
    {
      public:
        /**
         * @brief Constructs the file menu.
         *
         * Populates the menu with file-related actions such as New, Open,
         * Save, Import, Export, and Exit.
         */
        MenuFile();
    };

} // namespace ui::menus
