#pragma once

#include <wx/menu.h>

namespace ui::menus
{
    /**
     * @brief Temporary menu used to validate the menu infrastructure.
     *
     * This menu exists solely to verify that the complete menu creation
     * pipeline is working correctly:
     *
     * - RegisterAllMenus()
     * - MenuRegistry
     * - MainWindow::InitMenuBar()
     * - wxMenuBar
     *
     * Once real application menus (File, Edit, View, Help...) are
     * implemented, this class can safely be removed along with its
     * registration.
     */
    class ExampleMenu : public wxMenu
    {
      public:
        /**
         * @brief Constructs the example menu.
         *
         * Populates the menu with a few placeholder entries using the
         * application's menu identifiers.
         */
        ExampleMenu();
    };

} // namespace ui::menus
