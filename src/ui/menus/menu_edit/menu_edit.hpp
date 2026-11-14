#pragma once

#include <wx/menu.h>

namespace ui::menus
{
    /**
     * @brief Menu that contains editing actions.
     */
    class MenuEdit : public wxMenu
    {
      public:
        /**
         * @brief Constructs the edit menu: Undo and Redo.
         */
        MenuEdit();
    };

} // namespace ui::menus
