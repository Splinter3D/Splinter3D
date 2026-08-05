#include "menu_registration.hpp"

#include "example_menu/example_menu.hpp"
#include "menu_registry.hpp"
#include "ui/framework/wx/ids/ids.hpp"

// Add one include + one Register(...) line per real menu as they get built.

namespace ui::menus
{
    void RegisterAllMenus()
    {
        // Retrieve the singleton registry responsible for menu factories.
        auto& registry = MenuRegistry::getInstance();

        // Register the example menu.
        //
        // Each menu is identified by a unique id and associated with a
        // factory capable of constructing a fresh wxMenu instance whenever
        // a window requests it.
        registry.Register(ui::framework::wx::ids::menus::kExample,
                          []() {
                              return new ExampleMenu();
                          });
    }

} // namespace ui::menus
